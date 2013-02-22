#!/usr/bin/python

## Bellman-Ford Multi-Core test platform
## Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
## Licensed under the GPL 2.0 or later.

## Parameter test

import sys
import os
import string

from pysqlite2 import dbapi2 as sqlite3

job="""#!/bin/zsh
##PBS -q testq
#PBS -N multi-core
#PBS -M t.middelkoop@ufl.edu
#PBS -l walltime=00:03:00
#PBS -l pmem=256mb
#PBS -l nodes=1:ppn=8

cd ufhpc/multi-core/src

"""


if __name__ == "__main__":

    con=sqlite3.connect("experiments.sqlite")
    cur=con.cursor()
    cur.execute("SELECT eid FROM Experiments")
    
    for (eid,) in cur:
        qsub=os.popen("qsub","w")
        #qsub=os.popen("cat","w")
        qsub.write(job)
        qsub.write("cp experiments.sqlite ../data/experiments.sqlite-$PBS_JOBID\n" )
        qsub.write("time ./bf-dist-tbb-task -d ../data/experiments.sqlite-$PBS_JOBID -e %d \n" % (eid))
        qsub.close()
