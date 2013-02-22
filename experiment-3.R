## Copyright 2010 Timothy Middelkoop, Ph.D.  All rights reserved.
## Experiment 3 -- Probe properties.

# options(width=90)
# options(width=100)

###### Data
library(DBI)
library(RSQLite)
library(plyr)
library(reshape)
library(lattice)

drv <- dbDriver("SQLite")

## Stat only view
query_stat <- "
	SELECT 
		run, ref, id, stat, value
	FROM Statistics
	WHERE ref < 0
	ORDER BY run, ref, id
;"

## Medium Data
query_ref <- "
	SELECT 
		run, ref, id, stat, value
	FROM Statistics
	WHERE ref > 0
	ORDER BY run, ref, id
;"


db <- dbConnect(drv, dbname = "results/experiment-3.sqlite")
a <- cast(dbGetQuery(db,query_ref),ref ~ id | run + stat)
#s <- cast(dbGetQuery(db,query_stat),run+ref+id~stat)
#r <- cast(dbGetQuery(db,query_ref),run+ref+id~stat)

# Config
cpu <- 2300 # CEOCI

# Analysis
names(r)
plot(r$loadcomp/r$size)


run <- '1'
x <- as.matrix(a[[run]]$loadcomp)/as.matrix(a[[run]]$size)*cpu
x <- as.matrix(a[[run]]$itercomp)/as.matrix(a[[run]]$size)/(as.matrix(a[[run]]$eval)-1)*cpu
x <- as.matrix(a[[run]]$eval)
heatmap(x, Rowv=NA, Colv=NA, scale="none",col = topo.colors(16))
hist(as.vector(x))

x <- rowMeans(x)
plot(x)


## Large Data
query <- "
		SELECT 
		run, ref, id, stat, value
		FROM Statistics
		WHERE stat='itercomp'
		;"

db <- dbConnect(drv, dbname = "experiments.sqlite")
d <- dbGetQuery(db,query)

hist(d$value,breaks=60)
hist(subset(d,value > 0 & value < 5 )$value,breaks=60)

