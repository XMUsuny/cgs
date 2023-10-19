Benchmark Preparation
=============================================================================================================

This is the folder for the evaluations of our work.

## File
config.txt: necessary configurations for our implimentations, each line is for one program

prepare.sh: download and build steps for each program

xmlparse.c: a fuzz harness downloaded from official website for testing libxml2 library

# New program

There are three steps to add new a program:

1. download the source code.

2. add configurations to [config.txt](/benchmark/config.txt).

3. build the program accoring to [prepare.sh](prepare.sh).
