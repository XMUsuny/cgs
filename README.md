Concrete Constraint Guided Symbolic Execution
=============================================================================================================

CGS (concrete-constraint guided searcher) is a dapendency-based path prioritization method for symbolic execution. It is motivated by an important observation that concrete branching conditions encompass a significant majority and a large portion of them are only partially covered. Therefore, there is a great potential to improve overall code coverage by guiding symbolic execution towards covering more concrete branches.

The folder [IDA](IDA/README.md) and [KLEE](klee) contains the codes to impliment our methods in Section 3 in our [paper](https://) in ICSE '24.

The artifacts of motivation and experimentation can be found [here](https://zenodo.org/records/10020236).

## Installation

Install LLVM 11.1.0 first and build: 

1.Branch dependency analysis: [`Installation`](IDA/README.md)

2.KLEE (using STP solver): [`Building KLEE`](https://klee.github.io/build-llvm13/)



## Usage

First, set environment variables:

```
export SANDBOX_DIR=/tmp
export SOURCE_DIR=/path-to/cgs
export OUTPUT_DIR=/path-to/results
```

Then, build program in [benchmark](/benchmark) folder.


Next, generate new llvm bitcode:
```
python3 run.py [program] gen
```

Last, use our KLEE to test:
```
python3 run.py [program] run [searcher]
```


## Cite our paper
```
@inproceedings{XXX,
  author = {Yue Sun, Guowei Yang, Shichao Lv, Zhi Li, Limin Sun},
  title = {Concrete Constraint Guided Symbolic Execution},
  year = {2024},
  address = {Lisbon, Portugal},
  url = {https://doi.org/xxx},
  numpages = {12},
  series = {ICSE '24}
}
```
