Branch Dependency Analysis
=============================================================================================================

This is a llvm pass to analyse the data dependnecy of the variables in branching conditions.
For more information, please reter to Section 3.1 in our paper.

## Installation
in /IDA folder:
```
mkdir build && cd build
cmake ..
make
```

In /build, file libidapass.so is built.

## Usage
We can use clang "opt" command to use this pass
:
```
opt -load libidapass.so -ida <program.bc>
```