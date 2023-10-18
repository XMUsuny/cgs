#include <list>
#include <unordered_set>


#include "CallGraph.h"

#define INDIRECT_CALL	0


using namespace llvm;


void ida::CallGraph::getAnalysisUsage(AnalysisUsage & AU) const {
    AU.setPreservesAll();
}


char ida::CallGraph::ID = 0;
static RegisterPass<ida::CallGraph> X("CallGraph", "Call Graph Pass", false, true);


bool ida::CallGraph::runOnModule(Module &M) {

	// connect nodes
  	for (auto &F : M) {
	    if (F.isDeclaration() || F.empty() || F.isIntrinsic())
	     	continue;
	    
	    Node *caller = getNode(&F);
	    for (auto it = inst_begin(F); it != inst_end(F); ++it) {
	    	Instruction *I = &(*it);
	      	if (auto CI = dyn_cast<CallInst>(I)) {
	      		Function *f = CI->getCalledFunction();
	        	if (f && f->isIntrinsic())
	        		continue;

	        	Node *callee = getNode(f);

	        	// direct calls
	        	if (callee) {
	        		addInEdge(callee, caller, CI, DIRECT);
	        		addOutEdge(caller, callee, CI, DIRECT);
		        }
		        else {
		        	if(INDIRECT_CALL) {
		        		
			         	// indirect calls
			         	auto ind_call_candidates = getIndirectCallCandidates(M, CI);
			         	for (auto ind_call: ind_call_candidates) {
			            	addInEdge(ind_call, caller, CI, INDIRECT);
			            	addOutEdge(caller, ind_call, CI, INDIRECT);
			        	}
			        }
		        }
		    }
		}
	}

	return true;
}


ida::Node *ida::CallGraph::getNode(Function *F) {
	if (!F) {
		return nullptr;
	}

	Node *N = nullptr;
	for (auto node: _CG) {
		if (node->func == F) {
			N = node;
			break;
		}
	}

	if (!N) {
		N = new Node();
		N->func = F;
		if (F->isDeclaration() || F->empty()) {
			N->type = EXTERNAL;
		}
		else {
			N->type = INTERNAL;
		}
	}

	_CG.insert(N);

	return N;
}


void ida::CallGraph::addInEdge(Node *cur, Node *other, CallInst *inst, EdgeType type) {
	Edge *in_edge = new Edge();
	in_edge->src = other->func;
	in_edge->dst = cur->func;
	in_edge->type = type; 
	in_edge->inst = inst;
	cur->in_edges.insert(in_edge);
}


void ida::CallGraph::addOutEdge(Node *cur, Node *other, CallInst *inst, EdgeType type) {
	Edge *out_edge = new Edge();
	out_edge->src = cur->func;
	out_edge->dst = other->func;
	out_edge->type = type; 
	out_edge->inst = inst;
	cur->out_edges.insert(out_edge);
}

void ida::CallGraph::printNode(Node *N) {
	outs() << "Function Name: " << N->func->getName() << "\n";
	outs() << "As caller: \n";
	for (auto e: N->out_edges) {
		outs() << "   " << e->src->getName() << " --> " << e->dst->getName() << "\n";
	}
	outs() << "As callee: \n";
	for (auto e: N->in_edges) {
		outs() << "   " << e->src->getName() << " --> " << e->dst->getName() << "\n";
	}
}

bool ida::CallGraph::isTypeEqual(Type *t1, Type *t2) {
	if (t1 == t2)
    	return true;
  
	// need to compare name for sturct, due to llvm-link duplicate struct types
	if (!t1->isPointerTy() || !t2->isPointerTy())
		return false;

	Type *t1_pointed_ty = t1->getPointerElementType();
	Type *t2_pointed_ty = t2->getPointerElementType();

	if (!t1_pointed_ty->isStructTy() || !t2_pointed_ty->isStructTy())
		return false;
  
	std::string t1_name = t1_pointed_ty->getStructName().str();
	std::string t2_name = t2_pointed_ty->getStructName().str();

  	return (t1_name == t2_name);
}


bool ida::CallGraph::isFuncSignatureMatch(CallInst *CI, llvm::Function *F) {
	if (F->isVarArg())
		return false;

	unsigned true_arg_num = CI->getNumArgOperands();
	unsigned arg_num = F->arg_size();
	if (arg_num != true_arg_num)
		return false;

 	// compare return type
 	Type *true_ret_type = CI->getType();
	Type *ret_type = F->getReturnType();
	if (!isTypeEqual(ret_type, true_ret_type))
		return false;
  	
  	// compare argument type
	for (unsigned i = 0; i < true_arg_num; i++) {
    	Value *true_arg = CI->getOperand(i);
    	Value *arg = F->getArg(i);
    	if (!isTypeEqual(true_arg->getType(), arg->getType()))
      	return false;
  	}

	return true;
}

ida::NodeSet ida::CallGraph::getIndirectCallCandidates(Module &M, CallInst *CI)
{
	Type *callee_ty = CI->getFunctionType();
  	assert(callee_ty != nullptr && "can not find indirect call for null function type!\n");
  
  	NodeSet ind_call_cands;
  	for (auto &F : M) {
	    if (F.isDeclaration() || F.empty())
	      continue;

	    if (isFuncSignatureMatch(CI, &F)) {
	    	Node *N = getNode(&F);
	      	ind_call_cands.insert(N);
	    }
  	}
	
	return ind_call_cands;
}

bool ida::CallGraph::isReachable(Node *src, Node *dst, bool indirect) {
	std::list<Node *> to_trace_nodes;
	std::unordered_set<Node *> traced_nodes;

    to_trace_nodes.push_back(src);
    while (!to_trace_nodes.empty())
    {
		Node *N = to_trace_nodes.front();
		to_trace_nodes.erase(to_trace_nodes.begin());
		traced_nodes.insert(N);

		if (N == dst)
			return true;

  		for (auto out_edge: N->out_edges) {
			if (!indirect && (out_edge->type == INDIRECT)) {
				continue;
			}

			Node *_N = getNode(out_edge->dst);
			if ((_N->type == INTERNAL) && (traced_nodes.find(_N) == traced_nodes.end())) {
				to_trace_nodes.push_back(_N);	
			}
		}
    }

    return false;
}
