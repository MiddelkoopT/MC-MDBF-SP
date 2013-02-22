## Copyright 2010 Timothy Middelkoop, Ph.D.  All rights reserved.

###### Data
library(DBI)
library(RSQLite)

library(plyr)
library(reshape)

drv <- dbDriver("SQLite")
db <- dbConnect(drv, dbname = "results/experiment-1.sqlite")

## Compare basic serial to TBB serial.
d <- dbGetQuery(db,"
	SELECT 
		Runs.eid, run, problem, Options.value,
		strftime('%s',stop)-strftime('%s',start) AS wtime
	FROM Runs
	JOIN Options USING (eid)
	JOIN Experiments USING (eid)
;")
d

## TBB Serial properties
d <- dbGetQuery(db,	"
	SELECT 
		Runs.eid, Runs.run, problem, 
		strftime('%s',stop)-strftime('%s',start) AS wtime,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='rtime') AS rtime,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='vtime') AS vtime,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='loopnodes' AND ref=1) AS nodes,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='iterprod' AND ref=1) AS iterprod
	FROM Runs
	JOIN Options USING (eid)
	JOIN Experiments USING (eid)
	WHERE Options.option='code' AND Options.value='bf-dist-tbb-task'
;")
d

## Problem scaling
d$steps <- as.integer(d$iterprod/d$nodes)
xyplot(wtime~steps,d,group=nodes)


