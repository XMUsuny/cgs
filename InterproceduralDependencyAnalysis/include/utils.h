#include <fstream>

#include "LLVMEssentials.h"


std::string getSourceFilePath(llvm::Instruction *I);
unsigned getSourceFileLineNumber(llvm::Instruction *I);

std::string getModuleName(llvm::Module &M);
bool initGlobalStats(std::ofstream &globalStats, std::string name, bool app); 

bool outputNewModule(llvm::Module &M, std::string name);