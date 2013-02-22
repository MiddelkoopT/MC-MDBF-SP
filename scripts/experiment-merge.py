#!/usr/bin/python

## Bellman-Ford Multi-Core test platform
## Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
## Licensed under the GPL 2.0 or later.

## Parameter test

import sys
import os
import string

from pysqlite2 import dbapi2 as sqlite3

if __name__ == "__main__":

    con=sqlite3.connect("experiments.sqlite")
    cur=con.cursor()

    files=os.listdir("../data")

    for f in files:
        print f
        cur.execute("ATTACH '../data/%s' AS Merge" % f)
        cur.execute("INSERT INTO Results SELECT * FROM Merge.Results")
        cur.execute("Detach Merge")
        os.remove("../data/%s" % f )

    con.close()
