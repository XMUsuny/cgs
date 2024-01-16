Benchmark Preparation
=============================================================================================================

This is the folder to download and build the benchmarks.

## File
config.txt: necessary configurations for our implimentations, each line is for one program.

prepare.sh: download and build steps for each program.

xmlparse.c: a fuzz harness downloaded from official website for testing libxml2 library.

## New program

There are three steps to add new a program:

1. download the source code.

2. add program configurations to [config.txt](/benchmark/config.txt).

3. build the program following the buiding options in [prepare.sh](prepare.sh).
