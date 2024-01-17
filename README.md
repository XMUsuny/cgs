Concrete Constraint Guided Symbolic Execution
=============================================================================================================

CGS (concrete-constraint guided searcher) is a dapendency-based path prioritization method for classical symbolic execution. It is motivated by an important observation that concrete branching conditions encompass a significant majority and a large portion of them are only partially covered. Therefore, there is a great potential to improve overall code coverage by guiding symbolic execution towards covering more concrete branches.

The folder [IDA](IDA/README.md) and [KLEE](klee) contains the codes to implement our methods in Section 3 in our [paper](pre-print.pdf) in ICSE '24.

The artifacts of motivation and experiment be found on [zenodo](https://zenodo.org/records/10516325).

## Installation

Install LLVM 11.1.0 first and build: 

1.Branch dependency analysis: [`Installation`](IDA/README.md)

2.KLEE (using STP solver): [`Building KLEE`](https://klee.github.io/build-llvm13/)


## Usage

First, set the following environment variables:

```
export SANDBOX_DIR=/tmp
export SOURCE_DIR=/path-to/cgs
export OUTPUT_DIR=/path-to/results
```

Then, build program in [benchmark](/benchmark) folder.


Next, generate new llvm bitcode (`new_benchmark` and `stat` folders are created):
```
python3 run.py [program] gen
```

Last, use our klee to test:
```
python3 run.py [program] run [searcher]
```

Moreover, we provide an option `COV_STATS` in `run.py`. If it is set to `True` before running klee, we can collect the statistics about symbolic and concrete branching conditions and use our modified `klee-stats` to show the results:

```
------------------------------------------------------------------------------------------------------------
|      Path       |  Instrs|  Time(s)|  ICov(%)|  BCov(%)|  ICount|  PCB|  FCB|  TC|  SC|  CC|  FCSC|  FCCC|
------------------------------------------------------------------------------------------------------------
|random-path/grep/|31573693|  3792.19|    27.68|    17.60|   93708|    0|    0|1404| 120|1372|    85|   385|
------------------------------------------------------------------------------------------------------------

PCB=Partially Covered Branches
FCB=Fully Covered Branches
TC=Total (Branching) Conditions
SC=Symbolic (Branching) Conditions
CC=Concrete (Branching) Conditions
FCSC=Fully Covered Symbolic (Branching) Conditions
FCCC=Fully Covered Concrete (Branching) Conditions
```

PCB and FCB are non-zero only if we set `COV_STATS` to `False`.
 

## Cite our paper
```
@inproceedings{XXX,
  author = {Yue Sun, Guowei Yang, Shichao Lv, Zhi Li, Limin Sun},
  title = {Concrete Constraint Guided Symbolic Execution},
  year = {2024},
  address = {Lisbon, Portugal},
  url = {https://doi.org/10.1145/3597503.3639078},
  numpages = {13},
  series = {ICSE '24}
}
```
