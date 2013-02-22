## multi-core paper Makefile
## Copyright 2009 Timothy Middelkoop.  All Rights Reserved.  Public Domain.

CONTRIB=../contrib
TBB=$(CONTRIB)
PAPI=$(CONTRIB)

-include config/Makefile.$(shell hostname -d)
-include config/Makefile.$(shell hostname)
-include Makefile.local

BRANCH=-fguess-branch-probability
ALIGN=-falign-functions -falign-jumps -falign-loops -falign-labels
OPTIMIZE:=-O2 ${BRANCH} ${ALIGN} ${OPTIMIZE}

CXXFLAGS+=-Wall -g $(OPTIMIZE) -I$(TBB)/include
LDLIBS+=-L$(TBB)/lib -ltbb_debug -Wl,-rpath,$(TBB)/lib -lsqlite3 -lssl -lcrypto

OBJS=build/getopt_pp.o build/bfdata.o build/database.o build/probe.o build/experiment.o build/thread.o

CXXFLAGS+=-I$(PAPI)/include -DPROFILE_PAPI
LDLIBS+=-L$(PAPI)/lib -lpapi -Wl,-rpath,$(PAPI)/lib
OBJS+=build/profile.o

ifdef EXPERIMENT
CXXFLAGS+=-DEXPERIMENT
endif

## Default Target
all: bf

## Targets	
bf: bin/bf-dist bin/bf-dist-tbb-task bin/mc-benchmark

configure:
	install -dv build/ bin/
	ln -sf config/$(shell hostname).h config.h

clean:
	rm -f bin/* build/*

data:
	./problem-data.py problem-B.gr
	./problem-data.py problem-C.gr

## Linking
bin/bf-dist: build/bf-dist.o $(OBJS)
bin/bf-dist-tbb-task: build/bf-dist-tbb-task.o $(OBJS)

bin/mc-benchmark: build/mc-benchmark.o build/mc-benchmark-tests.o $(OBJS)

## Rules
bin/%:
	g++ $(LDFLAGS) -o $@ $^ $(LDLIBS) 

build/%.o: %.cc
	g++ $(CXXFLAGS) -o $@ -c $< 

build/%.s: %.cc
	g++ $(CXXFLAGS) -S -o $@ $<

%.sqlite: %.sql
	sqlite3 $@ < $<

#%.sql: %.sqlite
#	sqlite3 $@ .dump > $<
