#include <iostream>

#include "utils.h"


using namespace llvm;


bool outputNewModule(Module &M, std::string name) {

    // output new .bc file
    std::string source_dir = getenv("SOURCE_DIR");
    std::string file_path = source_dir + "/new_benchmark/" + name + ".bc";
    std::error_code ec;

    auto file = std::make_unique<llvm::raw_fd_ostream>(file_path.c_str(), ec, llvm::sys::fs::OF_None);
    WriteBitcodeToFile(M, *file);

    return true;
}


std::string getModuleName(Module &M) {
    std::string module_name;

    std::string module_path = M.getModuleIdentifier();
    int ind = module_path.find_last_of("/");

    // remove ".bc"
    if (ind > 0)
        module_name = module_path.substr(ind + 1, module_path.size() - ind - 4);
    else
        module_name = module_path.substr(0, module_path.size() - ind - 4);

    return module_name;
}


bool initGlobalStats(std::ofstream &globalStats, std::string name, bool app) {
    
    // create file for global statistics
    std::string source_dir = getenv("SOURCE_DIR");
    std::string filename = source_dir + "/stat/" + name + ".txt";
    if (app)
        globalStats.open(filename, std::ios_base::app);
    else
        globalStats.open(filename);

    if (!globalStats.is_open()) {
        std::cout << "failed to open file " << filename << std::endl;
        return false;
    }
    return true;
}

std::string getSourceFilePath(Instruction *I) {
    std::string filePath = "";

    const llvm::DebugLoc &debugInfo = I->getDebugLoc();
    if (debugInfo)
        filePath = debugInfo->getFilename().str();
    
    return filePath;
}


unsigned getSourceFileLineNumber(Instruction *I) {
    unsigned lineNumber = -1;

    const llvm::DebugLoc &debugInfo = I->getDebugLoc();
    if (debugInfo)
        lineNumber = debugInfo->getLine();

    return lineNumber;
}

