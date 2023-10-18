#include <set>
#include <list>
#include <iterator>
#include <unordered_set>
#include <unordered_map>


#include "utils.h"


#define BB_THRESHOLD	32


namespace ida {
	enum rootNodeType {
	    rn_null, rn_localVariable, rn_globalVariable,
	    rn_nonPointerParam, rn_structPointerParam, rn_otherParam
	};

	typedef struct struct_pointer {
		llvm::Type *type;
		int offset;		// in bits

		bool operator==(struct_pointer& other) const {
			return ((type == other.type) && (offset == other.offset));
		}

		bool operator!=(struct_pointer& other) const {
			return ((type != other.type) || (offset != other.offset));
		}

	}StructPointer;

	typedef std::list<StructPointer *> StructPointers;

	typedef struct data_dependency {
		rootNodeType node_type;			

		llvm::AllocaInst *local_var;		// for parameter and local variable (intra procedure)
		llvm::Value *global_var;			// for global variable (no handler)
		StructPointers sp_list;				// for struct-pointer variable (type chain) (inter/intra procedure)
		llvm::Argument *op_param;			// for other pointer param (no handler)
		
		unsigned nsp_param;					// for non pointer parameter variable (argument index)
	}DataDep;

	typedef std::unordered_set<DataDep *> DataDepSet;

	typedef struct branch_dependency {
		llvm::Instruction *inst;
		llvm::Function *func;
		std::string file_path;
		unsigned line_number;
	    DataDepSet data_dep_set;
	}BranchDep;

	typedef std::unordered_set<BranchDep *> BranchDepSet;

	class BranchDependencyAnalysis: public llvm::ModulePass {
		public:
			static char ID;

		    BranchDependencyAnalysis(): llvm::ModulePass(ID) {};

		    virtual bool runOnModule(llvm::Module &M);
		    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

		    std::unordered_map<llvm::Function *, BranchDepSet> getBranchDependency() { return FuncBranchDeps; }
		    
		    DataDepSet extractDataDependency(llvm::Instruction *_inst, \
		    			llvm::DominatorTree* DT, llvm::LoopInfo* LI, bool isStoreInst);
			bool isEqualStructPointerList(StructPointers splist_1, StructPointers splist_2);

			void showStructPointer(StructPointer *struct_pointer);
            void outputBranchDep(BranchDep *BD);
            void showDataDependency(DataDep *data_dep);

		private:

			std::unordered_map<llvm::Function *, BranchDepSet> FuncBranchDeps;
			
			int getBBLabel(llvm::BasicBlock * BB);
			// unsigned handleStructBit(llvm::Instruction *inst);

			llvm::StoreInst *selectStoreInst(std::vector<llvm::StoreInst *> Froms, \
											llvm::Instruction *To, \
											llvm::DominatorTree* DT, llvm::LoopInfo* LI);
			
	};
} // namespace ida
