#include <set>
#include <string>

#include "LLVMEssentials.h"


namespace ida {
	enum NodeType { INTERNAL, EXTERNAL };
	enum EdgeType { DIRECT, INDIRECT };

	typedef struct edge {
		EdgeType type;
		llvm::CallInst *inst;
		llvm::Function *src;
		llvm::Function *dst;
	}Edge;

	typedef std::set<Edge *> EdgeSet;

	typedef struct node {
		NodeType type;
		llvm::Function *func;
		EdgeSet in_edges;
		EdgeSet out_edges;
	}Node;

	typedef std::set<Node *> NodeSet;

	class CallGraph : public llvm::ModulePass {
		public:
			static char ID;

			CallGraph() : llvm::ModulePass(ID){}

			virtual bool runOnModule(llvm::Module &M);
			virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

			// work
			Node *getNode(llvm::Function *F);
			bool isReachable(Node *src, Node *dst, bool indirect);

			void printNode(Node *N);
			
		private:
			void addInEdge(Node *cur, Node *other, llvm::CallInst *inst, EdgeType type);
			void addOutEdge(Node *cur, Node *other, llvm::CallInst *inst, EdgeType type);
			bool isTypeEqual(llvm::Type *t1, llvm::Type *t2);
			bool isFuncSignatureMatch(llvm::CallInst *CI, llvm::Function *F);
			NodeSet getIndirectCallCandidates(llvm::Module &M, llvm::CallInst *CI);

			NodeSet _CG;
	};
}