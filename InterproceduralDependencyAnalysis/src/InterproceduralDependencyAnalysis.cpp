#include <chrono>
#include <fstream>
#include <iostream>


#include "InterproceduralDependencyAnalysis.h"

using namespace llvm;


// debug
bool output_dependency = 0;


void ida::InterproceduralDependencyAnalysis::getAnalysisUsage(AnalysisUsage& AU) const {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<CallGraph>();
    AU.addRequired<BranchDependencyAnalysis>();
    AU.setPreservesAll();
}


char ida::InterproceduralDependencyAnalysis::ID = 0;
static RegisterPass<ida::InterproceduralDependencyAnalysis> IDA("ida",
    "Analyse Inter-procedure data dependency", false, true);


std::ofstream globalStats;


bool ida::InterproceduralDependencyAnalysis::runOnModule(Module &M) {

    // I forget why not to use existed llvm pass :)
    _CG = &getAnalysis<CallGraph>();

    // get branch dependency analysis result
    _BDA = &getAnalysis<BranchDependencyAnalysis>();
    _FBD = _BDA->getBranchDependency();
    
    // get module name and configure global statistics file
    std::string module_name = getModuleName(M);
    if (!initGlobalStats(globalStats, module_name, true)) {
        std::cout << "Create global statistics fails\n";

        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // candidate StoreInsts and RetInsts
    unsigned store_num = 0, ret_num = 0;
    unsigned all_store_num = 0, all_ret_num = 0;
    for (Function &F: M) {
        if (F.isDeclaration() || F.empty())
            continue;

        for (auto inst_iter = inst_begin(&F); inst_iter != inst_end(&F); inst_iter++) {

            // store data to non local variable
            if (auto *SI = dyn_cast<StoreInst>(&(*inst_iter))) {
                all_store_num += 1;

                Value *data = SI->getOperand(0);

                // two filters
                if (!isa<Argument>(data)) {
                    if (data->getType()->isIntegerTy()) {
                        StoreInsts[&F].insert(SI);  
                        store_num += 1;
                    }
                }
            }

            // that is, some local variables have inter-procedural impacts
            if(auto *RI = dyn_cast<ReturnInst>(&(*inst_iter))) {
                all_ret_num += 1;

                Value *ret = RI->getReturnValue();
                if (ret) {

                    // ret struct abc* addr
                    Type *t = ret->getType();
                    if (t->isPointerTy() && t->getPointerElementType()->isStructTy()) {
                        RetFuncs.insert(&F);
                        ret_num += 1;
                    }
                }
            }
        }
    }

    globalStats << "[IDA] Valid StoreInst: " << store_num << "/" << all_store_num << \
                    ", ReturnInst: " << ret_num << "/" << all_ret_num << "\n";


    // find all data dependency for above candidatas
    for (Function &F: M) {
        if (F.isDeclaration() || F.empty())
            continue;

        LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
        DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();

        if (StoreInsts.find(&F) != StoreInsts.end()) {
            findStoreDependencyFromSI(&F, &DT, &LI);
        }

        if (RetFuncs.find(&F) != RetFuncs.end()) {
            findStoreDependencyFromRI(&F);
        }
    }

    auto mid = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
    globalStats << "[IDA] Find useful store instructions takes: " <<  duration1.count() << "\n";

    // match store instructions and branch instructions
    unsigned branch_num = 0;
    
    for(auto it: _FBD) {
        Function &F = *(it.first);
        if (F.isDeclaration() || F.empty())
            continue;

        LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
        DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();

        BranchDepSet BDSet = it.second;
        for (auto BD: BDSet) {
            BD2VDDep *bvDep = new BD2VDDep();
   
            unsigned BD_var_idx = 0;

            // for limitations, there is only one data_dep
            for (auto data_dep: BD->data_dep_set) {   
                
                // directly find stores
                if (data_dep->node_type != rn_nonPointerParam) {
                    VD2SDDep *vsDep = new VD2SDDep();
                    vsDep->id = BD_var_idx;
                    vsDep->data_dep = data_dep;

                    findStoresForBranch(BD, data_dep, vsDep, &DT, &LI);
                    bvDep->varDeps.insert(vsDep);

                    BD_var_idx++;
                }
                
                // trace function call graph to find store instructions
                else {
                    findStoresForBranchOnCG(BD, data_dep, bvDep, &DT, &LI);
                }
            }              

            branch_num++;
            bvDep->id = branch_num;
            bvDep->branch_dep = BD;

            bvDep->store_num = 0;
            for (auto varDep: bvDep->varDeps) {
                bvDep->store_num += varDep->storeDeps.size();
            }

            outs() << "[IDA] Find " << bvDep->store_num << " StoreInsts for" << *BD->inst << "\n";

            _BD2VDMap.insert(bvDep);   
        }
    }

    // branch dependency stats
    showStructPointersInfo(M);

    // output
    if (output_dependency) {
        outputBD2SDsMap();
    }

    // set store-branch inter-procedural dependency to above instructions
    setInstMetaData();

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop - mid);
    globalStats << "[IDA] Find store instruction for branch instruction takes: " <<  duration2.count() << "\n";

    globalStats.close();

    // output new .bc with our metadata
    if (!outputNewModule(M, module_name)) {
        std::cout << "Output new module fails\n";

        return false;
    }

    return true;
}


void ida::InterproceduralDependencyAnalysis::setInstMetaData() {
    std::map<StoreInst *, unsigned> store_id;

    // set id for each BranchInst
    unsigned valid_branch = 0;
    for(auto bvDep: _BD2VDMap) {
        if (!bvDep->store_num) {
            continue;
        }
        
        valid_branch += 1;

        Instruction *BI = bvDep->branch_dep->inst;
        Function *F = BI->getParent()->getParent();
        LLVMContext& ctx = F->getContext();
    
        MDNode* B_N = MDNode::get(ctx, MDString::get(ctx, std::to_string(bvDep->id)));
        (*BI).setMetadata("bid", B_N);
    }

    // set id for each StoreInst
    unsigned store_num = 0;
    for(auto it: _SDMap) {
        Function *F = it.first;
        StoreDepSet SDSet = it.second;

        for (auto SD: SDSet) {
            StoreInst *SI = SD->inst;
            LLVMContext& ctx = F->getContext();

            store_num += 1;
            MDNode* S_N = MDNode::get(ctx, MDString::get(ctx, std::to_string(store_num)));
            (*SI).setMetadata("sid", S_N);
            store_id[SI] = store_num;
        }     
    }

    // set BI->SI relation
    std::set<unsigned> added_store;
    for(auto bvDep: _BD2VDMap) {
        if (!bvDep->store_num) {
            continue;
        }

        Instruction *BI = bvDep->branch_dep->inst;
        Function *F = BI->getParent()->getParent();
        LLVMContext& ctx = F->getContext();
        
        // Note that some variables do not contain stores
        unsigned var_num = bvDep->varDeps.size();

        // For each variable, save stores
        // In fact, there is only one branch variable due to the limitations of later analysis,
        // If there is more than one, it must be a local variable that is equal to the former one.
        for (auto varDep: bvDep->varDeps) {
            StoreDepSet SDSet = varDep->storeDeps;
            if (SDSet.empty()) {
                var_num -= 1;
                continue;
            }

            unsigned type;
            rootNodeType v_type = varDep->data_dep->node_type;
            if (v_type == rn_globalVariable) {
                type = 0x1;
            }
            else if (v_type == rn_localVariable) {
                type = 0x2;
            }
            else if (v_type == rn_structPointerParam) {
                type = 0x3;
            }
            else {
                assert(0 && "invalid variable type");
                continue;
            };

            unsigned vid = varDep->id;

            // format: v_[vid]_t = [type]
            std::string label_t = "v_" + std::to_string(vid) + "_t";
            std::string value_t = std::to_string(type);
            MDNode* VT_N = MDNode::get(ctx, MDString::get(ctx, value_t));
            (*BI).setMetadata(label_t, VT_N); 

            // format: v_[vid]_s_num = [store_num]
            unsigned store_num = SDSet.size();
            std::string label_s_n = "v_" + std::to_string(vid) + "_s_num";
            MDNode* VSN_N = MDNode::get(ctx, MDString::get(ctx, std::to_string(store_num)));
            (*BI).setMetadata(label_s_n, VSN_N);

            unsigned s_idx = 0;
            for (auto *SD: SDSet) {
                StoreInst *SI = SD->inst;

                // format: s_[vid]_[s_idx] = [sid]
                std::string label_id = "s_" + std::to_string(vid) + "_" + std::to_string(s_idx);
                std::string value_id = std::to_string(store_id[SI]);
                MDNode* VSID_N = MDNode::get(ctx, MDString::get(ctx, value_id));
                (*BI).setMetadata(label_id, VSID_N); 

                s_idx += 1;

                added_store.insert(store_id[SI]);       
            }
        }

        // format: v_num = [var_num]
        MDNode* V_N = MDNode::get(ctx, MDString::get(ctx, std::to_string(var_num)));
        (*BI).setMetadata("v_num", V_N);
    }

    globalStats << "[IDA] Find " << added_store.size() << " branch-related store instructions for " \
                << valid_branch << " branches\n";

    
    // test for setting metadata
    /*
    for(auto it: _BD2VDMap) {
        BranchDep *BD = it.first;
        Instruction *BI = BD->inst;
        Function *F = BI->getParent()->getParent();

        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        F->getAllMetadata(MDs);

        MDNode* N = (*BI).getMetadata("v_num");
        if (!N)
            continue;

        std::string value_s = cast<MDString>(N->getOperand(0))->getString().str();
        unsigned s_count = stoi(value_s);
    }*/
}


bool ida::InterproceduralDependencyAnalysis::isContainStructPointerList(
    StructPointers splist_1, StructPointers splist_2) {
    // make sure splist_1 contains splist_2
    if (splist_1.size() < splist_2.size())
        return false;

    // Data in splist are in reverse order. That is, the top struct pointer is at the end
    splist_1.reverse();
    splist_2.reverse();
    auto it1b = splist_1.begin();
    auto it2b = splist_2.begin();
    auto it1e = splist_1.end();
    auto it2e = splist_2.end();
    for (auto it1 = it1b, it2 = it2b; (it1 != it1e) && (it2 != it2e); ++it1, ++it2) {
        StructPointer *sp1 = *it1;
        StructPointer *sp2 = *it2; 
        if (*sp1 != *sp2) {
            return false;
        }
    }

    return true;
}


void ida::InterproceduralDependencyAnalysis::findStoresForBranch(BranchDep *BD, DataDep *data_dep, \
    ida::InterproceduralDependencyAnalysis::VD2SDDep *vsDep, llvm::DominatorTree* DT, llvm::LoopInfo* LI) {

    // local variable: intra-procedure type matching
    Function *BD_F = BD->func; 
    if (data_dep->node_type == rn_localVariable) {
        for (auto SD: _SDMap[BD_F]) {
            DataDep *_data_dep = SD->data_dep;
            SmallPtrSet<BasicBlock*, BB_THRESHOLD> ExclusionSet;

            // node type & local variable & reachability
            if ((_data_dep->node_type == rn_localVariable) && \
                (data_dep->local_var == _data_dep->local_var) && \
                (isPotentiallyReachable(SD->inst, BD->inst, &ExclusionSet, DT, LI))) {
                
                vsDep->storeDeps.insert(SD);
                // outputStoreDep(SD); 
            }
        }
    }

    // global variable: inter-procedure type matching
    else if (data_dep->node_type == rn_globalVariable) {
        for(auto it: _SDMap) {
            StoreDepSet SDSet = it.second;
            for (auto SD: SDSet) {
                DataDep *_data_dep = SD->data_dep;

                // node type & global variable 
                if ((_data_dep->node_type == rn_globalVariable) && \
                    (data_dep->global_var == _data_dep->global_var)) { 
                    
                    vsDep->storeDeps.insert(SD);
                    // outputStoreDep(SD); 
                }
            }
        }
    }   

    // struct pointer: inter-procedure type matching
    else if (data_dep->node_type == rn_structPointerParam) {
        // inter-procedure type matching
        for(auto it: _SDMap) {
            StoreDepSet SDSet = it.second;
            for (auto SD: SDSet) {
                DataDep *_data_dep = SD->data_dep;

                // node type & type chain (SD contains BD)
                if ((_data_dep->node_type == rn_structPointerParam) && \
                    isContainStructPointerList(_data_dep->sp_list, data_dep->sp_list)) {

                    vsDep->storeDeps.insert(SD);
                    // outputStoreDep(SD); 
                }
            }
        }
    }   

    else;
}


void ida::InterproceduralDependencyAnalysis::findStoresForBranchOnCG(BranchDep *BD, DataDep *data_dep, \
    ida::InterproceduralDependencyAnalysis::BD2VDDep *bvDep, llvm::DominatorTree* DT, llvm::LoopInfo* LI) {

    unsigned BD_var_idx = 0;

    std::list<CallInst *> to_trace_insts = {};
    std::unordered_set<Function *> traced_funcs = {};    
    std::unordered_map<CallInst *, unsigned> func_param_index;

    // create new branch variable
    VD2SDDep *vsDep = new VD2SDDep();
    vsDep->id = 0;
    // vsDep->data_dep = data_dep;

    unsigned param_index = data_dep->nsp_param;

    Function *BD_F = BD->func;
    traced_funcs.insert(BD_F);
    
    Node *NF = _CG->getNode(BD_F);
    for (auto edge: NF->in_edges) {
        CallInst *ci = edge->inst;
        Function *cf = edge->src;
        if (traced_funcs.find(cf) == traced_funcs.end()) {
            to_trace_insts.push_back(ci);
            traced_funcs.insert(cf);

            func_param_index[ci] = param_index;
        }
    }

    while (!to_trace_insts.empty()) {
        CallInst *cur_inst = to_trace_insts.back();
        to_trace_insts.pop_back();

        Function *cur_func = cur_inst->getParent()->getParent();
    
        // handle the param_index variable of callInst
        param_index = func_param_index[cur_inst];
        // outs() << "[BDA] handle " << cur_func->getName() << ": call callee(" << param_index << ")\n";

        // determine if this variable is not from function parameteres
        Value *v = cur_inst->getOperand(param_index);
        auto call_op = dyn_cast<Instruction>(v);
        if (!call_op) {
            continue;
        }

        DataDepSet data_dep_set = _BDA->extractDataDependency(call_op, DT, LI, true);
        if (data_dep_set.size() > 0) { 
            DataDep *data_dep = *data_dep_set.begin();
            // _BDA->showDataDependency(data_dep);

            if (data_dep->node_type == rn_nonPointerParam) {
                param_index = data_dep->nsp_param;

                Node *NF = _CG->getNode(cur_func);
                for (auto edge: NF->in_edges) {
                    CallInst *ci = edge->inst;
                    Function *cf = edge->src;
            
                    if (traced_funcs.find(cf) == traced_funcs.end()) {                        
                        to_trace_insts.push_back(ci);
                        traced_funcs.insert(cf);

                        func_param_index[ci] = param_index;

                        // outs() << "[BDA] add " << cf->getName() << " call " 
                        //         << cur_func->getName() << "(" << param_index << ")\n";
                    }
                }                   
            }
            else {
                // since one function can be called with different values for the same parameter
                traced_funcs.erase(cur_func);

                // find definitions in the local variables in other functions
                if (data_dep->node_type == rn_localVariable) {
                    for (auto SD: _SDMap[cur_func]) {
                        DataDep *_data_dep = SD->data_dep;
                        SmallPtrSet<BasicBlock*, BB_THRESHOLD> ExclusionSet;

                        // node type & local variable & reachability
                        if ((_data_dep->node_type == rn_localVariable) && \
                            (data_dep->local_var == _data_dep->local_var)) {

                            vsDep->storeDeps.insert(SD);
                            // outputStoreDep(SD); 
                        }
                    }
                }

                else {
                    findStoresForBranch(BD, data_dep, vsDep, DT, LI);  
                }

                // if there are more source variables, this dependency can be overrided.
                vsDep->data_dep = data_dep;
            }
        }
    }

    if (!vsDep->storeDeps.empty()) {
        bvDep->varDeps.insert(vsDep);
    }
}


void ida::InterproceduralDependencyAnalysis::findStoreDependencyFromSI( \
     Function *F, llvm::DominatorTree* DT, llvm::LoopInfo* LI) {

    for(auto SI: StoreInsts[F]) {
        // outs() << "[IDA] StoreInst: " << *SI << "\n";

        // get struct pointer dependency
        DataDepSet data_dep_set;

        Value *v = SI->getOperand(1);
        auto addr = dyn_cast<Instruction>(v);
        if (!addr) {
            
            // global variable
            DataDep *data_dep = new DataDep();
            data_dep->node_type = rn_globalVariable;
            data_dep->global_var = SI->getOperand(1);
            data_dep_set.insert(data_dep);
        }

        else if (isa<AllocaInst>(addr)) {

            // local variable
            DataDep *data_dep = new DataDep();
            data_dep->node_type = rn_localVariable;
            data_dep->local_var = dyn_cast<AllocaInst>(addr);
            data_dep_set.insert(data_dep);
        }

        else {
            data_dep_set = _BDA->extractDataDependency(addr, DT, LI, true);
        }

        // only consider one source for store instruction
        if (data_dep_set.size() > 0) {
            assert((data_dep_set.size() == 1) && "find more than one source for store instruction");     

            StoreDep *store_dep = new StoreDep();
            store_dep->inst = SI;
            store_dep->func = F;
            store_dep->file_path = getSourceFilePath(SI);
            store_dep->line_number = getSourceFileLineNumber(SI);
            store_dep->data_dep = *data_dep_set.begin();

            _SDMap[F].insert(store_dep);

            // outputStoreDep(store_dep);
        }   
    }
}


void ida::InterproceduralDependencyAnalysis::findStoreDependencyFromRI(Function *F) {
    // get ret_vars with type, perhaps not only one:
    std::list<AllocaInst *> ret_vars;

    Type *ret_type = F->getReturnType();
    for (auto inst_iter = inst_begin(F); inst_iter != inst_end(F); inst_iter++) {
        if (auto *AI = dyn_cast<AllocaInst>(&(*inst_iter))) {
            Type *t = AI->getAllocatedType();
            if (t == ret_type) {
                // outs() << "[BDA] find AllocaInst: " << *AI << "\n";
                ret_vars.push_back(AI);
            }
        }
    }
    
    // find StoreInsts that the address pointer equals one of return variables
    for (auto SD: _SDMap[F]) {
        DataDep *data_dep = SD->data_dep;

        // some local variables are related to ret value
        if (data_dep->node_type == rn_localVariable) {
            for (auto *AI: ret_vars) {
                if ((AI == data_dep->local_var) && !data_dep->sp_list.empty()) {
                    data_dep->node_type = rn_structPointerParam;
                    // outputStoreDep(SD);
                    break;
                }
            }
        }
        
    }
}


void ida::InterproceduralDependencyAnalysis::showStructPointersInfo(Module &M) {
    std::set<StructPointers> SPSet;
    std::map<llvm::Type *, std::set<StructPointers>> SPMap;
    
    for (Function &F: M) {
        for (auto it = _FBD[&F].begin(); it != _FBD[&F].end(); it++) {
            BranchDep *BD = *it;

            // determine whether each strcut pointer list is repeated
            for (auto data_dep: BD->data_dep_set) {
                if (data_dep->node_type == rn_structPointerParam) {
                    bool flag = false;
                    for (auto sp_list: SPSet) {
                        if (_BDA->isEqualStructPointerList(sp_list, data_dep->sp_list)) {
                            flag = true;
                            break;
                        }
                    }
                    if (!flag) {
                        StructPointer *top_sp = data_dep->sp_list.back();
                        Type *top_sp_t = top_sp->type;
                        SPMap[top_sp_t].insert(data_dep->sp_list);
                        SPSet.insert(data_dep->sp_list);
                    }
                }
            }
        }
    }

    globalStats << "[IDA] Find " << SPMap.size() << " unique condition-dependent struct types\n";
    globalStats << "[IDA] Find " << SPSet.size() << " unique condition-dependent struct variables\n";
}


void ida::InterproceduralDependencyAnalysis::outputStoreDep(StoreDep *SD) {
    if (SD->inst) {
        outs() << "[IDA] SD Inst: " << *SD->inst << " in " << SD->func->getName() << "\n";
    }
    else {
        outs() << "[IDA] SD Inst: " << "Argument " << SD->data_dep->nsp_param << " in " << SD->func->getName() << "\n";
    }
    outs() << "[IDA] SD File path: " << SD->file_path << ", line: " << SD->line_number << "\n";

    outs() << "[IDA] store address variables:\n";

    outs() << "[IDA] \t" << "Type: ";
    _BDA->showDataDependency(SD->data_dep);
    
}


void ida::InterproceduralDependencyAnalysis::outputBD2SDsMap() {
    unsigned branch_num = 0;
    std::set<StoreInst *> storeSet;

    for(auto bvDep: _BD2VDMap) {
        if (!bvDep->store_num) {
            continue;
        }

        BranchDep *BD = bvDep->branch_dep;

        outs() << "[IDA] -----------------------------------------------------------------------------\n";
        outs() << "[IDA] BD Inst: " << *BD->inst << " in " << BD->func->getName() << "\n";
        outs() << "[IDA] BD File path: " << BD->file_path << ", line: " << BD->line_number << "\n";

        outs() << "[IDA] branch condition variables:\n";

        // output each variable
        for (auto varDep: bvDep->varDeps) {
            StoreDepSet SDSet = varDep->storeDeps;
            if (SDSet.empty()) {
                continue;
            }
            
            outs() << "[IDA] \t" << varDep->id << ".Type: ";
            _BDA->showDataDependency(varDep->data_dep);

            outs() << "[IDA] branch condition variable related store instructions:\n";

            unsigned store_idx = 0;
            for (auto SD: SDSet) {
                outs() << "[IDA] store " << ++store_idx << ":\n";
                outputStoreDep(SD);
                storeSet.insert(SD->inst);
            }      
        }
    }

    globalStats << "[IDA] Total Branches: " << _BD2VDMap.size() << \
                    ", total Stores: " << storeSet.size() << "\n";
}


