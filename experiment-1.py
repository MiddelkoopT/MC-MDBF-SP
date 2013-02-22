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
    database='results/experiment-1.sqlite'
    print "Experiment 1 %s\n" % database
    
    if os.path.exists(database):
        os.unlink(database)
    os.system("sqlite3 %s < database.sql" % database)
    
    print "Experiment Generation\n"
    con=sqlite3.connect(database)
    cur=con.cursor()
    experiment="INSERT INTO Experiments (problem,threads,chunk) VALUES (?,?,?)"
    option="INSERT INTO Options (eid,option,value) VALUES (?,?,?)"

    size={'NY':264346,'COL':435666,'FLA':1070376,'LKS':2758119,'W':6262104}

    ## Experiment 1
    ps=('NY','COL','FLA','LKS','W')
    cs=('bf-dist','bf-dist-tbb-task')
    runs=10
    
    ## Test
    #ps=('NY',)
    #cs=('bf-dist-tbb-task',)
    runs=1
   
    for p in ps:
        for c in cs:
            pf="USA-road-d.%s.gr.bin" % p
            cur.execute(experiment, (pf,1,-1))
            eid=cur.lastrowid
            cur.execute(option,(eid,'code',c))

    cur.close()
    con.commit()
    con.close()

    print "Experiment Run"
    con=sqlite3.connect(database)
    cur=con.cursor()
    cur.execute("SELECT eid,value FROM Experiments JOIN Options USING(eid) WHERE Options.option='code'")

    run=list()
    for row in cur:
        run.append(row)

    cur.close()
    con.close()

    ## Run experiments (database closed)
    for eid,code in run:
        for r in range(0,runs):
            os.system("time bin/%s -d %s -e %d" % (code,database,eid))
        
    print "Experiment Done"
