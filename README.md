# Multi-Core Distributed Bellman-Ford Shortest Path #

This code is from the following conference paper:

Timothy Middelkoop, "The Impact of Multi-Core Computing on
Computational Optimization," In Proceedings of the 2012 Industrial and
Systems Engineering Research Conference, Orlando, FL, May 2012.

The following dataset was used: 
"9th DIMACS Implementation Challenge - Shortest Paths"


## Code ##
The source contains the following work:

* A serial distributed Bellman-Ford algorithm implementation.
* A multi-core distributed Bellman-Ford shortest path algorithm implemented
  using Threading Building Blocks by Intel (TBB)
* A multi-core context aware profiling probe library
* A micro-benchmark suit for multi-core and multi-level cache computation.
* A C++ OO wrapper for SQLITE
* SQL and R based analysis code for the work done in the paper.
* A small experiment management suit for running multiple test cases
* Code to pre-process and filter the problem set into binary data files.

The source requires the support of some contributed libraries found in
the parent directory. Major dependencies include PAPI and TBB (Thanks!).

git clone https://github.com/mtim/Contrib ../contrib


## Building ##

* build libraries in contrib
* config.h should be a symlink to a config in config/
* run Make

