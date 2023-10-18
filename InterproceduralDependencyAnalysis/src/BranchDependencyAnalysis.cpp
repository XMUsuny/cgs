#include <chrono>
#include <fstream>
#include <iostream>

#include "BranchDependencyAnalysis.h"


using namespace llvm;


bool output_branch_dependency = 0;


void ida::BranchDependencyAnalysis::getAnalysisUsage(AnalysisUsage & AU) const {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.setPreservesAll();
}


std::ofstream globalStats1;



char ida::BranchDependencyAnalysis::ID = 0;
static RegisterPass<ida::BranchDependencyAnalysis> BDA("bda", 
    "Branch Dependency Analysis pass", false, true);


bool ida::BranchDependencyAnalysis::runOnModule(Module &M) {

    // get module name and configure global statistics file
    std::string module_name = getModuleName(M);
    if (!initGlobalStats(globalStats1, module_name, false))
        return false;

    // init start time
    auto start = std::chrono::high_resolution_clock::now();

    // find all branch dependency
    unsigned branch_num = 0, total_branch_num = 0;
    unsigned switch_num = 0;
    for (Function &F: M) {
        if (F.isDeclaration() || F.empty())
            continue;

        // outs() << "[BDA] begin to analyse " << F.getName() << "\n";

        LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
        DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();

        for(auto &BB: F) {
            for (auto &I: BB) {
                bool flag = false;
                Instruction *cond = nullptr;

                // conditional br instruction
                auto *BI = dyn_cast<BranchInst>(&I);
                if (BI && BI->isConditional()) {
                    total_branch_num += 1;

                    cond = dyn_cast<Instruction>(BI->getCondition());
                    if (cond && isa<ICmpInst>(cond)) {
                        Value *var1 = cond->getOperand(0);
                        Value *var2 = cond->getOperand(1);

                        // only consider non-pointer value
                        if (var1->getType()->isIntegerTy() && \
                            var2->getType()->isIntegerTy()) { 
 
                            // only reserve comparison of one constant and ont variable
                            auto CI1 = dyn_cast<ConstantInt>(var1);
                            auto CI2 = dyn_cast<ConstantInt>(var2);
                            if ((CI1 && !CI2) || (!CI1 && CI2)) {
                                flag = true;
                                branch_num += 1;
                            }
                        }
                    }
                }

                // switch instruction
                auto *SWI = dyn_cast<SwitchInst>(&I);
                if (SWI) {
                    cond = dyn_cast<Instruction>(SWI->getCondition());

                    flag = true;
                    switch_num += 1;
                }

                if (flag && cond) {
                    if (BI) {
                        outs() << "[BDA] Analyse BranchInst:" << *BI << "\n";
                    }
                    else if (SWI) {
                        outs() << "[BDA] Analyse SwitchInst:" << *SWI << "\n";
                    }
                    else;
                    
                    // backward data dependency analysis
                    DataDepSet data_dep_set = extractDataDependency(cond, &DT, &LI, false);

                    // sometimes there are more than one source variables, we use two rules to filter   

                    // rule 1:                
                    // only one non-local data dependency source is allowed to control the
                    // value change of branch variables, but local variables are allowed:
                    unsigned non_local = 0;
                    for (auto data_dep: data_dep_set) {
                        if ((data_dep->node_type == rn_globalVariable) || \
                            (data_dep->node_type == rn_structPointerParam) || \
                            (data_dep->node_type == rn_nonPointerParam)) {

                            non_local += 1;
                        }
                    }

                    if (non_local > 1) {
                        continue;
                    }

                    // rule 2:  
                    // remove data dependency that the source variables are not interger type
                    for (auto it = data_dep_set.begin(); it != data_dep_set.end();) {
                        DataDep *data_dep = *it;

                        // exclude non integer global variables
                        if (data_dep->node_type == rn_globalVariable) {
                            Type *pointer_type = data_dep->global_var->getType();
                            Type *type = pointer_type->getPointerElementType();

                            if (type->isIntegerTy()) {
                                it++;
                            }      
                            else {
                                data_dep_set.erase(it++);
                            }
                        }

                        // exclude struct(*) type local variables, but reserve array
                        else if (data_dep->node_type == rn_localVariable) {
                            Type *alloca_type = data_dep->local_var->getAllocatedType();

                            // strcut *xxx = ..
                            if (alloca_type->isIntegerTy()) {
                                it++;
                            }      
                            else {
                                data_dep_set.erase(it++);
                            }
                        }
 
                        // exclude non-integer type struct elements
                        else if (data_dep->node_type == rn_structPointerParam) {

                            // the back of struct pointer list is the top struct
                            StructPointer *sp = data_dep->sp_list.front();                  
                            Type *struct_pointer_type = sp->type->getPointerElementType();

                            auto struct_type = dyn_cast<StructType>(struct_pointer_type);
                            if (struct_type) {
                                Type *struct_element_type = struct_type->getElementType(sp->offset);
                                if (struct_element_type->isIntegerTy()) {
                                    it++;
                                }
                                else {
                                    data_dep_set.erase(it++);
                                }
                            }

                            // GEP can also get data from array
                            else {
                                auto array_type = dyn_cast<ArrayType>(struct_pointer_type);
                                if (array_type) {
                                    Type *array_element_type = array_type->getElementType();
                                    if (array_element_type->isIntegerTy()) {
                                        it++;
                                    }
                                    else {
                                        data_dep_set.erase(it++);
                                    }
                                }
                            }
                        }
                        else {
                            it++;
                        }
                    }

                    // add new branch dependency for current function
                    if (!data_dep_set.empty()) {
                        
                        BranchDep *branch_dep = new BranchDep();

                        branch_dep->inst = &I;
                        branch_dep->func = &F;
                        branch_dep->file_path = getSourceFilePath(&I);
                        branch_dep->line_number = getSourceFileLineNumber(&I);
                        branch_dep->data_dep_set = data_dep_set;

                        FuncBranchDeps[&F].insert(branch_dep);
                        
                        if (output_branch_dependency) {
                            outputBranchDep(branch_dep);
                        }
                    }
                }
            }
        }
    }

    // stats 
    unsigned global_var = 0, local_var = 0;
    unsigned non_pointer_param = 0, struct_pointer_param = 0, other_param = 0;
    for (Function &F: M) {
         for (auto branch_dep: FuncBranchDeps[&F]) {
            for (auto *data_dep: branch_dep->data_dep_set) {
                if (data_dep->node_type == rn_globalVariable) {
                    global_var += 1;
                }
                else if (data_dep->node_type == rn_localVariable) {
                    local_var += 1;
                }
                else if (data_dep->node_type == rn_nonPointerParam) {
                    non_pointer_param += 1;
                }
                else if (data_dep->node_type == rn_structPointerParam) {
                    struct_pointer_param += 1;
                }
                // else if (data_dep->node_type == rn_otherParam){
                //    other_param += 1;
                //}
                else;
            }
        }
    }

    globalStats1 << "[BDA] identify " << switch_num << " switches, " << branch_num << "/" 
                << total_branch_num << " target/total branches, global_var: " << global_var  
                << ", local_var: " << local_var << ", non_pointer_param: " << non_pointer_param 
                << ", struct_pointer_param: " << struct_pointer_param << "\n";
  
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    globalStats1 << "[BDA] extract branch dependency takes: " <<  duration.count() << "\n";

    globalStats1.close();

    return true;          
}


ida::DataDepSet ida::BranchDependencyAnalysis::extractDataDependency(
    Instruction *_inst, llvm::DominatorTree* DT, LoopInfo* LI, bool isStoreInst) { 
    
    // isStoreInst: for store instruction, we only save one data dependenty

    DataDepSet data_dep_set;
    DataDep *data_dep = new DataDep();
    data_dep->node_type = rn_null;

    // one pointer chain to save pointer deference in multi-layer struct
    StructPointers sp_list;
        
    // remove function argument related local variables (often in the entry block of function)
    AllocaInst *last_AI = nullptr;

    // a simple method to handle def-use in loops
    std::unordered_map<Instruction *, unsigned> instCount;
    unsigned maxInstCount = 10;

    // one trace means one def-use chain from root definition fo one of branch variables
    std::list<Instruction *> to_trace_insts = {};
    std::unordered_set<Instruction *> traced_insts = {};
    
    to_trace_insts.push_back(_inst);
    while(!to_trace_insts.empty()) {
        Instruction *cur_inst = to_trace_insts.back();     // DFS

        instCount[cur_inst] += 1;
        if (instCount[cur_inst] > maxInstCount) {
            break;
        }

        // return if there is a call/icmp/switch on the trace
        if (isa<CallInst>(cur_inst) || \
            isa<SwitchInst>(cur_inst) || \
            (isa<ICmpInst>(cur_inst) && (cur_inst !=_inst))) {
            break;
        }

        // control for multi paths to one source
        to_trace_insts.pop_back();
        traced_insts.insert(cur_inst);

        if (auto *GEP = dyn_cast<GetElementPtrInst>(cur_inst)) {
            if (GEP->getNumIndices() == 2) {

                // extract data from pointer
                unsigned offset;
                Value *v = GEP->getOperand(2);
                auto *CI = dyn_cast<ConstantInt>(v);
                if (CI) {
                    offset = CI->getZExtValue();
                    // offset = 8 * CI->getZExtValue();
                    // offset += handleStructBit(GEP); 
                }
                else
                    offset = -1;

                // new GEP pointer
                StructPointer *sp = new StructPointer();
                sp->type = GEP->getOperand(0)->getType();
                sp->offset = offset;

                sp_list.push_back(sp);
            }

            else {
                // "a[i] = k" do not mean branch condition "a[j] == k" is satisfied,
                // but we do not want to be so strict

                // arithmetic operations on pointer
                // add pointer to new instruction
                Value *pointer = cur_inst->getOperand(0);
                if (auto *I = dyn_cast<Instruction>(pointer)) {
                    to_trace_insts.push_back(I);
                }     

                continue;                
            }   
        }

        if (isa<StoreInst>(cur_inst)) {
            Value *v = cur_inst->getOperand(0);
            auto *I = dyn_cast<Instruction>(v);
            if (I && (traced_insts.find(I) == traced_insts.end())) {
                to_trace_insts.push_back(I);
            }
        }
        else {

            // add new insts based on def-use chains
            for (Use &U: cur_inst->operands()) {
                Value *v = dyn_cast<Value>(U);

                // 1. global varibales
                if (isa<GlobalVariable>(v)) {
                    data_dep->node_type = rn_globalVariable;
                    data_dep->global_var = v;
                    data_dep_set.insert(data_dep);
                    data_dep = new DataDep();

                    // reset
                    sp_list.clear();
                    traced_insts.clear();
                    break;
                }

                // 2. non-local variable
                else if (!isa<AllocaInst>(v)) {          
                    auto *I = dyn_cast<Instruction>(U);
                    if (I && (traced_insts.find(I) == traced_insts.end())) {
                        to_trace_insts.push_back(I);
                    }
                }

                // 3. local variable
                else {
                    AllocaInst *AI = dyn_cast<AllocaInst>(v);
                    // outs() << "[BDA] local: " << *AI << "\n";

                    // save local variable
                    bool flag = false;
                    for (auto _data_dep: data_dep_set) {
                        if ((_data_dep->node_type == rn_localVariable) && \
                            (_data_dep->local_var == AI)) {
                            flag = true;
                            break;
                        }
                    }

                    if (!flag) {
                        last_AI = AI;
                        data_dep->node_type = rn_localVariable;
                        data_dep->local_var = AI;
                        data_dep_set.insert(data_dep);
                       
                        data_dep = new DataDep();
                    }

                    // find which var defines this local variable
                    // 1. if prameter, end this trace
                    // 2, if others, find one definition based on reachability and 
                    //    distance of instruction and basic block, and continue

                    // save all stores to local variables in reverse order
                    std::vector<StoreInst *> store_insts;
                    
                    for (auto user : AI->users()) {
                        if (auto *SI = dyn_cast<StoreInst>(user)) {
                            Value *data = SI->getOperand(0);
                            
                            Argument *arg = dyn_cast<Argument>(data);

                            // local variables
                            if (!arg) {

                                // exclude store instruction if:
                                // 1. the written data is constant
                                // 2. the store instruction is before branch instruction
                                Value *data = SI->getOperand(0);
                                unsigned bb_to_index = getBBLabel(_inst->getParent());
                                unsigned bb_from_index = getBBLabel(SI->getParent());
                                if (!isa<ConstantInt>(data) && (bb_to_index >= bb_from_index)) {
                                    store_insts.push_back(SI);
                                }
                            }

                            // function arguments
                            else {
                                // remove function argument related local variable added earlier
                                if (last_AI) {
                                    for (auto _data_dep: data_dep_set) {
                                        if ((_data_dep->node_type == rn_localVariable) && \
                                            (_data_dep->local_var == last_AI)) {
                                            data_dep_set.erase(_data_dep);
                                            last_AI = nullptr;
                                            break;
                                        }
                                    }     
                                }

                                data_dep->local_var = AI;
                                Type *t = arg->getType();

                                // 1. non pointer type
                                if (!t->isPointerTy()) {
                                    // get argument index
                                    unsigned arg_index = 0xff;
                                    Function *F = _inst->getParent()->getParent();
                                    for (unsigned i = 0; i < F->arg_size(); i++) {
                                        if (arg == F->getArg(i)) {
                                            arg_index = i;
                                            break;
                                        }
                                    }

                                    if (arg_index != 0xff) {
                                        // check if has been added
                                        bool flag = false;
                                        for (auto _data_dep: data_dep_set) {
                                            if ((_data_dep->node_type == rn_nonPointerParam) && \
                                                (_data_dep->nsp_param == arg_index)) {
                                                flag = true;
                                                break;
                                            }
                                        }

                                        if (!flag) {
                                            // outs() << "[BDA] non pointer param: " << *arg << "\n";
                                            data_dep->nsp_param = arg_index;
                                            data_dep->node_type = rn_nonPointerParam;

                                            data_dep_set.insert(data_dep);  
                                        }
                                    }
                                }                                
                            
                                else {  
                                    // 2. struct pointer function param
                                    if (t->isPointerTy()
                                        && t->getPointerElementType()->isStructTy() \
                                        && !sp_list.empty()) {

                                        // check if has been added
                                        bool flag = false;
                                        for (auto data_dep: data_dep_set) {
                                            if ((data_dep->node_type == rn_structPointerParam) && \
                                                isEqualStructPointerList(data_dep->sp_list, sp_list)) {
                                                flag = true;
                                                break;
                                            }
                                        }
                                        if (!flag) {
                                            // outs() << "[BDA] struct-pointer param: " << *arg << "\n";
                                            data_dep->node_type = rn_structPointerParam;
                                            data_dep->sp_list = sp_list;

                                            data_dep_set.insert(data_dep);
                                        }                          
                                    }
                                    else {
                                        // 3. other function param
                                        // check if has been added
                                        /*
                                        bool flag = false;
                                        for (auto _data_dep: data_dep_set) {
                                            if ((_data_dep->node_type == rn_otherParam) && \
                                                (_data_dep->op_param == arg)) {
                                                flag = true;
                                                break;
                                            }
                                        }

                                        if (!flag) {
                                            // outs() << "[BDA] other pointer param: " << *arg << "\n";
                                            data_dep->op_param = arg;
                                            data_dep->node_type = rn_otherParam;

                                            data_dep_set.insert(data_dep);
                                        }*/
                                    }
                                }

                                // reset
                                sp_list.clear();
                                store_insts.clear();
                                traced_insts.clear();  
                                data_dep = new DataDep();
                                break;
                            }   // end of function argument
                        }
                    }
                    
                    // choose next instruction (for non parameter-releated local variables)
                    if (!store_insts.empty()) {  

                        // remove intermidiary local variables
                        if (last_AI) {
                            for (auto _data_dep: data_dep_set) {
                                if ((_data_dep->node_type == rn_localVariable) && \
                                    (_data_dep->local_var == last_AI)) {
                                    data_dep_set.erase(_data_dep);
                                    last_AI = nullptr;
                                    break;
                                }
                            }     
                        }

                        // 1) only one
                        if (store_insts.size() == 1) {
                            StoreInst *SI = store_insts.back();
                            SmallPtrSet<BasicBlock*, BB_THRESHOLD> ExclusionSet;

                            // may be this one can not reach current instrution
                            if ((traced_insts.find(SI) == traced_insts.end()) && \
                                isPotentiallyReachable(SI, cur_inst, &ExclusionSet, DT, LI)) {
                                to_trace_insts.push_back(SI);
                            }
                        }
                        
                        // 2) choose a StoreInst based on instruction and basic block reachability
                        else {
                            StoreInst *SI = selectStoreInst(store_insts, cur_inst, DT, LI);
                            if (SI && (traced_insts.find(SI) == traced_insts.end())) {
                                to_trace_insts.push_back(SI);
                            }
                        }
                        
                        store_insts.clear();
                    }           
                }
            }
        }

        if (isStoreInst && (data_dep_set.size() == 1)) {
            to_trace_insts.clear();
            break;
        }
    }

    return data_dep_set;
}


llvm::StoreInst *ida::BranchDependencyAnalysis::selectStoreInst(
    std::vector<StoreInst *> Froms, Instruction *To, llvm::DominatorTree* DT, LoopInfo* LI) {

    StoreInst *result;

    unsigned bb_threshold = Froms.size() < BB_THRESHOLD? Froms.size(): BB_THRESHOLD;
    SmallPtrSet<BasicBlock*, BB_THRESHOLD> ExclusionSet;

    // SIs in Froms are in reverse order
    unsigned idx = 0;
    for (auto it = Froms.rbegin(); it != Froms.rend(); it++) {
        StoreInst *From = *it;
        BasicBlock *BB = From->getParent();

        ExclusionSet.insert(From->getParent());
        idx++;
        if (idx >= bb_threshold)
            break;
    }

    std::vector<StoreInst *> reachStores;
    for (int i = 0; i < bb_threshold; i++) {
        StoreInst *From = Froms[i];
        BasicBlock *BB = From->getParent();

        // \\outs() << "[BDA] " << *From;
        ExclusionSet.erase(BB);

        // CFG::isPotentiallyReachable
        if(isPotentiallyReachable(From, To, &ExclusionSet, DT, LI)) {
            reachStores.push_back(From);
            // outs() << " [reach] ";
        }        
        // outs() << "\n";
        ExclusionSet.insert(BB);
    }

    // if more than one StoreInst can reach, select the nearest one (improve for out-loop and in-loop?)
    if (reachStores.size() == 0) {
        result = nullptr;
    }
    else if (reachStores.size() == 1) {
        result = reachStores.back();
    }
    else {
        unsigned min_bb_dist = 0xffff;
        for (auto From: reachStores) {
            unsigned from_bb = getBBLabel(From->getParent());
            unsigned to_bb = getBBLabel(To->getParent());
            unsigned dist = abs((int)from_bb - (int)to_bb);
            if (dist < min_bb_dist) {
                result = From;
                min_bb_dist = dist;
            }
        }
    }
    
    // outs() << "[BDA] use " << *result << "\n";
    return result;
}


bool ida::BranchDependencyAnalysis::isEqualStructPointerList(
    StructPointers splist_1, StructPointers splist_2) {
    if (splist_1.size() != splist_2.size())
        return false;

    for(auto sp1b = splist_1.begin(), sp1e = splist_1.end(), sp2b = splist_2.begin(), sp2e = splist_2.end(); \
        (sp1b != sp1e) && (sp2b != sp2e); ++sp1b, ++sp2b) {
        StructPointer *sp1 = *sp1b;
        StructPointer *sp2 = *sp2b;        
        if (*sp1 != *sp2)
            return false;
    }
    return true;
}

/*
unsigned ida::BranchDependencyAnalysis::handleStructBit(Instruction *inst) {
    unsigned offset = 0;
    bool _and = false, _lshr = false;       // execute add or lshr one time 
    std::list<Instruction *> to_trace_insts;
    std::list<std::string> allowed_insts = {"load", "and", "lshr", "bitcast"};

    to_trace_insts.push_back(inst);
    while(!to_trace_insts.empty()) {
        Instruction *I = to_trace_insts.back();
        to_trace_insts.pop_back();
        // outs() << *I << "\n";

        std::string i_str = I->getOpcodeName();
        if (i_str == "and" && !_and) {
            _and = true;
            Value *v = I->getOperand(1);
            auto *CI = dyn_cast<ConstantInt>(v);
            if (CI) {
                int c = CI->getSExtValue(); // int..
                if (c > 0) {
                    while((c & 1) == 0) {      // 2: 00000010
                        offset += 1;
                        c = c >> 1;
                    }
                }
                else {
                    while((c & 1) == 1) {      // -3: 11111101
                        offset += 1;
                        c = c >> 1;
                    }
                }
            }
        }
        if (i_str == "lshr" && !_lshr) {
            _lshr = true;
            Value *v = I->getOperand(1);
            auto *CI = dyn_cast<ConstantInt>(v);
            if (CI) {
                offset += CI->getZExtValue();
            }
        }

        for (auto *U :I->users()) {
            if (auto *next_inst = dyn_cast<Instruction>(U)) {
                std::string _i_str = next_inst->getOpcodeName();
                if (std::find(allowed_insts.begin(), allowed_insts.end(), _i_str) != allowed_insts.end()) {
                    to_trace_insts.push_back(next_inst);       
                }
            }
        }
    }

    return offset;
}*/


int ida::BranchDependencyAnalysis::getBBLabel(BasicBlock * BB) {
    std::string Str;
    raw_string_ostream OS(Str);
    std::string bb_name;

    BB->printAsOperand(OS, false);
    bb_name = OS.str();
    // outs() << "BasicBlock: "  << bb_name << "\n";
    
    bb_name.erase(bb_name.begin()); // remove '%'
    return atoi(bb_name.c_str());
}


void ida::BranchDependencyAnalysis::showStructPointer(StructPointer *struct_pointer) {
    outs() << "[BDA] \t   -Type: ";
    struct_pointer->type->print(outs());
    outs() << ", offset: " << struct_pointer->offset << "\n";
}


void ida::BranchDependencyAnalysis::outputBranchDep(BranchDep *BD) {
    outs() << "[BDA] -----------------------------------------------------------------------------\n";
    outs() << "[BDA] BD Inst: " << *BD->inst << " in " << BD->func->getName() << "\n";
    outs() << "[BDA] BD File path: " << BD->file_path << ", line: " << BD->line_number << "\n";

    outs() << "[BDA] branch condition variables:\n";

    unsigned num = 0;
    for (auto data_dep: BD->data_dep_set) {
        outs() << "[BDA] \t" << ++num << ".Type: ";
        showDataDependency(data_dep);
    }
}

void ida::BranchDependencyAnalysis::showDataDependency(DataDep *data_dep) {
    if (data_dep->node_type == rn_localVariable) {
        outs() << "local variable: ";
        outs() << *data_dep->local_var << "\n";
        for (auto sp: data_dep->sp_list)
            showStructPointer(sp);
    }
    else if (data_dep->node_type == rn_globalVariable) {
        outs() << "global variable: ";
        outs() << *data_dep->global_var << "\n";
    }
    else if (data_dep->node_type == rn_nonPointerParam) {
        outs() << "non pointer parameter: ";
        outs() << data_dep->nsp_param << "\n";
    }
    else if (data_dep->node_type == rn_structPointerParam) {
        outs() << "struct-pointer parameter\n";
        outs() << "[BDA] \tType Chain:\n";
        for (auto sp: data_dep->sp_list)
            showStructPointer(sp);
    }
    else if (data_dep->node_type == rn_otherParam){
        outs() << "other parameter: ";
        outs() << *data_dep->op_param << "\n";
    }
    else {
        outs() << "invalid data dependency\n";
    }
}

