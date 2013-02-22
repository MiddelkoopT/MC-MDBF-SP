## Copyright 2010 Timothy Middelkoop, Ph.D.  All rights reserved.

# options(width=60)
# options(width=100)

# do.pdf <- FALSE
do.pdf <- TRUE

###### Data
library(DBI)
library(RSQLite)

library(plyr)
library(reshape)

library(lattice)
library(stats)

drv <- dbDriver("SQLite")

## Runs (missing map of serial/single eid->run works since run=eid)
query <- "
	SELECT 
		Runs.eid, Runs.run, problem, threads, chunk, 
		(SELECT value FROM Options WHERE Runs.eid=eid AND option='serial') AS serialID,
		(SELECT value FROM Options WHERE Runs.eid=eid AND option='single') AS singleID,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='nodes') AS nodes,

		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='steps') AS steps,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='loops') AS loops,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='loopnodes') AS loopnodes,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='iternodes') AS iternodes,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='iterprod') AS iterprod,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='communicate-time') AS itercomm,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='compute-time') AS itercomp,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='loadcomp-time') AS loadcomp,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='loadcomm-time') AS loadcomm,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='global-time') AS global,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='stime') AS stime,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='utime') AS utime,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='vtime') AS vtime,
		(SELECT value FROM Statistics WHERE run=Runs.run AND stat='wtime') AS wtime

	FROM Runs
	JOIN Experiments USING (eid)
	WHERE Runs.stop NOT NULL
	ORDER BY eid, run
;"

## Load Data
db <- dbConnect(drv, dbname = "src/results/experiment-2v3.sqlite")
d <- dbGetQuery(db,query);

## Derived
# number of blocks the problem is divided into
d$blocks <- d$nodes/(d$loopnodes/d$loops)
d$abs <- d$nodes/d$blocks

# total evalutation time 
d$communicate <- d$loadcomm + d$itercomm
d$compute <- d$loadcomp + d$itercomp
d$parallel <- d$loadcomp + d$loadcomm + d$itercomp + d$itercomm
d$serial <- d$global
d$total <- d$serial + d$parallel

# total number of nodes evaluated (in block computation)
d$iters <- d$loopnodes + d$iterprod

# total number of node evaluations
d$evals <- d$loopnodes + d$iterprod + d$steps*d$nodes


# shortened for dumping data/labels
d$c <- d$chunk
d$t <- d$threads
d$n <- d$nodes
d$node <- d$nodes

## Write out data
#write.csv(d,'experiment-2.csv')

## Config
# cycles per usec
cpu <- 2300 # CEOCI 

## Colors/PCH
#x<-0:20
#xyplot(x~x,pch=x,groups=x,)
trellis.par.set(superpose.symbol = list(pch = c(1,0,3,4,2)))

###### Stats

## average across runs
#a <- cast(melt(d,id=1:8), nodes + chunk + threads ~ variable, mean)

#### Problem size and threads
if(do.pdf) pdf('paper/plots/e2-size.pdf')
xyplot(vtime~nodes,subset(d,chunk>0 ),scales=list(log=TRUE),group=threads,
		auto.key=list(space="right",title="cores"))
if(do.pdf) dev.off()

if(do.pdf) pdf('paper/plots/e2-threads.pdf')
xyplot(vtime~threads,subset(d,chunk>0 & nodes==264346),scales=list(y=list(log=TRUE)),
		group=n,auto.key=list(space="right",title="nodes"),type='p')
if(do.pdf) dev.off()

# summary(m <- lm(log(vtime)~log(nodes)+log(abs)+log(threads),subset(d,chunk>0)))

#### Alogrithm effects

## Problem difficulty, number of serial iterations for the serial version
xyplot(iterprod/nodes~nodes,subset(d,chunk<0),scales=list(log=TRUE),group=nodes,
		auto.key=list(space="right",title="nodes"),type='o')

## Factor: serial/steps
## Increased block size decreases the number of steps
if(do.pdf) pdf('paper/plots/e2-steps.pdf')
xyplot(steps~abs,subset(d,chunk>0),scales=list(log=TRUE),group=nodes,
		auto.key=list(space="right",title="nodes"),type='o')
if(do.pdf) dev.off()

## Factor: parallel/iterations
## Increasing the block size increases the number of iterations per node
if(do.pdf) pdf('paper/plots/e2-iters.pdf')
xyplot(iters/nodes~abs,subset(d,chunk>0),group=nodes,
		scales=list(x=list(log=TRUE),y=list(log=TRUE)),
		auto.key=list(space="right",title="nodes"),type='o')
if(do.pdf) dev.off()


#### Macro effects.
## PAPI seems to track system time usage.

## Unaccounted for time (including locking)
d$overhead <- d$utime+d$stime-d$global-d$compute-d$communicate
d$idle <- ((d$wtime-d$global)*d$threads-(d$utime-d$global)-d$stime)
d$idle_serial <- d$global*d$threads-d$global;
d$macro <- (d$wtime-d$global)*d$threads-(d$compute+d$communicate)

## High correlation between system time and overhead.
#xyplot(overhead~stime,subset(d,chunk>0 ),group=threads,
#		scales=list(log=TRUE),auto.key=list(space="right",title="nodes"),type='p')
#summary(m<-lm(log(overhead)~log(stime),subset(d,stime>10000)))

## Parallel idle time per node
if(do.pdf) pdf('paper/plots/e2-idle.pdf')
xyplot(idle/node~abs,subset(d,chunk>0),
		group=threads,
		scales=list(log=TRUE), auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()

## Program overhead (TBB/Locking)
if(do.pdf) pdf('paper/plots/e2-overhead.pdf')
xyplot(overhead*cpu/iters~abs,subset(d,chunk>0),
		group=threads,scales=list(log=TRUE),
		auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()

## Scheduler overhead -- processing/locking
xyplot(overhead~loops,subset(d,chunk>0 & threads==1),group=abs<10000,
		scales=list(log=TRUE),auto.key=list(space="right",title="nodes"),type='p')

## Idle time increases with block size
#if(do.pdf) pdf('paper/plots/e2-idle-loop.pdf')
xyplot(idle/threads/loops~abs,subset(d,chunk>0 & threads==8),group=nodes,
		scales=list(log=TRUE),auto.key=list(space="right",title="nodes"),type='p')
#if(do.pdf) dev.off()

## Shows the relationship between idle time and overhead per loop
if(do.pdf) pdf('paper/plots/e2-waiting-overhead.pdf')
xyplot(overhead/loops~idle/loops/threads,subset(d,chunk>0 & threads>1),
		group=threads,
		scales=list(log=TRUE),auto.key=list(space="right",title="nodes"),type='p')
if(do.pdf) dev.off()

## Macro effects.
if(do.pdf) pdf('paper/plots/e2-macro.pdf')
xyplot(macro*cpu/iters~abs,subset(d,chunk>0),
		group=threads,
		ylab='cycles of overhead per parallel node evaluation',
		xlab='average block size',
		scales=list(x=list(log=TRUE)),
		auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()


## Stats.
summary(round(d$overhead/(d$utime+d$stime),4)*100)
summary(round(d$idle/(d$wtime*d$threads),4)*100)

#### Micro effects

d$cpglobal <- d$global/(d$steps*d$nodes)*cpu
d$cploadcomp <- d$loadcomp/d$loopnodes*cpu
d$cploadcomm <- d$loadcomm/d$loopnodes*cpu
d$cpitercomp <- d$itercomp/d$iterprod*cpu
d$cpitercomm <- d$itercomm/d$iterprod*cpu

#subset(d,select=c(n,c,t,cpglobal,cploadcomp,cploadcomm,cpitercomp,cpitercomm))

## Global times remain constant.  Times differ due to node connectivity and density.
if(do.pdf) pdf('paper/plots/e2-global.pdf')
xyplot(cpglobal~abs,subset(d,chunk>0),group=nodes,
		scales=list(x=list(log=TRUE)),auto.key=list(space="right",title="nodes"),type='p')
if(do.pdf) dev.off()

## Parallel graph, plot on the same axis
lim <- c(0,500)

## Load computation times remain constant.   drop when more bus utilization
if(do.pdf) pdf('paper/plots/e2-loadcomp.pdf')
xyplot(cploadcomp~abs,subset(d,chunk>0),group=t,
		scales=list(x=list(log=TRUE),y=list(limits=lim)),
		auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()

## Load communication times show similar behaviour but more pronounced. 
##   Small ABS show cache effect (100 cycles)
##   Dramatic effect as cache runs out
if(do.pdf) pdf('paper/plots/e2-loadcomm.pdf')
xyplot(cploadcomm~abs,subset(d,chunk>0),group=t,
		scales=list(x=list(log=TRUE),y=list(limits=lim)),
		auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()

## Hot cache is 50 cycles.  Tail off is from lower iterations at the end. 
if(do.pdf) pdf('paper/plots/e2-itercomp.pdf')
xyplot(cpitercomp~abs,subset(d,chunk>0),group=t,
		scales=list(x=list(log=TRUE),y=list(limits=lim)),
		auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()

## Similar but higher penalty (writes?)
if(do.pdf) pdf('paper/plots/e2-itercomm.pdf')
xyplot(cpitercomm~abs,subset(d,chunk>0),group=t,
		scales=list(x=list(log=TRUE), y=list(limits=lim)),
		auto.key=list(space="right",title="cores"),type='p')
if(do.pdf) dev.off()

######
### IERC Paper...

# main factors are nodes, threads, steps, iterations

## macro uncached

# lookup for single performance
spmacroMap <- tapply(d$macro,factor(d$eid),mean)
d$macroSingle <- spmacroMap[d$singleid]

xyplot((macro)*cpu/iters~abs,subset(d,chunk>0 & abs>16384 & threads<4),
		group=threads,
		scales=list(x=list(log=TRUE)),
		auto.key=list(space="right",title="cores"),type='p')

## too small of effect with cache.
summary(m1 <- lm((macro)*cpu/iters~t,subset(d,chunk>0 & abs<16384)))

## higher impact w/ cache 
summary(m2 <- lm((macro)*cpu/iters~t,subset(d,chunk>0 & abs>16384)))

## simplified model
a <- subset(d,chunk>0 & abs<16384)
m3 <- mean(a$macro*cpu/a$iters)
m3
for(i in c(1,2,4,8)) {
	a <- subset(d,chunk >0 & abs<16384 & threads==i)
	print(mean(a$macro*cpu/a$iters))
}

## paper uses m1 and m3


### prediction model (IERC Numbers)
# cached, threads,
# overhead/iters, 
# global/(steps*nodes),
# first compute/loopnodes 
# first communicate/loopnodes, 
# iter compute/iterprod, 
# iter communicate/iterprod

d$cache <- d$abs<16384
model <- data.frame(
		cached=c(TRUE,TRUE,FALSE,FALSE), 
		threads=c(1,8,1,8), 
		global=c(145,145,145,145),
		overhead=c(10,10,3,234), 
		fcomp=c(102,224,98,330),
		fcomm=c(71,71,98,381),
		icomp=c(35,35,75,251),
		icomm=c(49,49,96,297)
)

p <- data.frame()
for(T in c(1,8)){
	for (C in c(TRUE,FALSE)){
		m<-model[model$cached==C & model$threads==T,]
		print(m)
		a <- subset(d,cache==C & threads==T & chunk>0)
		print(nrow(a))
		a$predicted = m$global*a$steps*a$nodes + ( 
					m$overhead*a$iters + 
					m$fcomp*a$loopnodes + 
					m$fcomm*a$loopnodes + 
					m$icomp*a$iterprod + 
					m$icomm*a$iterprod
					) / a$threads
		
		a$ptotal <- m$global*a$steps*a$nodes +
				m$overhead*a$iters + 
				m$fcomp*a$loopnodes + 
				m$fcomm*a$loopnodes + 
				m$icomp*a$iterprod + 
				m$icomm*a$iterprod
		
		a$pglobal <- m$global*a$steps*a$nodes / a$ptotal * 100
		a$poverhead <- m$overhead*a$iters  / a$ptotal * 100
		a$pfcomp <- m$fcomp*a$loopnodes / a$ptotal * 100
		a$pmcomm <- m$fcomm*a$loopnodes / a$ptotal * 100
		a$picomp <- m$icomp*a$iterprod / a$ptotal * 100
		a$picomm <- m$icomm*a$iterprod / a$ptotal * 100
		
		a$predictedtime <- a$predicted/cpu 
		p <- rbind(p,a)
		
	}
}

p$wtotal <- (p$wtime - p$serial)*p$threads + p$serial
p$cpglobal <- p$global/p$wtotal * 100
p$cpfcomp <- p$loadcomp/p$wtotal * 100
p$cpfcomm <- p$loadcomm/p$wtotal * 100
p$cpicomp <- p$itercomp/p$wtotal * 100
p$cpicomm <- p$itercomm/p$wtotal * 100

(p$pglobal + p$poverhead + p$pfcomp + p$pmcomm + p$picomp + p$picomm)
p$cpoverhead <- 100-(p$cpglobal + p$cpfcomp + p$cpfcomm + p$cpicomp + p$cpicomm)

summary(m4 <- lm(predictedtime~wtime,p))
p$pe <- (p$wtime-p$predictedtime)/p$wtime * 100

plot(p$pe)

subset(p,pe< -30)

sqrt( sum( ((p$predictedtime-p$wtime)/p$wtime)^2 )/nrow(p) ) * 100
mape <- ( sum( abs((p$predictedtime-p$wtime)/p$wtime) )/nrow(p) ) * 100

#### Summary of overall IERC Analysis 

## Mean Absolute Persentage Error
mape

## less than 10% outliers (unknown but all uncached, similar percentage breakdown)
sum(p$pe< -30)/nrow(p) * 100

## Good fit not much bias to predictor.
summary(m4)

## Plot of predictions

if(do.pdf) pdf('paper/plots/e2-compare.pdf')
xyplot(predictedtime~wtime,subset(p,chunk>0),
		scales=list(x=list(log=TRUE),y=list(log=TRUE)),
		ylab='wall clock time [log(usec)]',
		xlab='predicted wall clock time [log(seconds)]',
		type='p')
if(do.pdf) dev.off()



###### OLD MATERIAL ######

######
### Impact Reports

### Computational time
#d$wtime*d$threads-(d$serial+d$parallel+d$overhead+d$idle+d$idle_serial)

## Single
b <- cast(melt(subset(d,chunk<20000,select=c(1:8,global,loadcomp,loadcomm,itercomp,itercomm,overhead,idle,idle_serial)),id=1:8),
		variable ~ threads ~ chunk | nodes,subset=chunk>0,mean,na.rm=TRUE)

barplot(b[[1]][,'8',],col=rainbow(8),legend=TRUE,args.legend=list(x='topleft'))

par(mfrow=c(5,4))
for(n in b) {
	for(t in dimnames(n)$threads){
		barplot(n[,t,],col=rainbow(8))#,legend=TRUE,args.legend=list(x='topleft'))
	}
}
par(mfrow=c(1,1))

## Grid
a <- as.data.frame(cast(melt(d,id=1:8), nodes + chunk + threads ~ variable, mean))
b <- melt(subset(a,chunk>0,select=c(chunk,threads,nodes, global,loadcomp,loadcomm,itercomp,itercomm,overhead,idle,idle_serial)),id=1:3)

b$chunk <- factor(b$chunk)
b$threads <- factor(b$threads)
b$nodes <- factor(b$nodes)
b$variable <- factor(b$variable)
factor(d$nodes)
barchart(value~threads|chunk,subset(b,chunk!=-1 & nodes==1070376),groups=variable,
		box.ratio=8,stack=TRUE,col=rainbow(8))

#### Wall clock time (value/threads)
p <- d
p$loadcomp <- p$loadcomp/p$threads
p$loadcomm <- p$loadcomm/p$threads
p$itercomp <- p$itercomp/p$threads
p$itercomm <- p$itercomm/p$threads
p$overhead <- p$overhead/p$threads
p$idle <- p$idle/p$threads

# p$wtime-(p$global+p$loadcomp+p$loadcomm+p$itercomp+p$itercomm+p$overhead+p$idle)

## Single
b <- cast(melt(subset(p,select=c(1:8,global,loadcomp,loadcomm,itercomp,itercomm,overhead,idle)),id=1:8),
		variable ~ threads ~ chunk | nodes,subset=chunk>0,mean,na.rm=TRUE)

barplot(b[[3]][,'8',],col=rainbow(8),legend=TRUE,args.legend=list(x='topleft'))

## Grid
a <- as.data.frame(cast(melt(p,id=1:8), nodes + chunk + threads ~ variable, mean))
b <- melt(subset(a,chunk>0,select=c(chunk,threads,nodes, global,loadcomp,loadcomm,itercomp,itercomm,overhead,idle)),id=1:3)

b$chunk <- factor(b$chunk)
b$threads <- factor(b$threads)
b$nodes <- factor(b$nodes)
b$variable <- factor(b$variable)

n <- 6262104
n <- 1070376
barchart(value~chunk|threads,subset(b,nodes==n),groups=variable,
	box.ratio=8,stack=TRUE,col=rainbow(8))

######
###### Factor Reports
nf <- function(x) { x/min(x,na.rm=TRUE)} ## Normalize factor
na.min <- function(x) ifelse(length(x)>0,min(x),NA) ## Minimums with NA

#### Alogrithmic factors (same across threads)
b <- cast(melt(d,id=1:8), chunk ~ nodes | variable,subset=chunk>0,mean)

## Steps
b$steps
y <- apply(b$steps,c(2),nf)
matplot(rownames(y),y,type='o',log='xy')

## Iterations
b$iters
y <- apply(b$iters,c(2),nf)
matplot(rownames(y),y,log='xy',type='o')
#heatmap(y,Rowv=NA, Colv=NA, scale="none", col = topo.colors(16))

#### Macro factors
b <- cast(melt(d,id=1:8), chunk ~ threads ~ nodes | variable,subset=chunk>0,mean,na.rm=TRUE)

dimnames(b$overhead)$nodes
n <- '1070376' #debuging/visual

if(do.pdf) pdf('paper/plots/e2-macro-factors.pdf')
par(mfrow=c(length(dimnames(b$overhead)$nodes),3))
for(n in dimnames(b$overhead)$nodes) {
	## Overhead
	y <- apply(b$overhead[,,n],c(2),nf)
	matplot(rownames(y),y,type='o',log='xy',pch=c('1','2','4','8'),
			main=paste('nodes:',n),xlab='chunk size (nodes)',ylab='overhead (usec)')

	## Idle
	y <- apply(b$idle[,,n],c(2),nf)
	matplot(rownames(y),y,type='o',log='xy',pch=c('1','2','4','8'),
			main=paste('nodes:',n),xlab='chunk size (nodes)',ylab='idle (usec)')

	## Macro
	y <- apply((b$overhead+b$idle)[,,n],c(2),nf)
	matplot(rownames(y),y,type='o',log='xy',pch=c('1','2','4','8'),
			main=paste('nodes:',n),xlab='chunk size (nodes)',ylab='macro (usec)')
}
par(mfrow=c(1,1))
if(do.pdf) dev.off()


#### Micro factors

## Cache/Uncached setup.
bp <- 10^4.0 ## breakpoint on the graph

## chunk size is easier to manage.
x <- c(max(d$chunk[d$abs<bp]),min(d$chunk[d$abs>bp & d$chunk>0]))
cache <- mean(x)
## display ABS values on graph
max(d$abs[d$chunk<=cache & d$chunk>0])
min(d$abs[d$chunk>cache])
cache

## sequential serial load ~ 145 cycles
b <- cast(melt(d,id=1:8), . ~ variable,subset=chunk>0,c(mean,median,sd))
b$cpglobal_mean
b$cpglobal_median
b$cpglobal_sd

## cached block load
b <- cast(melt(d,id=1:8), threads ~ variable,subset=chunk>0 & chunk<=cache,c(mean,median,sd,min,max))
round(b$cploadcomp_mean)
b$cploadcomp_min

b <- cast(melt(d,id=1:8), threads ~ nodes ~ variable,subset=chunk>0 & chunk<=cache,c(mean,median,sd,min,max))
b


## cached block iteration
b <- cast(melt(d,id=1:8), run ~ variable ,subset=chunk>0 & chunk<=cache )
round(mean(b$cploadcomm))
round(mean(b$cpitercomp)) 
round(mean(b$cpitercomm))

round(mean(c(b$cploadcomm,b$cpitercomp,b$cpitercomm))) 
plot(b$cpitercomp)

## uncached block access by thread
b <- cast(melt(d,id=1:8), run ~ variable | threads ,subset=chunk>cache)
as.integer(lapply(b,function(x) { round(mean(x$cploadcomp)) } ))
as.integer(lapply(b,function(x) { round(mean(x$cploadcomm)) } ))
as.integer(lapply(b,function(x) { round(mean(x$cpitercomp)) } ))
as.integer(lapply(b,function(x) { round(mean(x$cpitercomm)) } ))

as.integer(lapply(b,function(x) { round(mean(c(x$cploadcomp,x$cploadcomm,x$cpitercomp,x$cpitercomm))) } ))

#subset(d,chunk<=cache & chunk>0,select=c(n,t,c,abs,cploadcomp,cploadcomm,cpitercomp,cpitercomm))

#### Micro/Alogrithmic contribution factor for parallel computation.

## Generate new columns

# Parallel Time if number of iters was the same as chunk==-1
a <- cast(melt(d,id=1:8), nodes ~ chunk ~ variable,mean)
y <- a[,,'iters']/a[,1,'iters']
y <- a[,,'parallel']/y

y1 <- melt(y)
y1$variable <- 'sf_iters_parallel'
a <- as.data.frame(cast(merge(melt(a),y1,all=TRUE),
		nodes + chunk ~ variable, mean))

# Compute parallel time using cached values.

## Reduce the number of columns used.
b <- melt(subset(a,chunk>0 & nodes < 1e6,select=c(chunk,nodes,  
						sf_iters_parallel,parallel )),id=1:2)

b$chunk <- factor(b$chunk)
b$nodes <- factor(b$nodes)
b$variable <- factor(b$variable)

barchart(value~chunk|nodes,b,groups=variable,
		box.ratio=8,stack=FALSE,col=rainbow(8))



