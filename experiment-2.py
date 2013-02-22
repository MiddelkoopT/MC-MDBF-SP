#!/usr/bin/python

## Bellman-Ford Multi-Core test platform
## Copyright (c) 2008,2010 Timothy Middelkoop. All rights reserved.
## Licensed under the GPL 2.0 or later.

## Serial test

import sys
import os
import string

from pysqlite2 import dbapi2 as sqlite3

if __name__=='__main__':
    experiment=3
    database="results/experiment-%s.sqlite" % experiment
    print "Experiment %s\n" % database
    
    config="""
static const bool INFO=true;
static const bool DEBUG=false;
static const bool TRACE=false;
static const bool PROBE=false;
static const bool CHECK=false;
"""
    if(experiment==2):
        config+="""
static const bool STAT=false;
static const bool STAT2=false;
"""
    if(experiment==3):
        config+="""
static const bool STAT=true;
static const bool STAT2=false;
"""
    print "Make"
    open("config-experiment.h",'w').write(config)
    os.system("make clean")
    if(os.system("make EXPERIMENT=1 -j bin/bf-dist-tbb-task")>0):
        print "Experiment make failed"
        exit(-1)
    
    if os.path.exists(database):
        os.unlink(database)
    os.system("sqlite3 %s < database.sql" % database)
    
    print "Experiment Generation\n"
    con=sqlite3.connect(database)
    cur=con.cursor()
    experiment_sql="INSERT INTO Experiments (problem,threads,chunk) VALUES (?,?,?)"
    option_sql="INSERT INTO Options (eid,option,value) VALUES (?,?,?)"

    size={'NY':264346,'COL':435666,'FLA':1070376,'LKS':2758119,'W':6262104}

    if(experiment==2):
        ps=('NY','COL','FLA','LKS','W')
        cs=(100,500,1000,2500,5000,7500,10000,25000,50000,100000,250000,500000)
        ts=(2,4,8)
        serial=True
        single=True
        runs=2
    
    if(experiment==3):
        ps=('NY',)
        cs=(1000,5000)
        ts=(8,)
        serial=False
        single=False
        runs=1
   
    for p in ps:
        pf="USA-road-d.%s.gr.bin" % p
        if(serial):
            cur.execute(experiment_sql, (pf,1,-1))
            serial=cur.lastrowid
        for c in cs:
            if(single):
                cur.execute(experiment_sql, (pf,1,c))
                single=cur.lastrowid
                cur.execute(option_sql,(single,'serial',serial))
            for t in ts:
                if (c/2*t)<size[p]:
                    cur.execute(experiment_sql, (pf,t,c))
                    eid=cur.lastrowid
                    cur.execute(option_sql,(eid,'serial',serial))
                    cur.execute(option_sql,(eid,'single',single))

    cur.close()
    con.commit()
    con.close()

    print "Experiment Run"
    con=sqlite3.connect(database)
    cur=con.cursor()
    cur.execute("SELECT eid FROM Experiments")

    run=list()
    for row in cur:
        run.append(row)

    cur.close()
    con.close()

    ## Run experiments (database closed)
    for eid, in run:
        for r in range(0,runs):
            os.system("time bin/bf-dist-tbb-task -d %s -e %d" % (database,eid))
        
    print "Experiment Done"
