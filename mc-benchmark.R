## multi-core paper stats
## Copyright 2009 Timothy Middelkoop.  All Rights Reserved.  Public Domain.

d <- read.csv(file="test.csv",head=TRUE,sep=",")


d$tp[d$runs==100]

plot(tp~size,subset(d,runs>2 & test=='SSR' & size>0 & size <8192))
