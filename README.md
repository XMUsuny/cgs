Concrete Constraint Guided Symbolic Execution
=============================================================================================================

CGS (concrete-constraint guided searcher) is a dapendency-based path prioritization method for symbolic execution. It is motivated by an important observation that concrete branching conditions encompass a significant majority and a large portion of them are only partially covered. Therefore, there is a great potential to improve overall code coverage by guiding symbolic execution towards covering more concrete branches.

The folder [/IDA](IDA/README.md) and [klee](klee) contains the codes to impliment our methods in Section 3 in our [paper](https://) in ICSE '24.

## Installation

IDA: [Installation](IDA/README.md)

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
