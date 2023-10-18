#include <set>
#include <map>
#include <list>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "utils.h"
#include "CallGraph.h"
#include "BranchDependencyAnalysis.h"


namespace ida {
    class InterproceduralDependencyAnalysis : public llvm::ModulePass {
        public:
            static char ID;

            InterproceduralDependencyAnalysis(): llvm::ModulePass(ID) {}

            virtual bool runOnModule(llvm::Module &M);
            virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;

            // note that this module supports multiple source variables, but in current cgs implimentation
            // in KLEE, we only tackle the branches with one source variable

            // source variable information for one store
            typedef struct store_dependency {
                llvm::StoreInst *inst;
                llvm::Function *func;
                std::string file_path;
                unsigned line_number;
                DataDep *data_dep;   
            }StoreDep;

            typedef std::unordered_set<StoreDep *> StoreDepSet; 

            // single source variable in one branch-> multiple stores
            typedef struct branch_variable_dependency {
                unsigned id;    
   
                DataDep *data_dep;
                StoreDepSet storeDeps;  
            }VD2SDDep;

            typedef std::unordered_set<VD2SDDep *> VariableDepSet; 

            // single branch -> multiple source variables
            typedef struct branch_store_dependency {
                BranchDep *branch_dep;
                unsigned id;    
                unsigned store_num; 

                VariableDepSet varDeps;
            }BD2VDDep;

            typedef std::unordered_set<llvm::StoreInst *> StoreInstSet;          
            typedef std::unordered_set<llvm::ReturnInst *> RetInstSet;   

            std::unordered_map<llvm::Function *, StoreInstSet> StoreInsts;
            std::unordered_set<llvm::Function *> RetFuncs;
            
            void findStoresForBranch(BranchDep *BD, DataDep *data_dep, VD2SDDep *vsDep, \
                                    llvm::DominatorTree* DT, llvm::LoopInfo* LI);    
            void findStoresForBranchOnCG(BranchDep *BD, DataDep *data_dep, BD2VDDep *bvDep, \
                                    llvm::DominatorTree* DT, llvm::LoopInfo* LI); 
            
            void findStoreDependencyFromSI(llvm::Function *F, llvm::DominatorTree* DT, llvm::LoopInfo* LI);
            void findStoreDependencyFromRI(llvm::Function *F);

        private:     

            void showStructPointersInfo(llvm::Module &M);
            bool isContainStructPointerList(StructPointers splist_1, StructPointers splist_2);

            void setInstMetaData();  

            void outputBD2SDsMap();
            void outputStoreDep(StoreDep *SD);
            
            std::unordered_map<llvm::Function *, StoreDepSet> _SDMap;
            std::unordered_set<BD2VDDep *> _BD2VDMap;

            CallGraph *_CG;
            BranchDependencyAnalysis *_BDA;
            std::unordered_map<llvm::Function *, BranchDepSet> _FBD;  
    };
} // namespace ida
