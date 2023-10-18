Concrete Constraint Guided Symbolic Execution
=============================================================================================================

This xxxx

## Installation

IDA: [`Build`](InterproceduralDependencyAnalysis/README.md).

KLEE: [Building KLEE](https://klee.github.io/build-llvm13/)



## Usage

First, set environment variables:

```
export SANDBOX_DIR=/tmp
export SOURCE_DIR=/path-to/cgs
export OUTPUT_DIR=/path-to/results
```

Then, the following README files explains how to use our tool:
* Build the benchmarks used in the paper: [`benchmark/README.md`](benchmark/README.md).
* Generate new llvm bitcode : [`InterproceduralDependencyAnalysis/README.md`](InterproceduralDependencyAnalysis/README.md).
* Running KLEE:
 ```
xxx:
1) python3 run.py [program] gen
xxx:
2) python3 run.py [program] run [searcher]
xxx:
3) python3 run.py [program] relay_ub [searcher]
```


## Citing Learch
```
@inproceedings{XXX,
  author = {Yue Sun, Guowei Yang, Shichao Lv, Zhi Li, Limin Sun},
  title = {Concrete Constraint Guided Symbolic Execution},
  year = {2024},
  address = {Lisbon, Portugal},
  url = {https://doi.org/10.1145/3460120.3484813},
  numpages = {12},
  keywords = {symbolic execution, data dependency analysis},
  series = {ICSE '24}
}
```
