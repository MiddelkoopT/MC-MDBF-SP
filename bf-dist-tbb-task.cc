// Bellman-Ford Multi-Core test platform
// Copyright (c) 2008-2010 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.
//
// Distributed Bellman-Ford using TBB Task model.

#include <iostream>
#include <iomanip>
#include <locale>
#include <cmath>
using namespace std;

#include <sys/time.h>
#include <sys/resource.h>

#include <tbb/task_scheduler_init.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
using namespace tbb;

#include "experiment.hh"
#include "bfdata.hh"

using namespace mtim;
using namespace mtim::bfmc;
using namespace mtim::experiment;
using namespace mtim::probe;

#ifdef EXPERIMENT
#include "config-experiment.h"
#else
static const bool INFO=true;
static const bool DEBUG=true;
static const bool TRACE=false;
static const bool PROBE=false;
static const bool STAT=false;
static const bool STAT2=false;
static const bool CHECK=false;
#endif

Probe::counter_names_t probe_names = {
		"count1",
		"count2",
		"c_in",
		"c_out"
};

class Task_Communicate {
private:
	nodej* n;
public:
	Probe p;
public:
	Task_Communicate(nodej* n) : n(n) {};
	void operator() (const blocked_range<int>& r) {
		p.local();
		p.start();

		const int rb=r.begin();
		const int re=r.end();

		if(PROBE)p.counter[0]++;
		if(PROBE)p.counter[1]+=re-rb;

		for(int j=rb;j!=re;++j){
			nodej& nj=n[j];
			for(int a=0;a<nj.An;a++){
				int k=nj.A[a].k;
				// intra-block communication only.
				if( k>=rb && k<re ){
					*nj.A[a].Dk=nj.Dj;
					if(PROBE)p.counter[2]++;
				}else{
					if(PROBE)p.counter[3]++;
				}
			}
		}
		p.stop();
	}
private: // Hidden default methods
	Task_Communicate(const Task_Communicate&);
};


class Task_Compute {
private:
	nodej* n;
public:
	int count;
	probe::Probe p;
public:
	Task_Compute(nodej* n) : n(n), count(0) {};
	void operator() (const blocked_range<int>& r) {
		p.local();
		p.start();

		int _count=0;
		const int rb=r.begin();
		const int re=r.end();

		if(PROBE)p.counter[0]++;
		if(PROBE)p.counter[1]+=re-rb;

		for(int j=rb;j!=re;++j){
			nodej& nj=n[j];
			if(TRACE)
				cout << "compute " << nj.j << endl;
			for(int b=0;b<nj.Bn;b++){
				if(TRACE)
					cout << "c: " << nj.B[b].c << endl;
				const int& Di=nj.B[b].Di;
				const int& c=nj.B[b].c;
				if(Di < nj.Dj - c ){
					if(PROBE)p.counter[2]++;
					nj.Dj=Di + c;
					nj.predj=nj.B[b].i;
					_count++;
				}else{
					if(PROBE)p.counter[3]++;
				}
			}
			if(TRACE && count)
				cout << "Dj: " << nj.Dj << endl;
		}
		count=_count;
		p.stop();
	}
private: // Hidden default methods
	Task_Compute(const Task_Compute&);
};

// Main parallel tool.

class Task_Loop {
public:
	nodej* n;
	Task_Compute compute;
	Task_Communicate communicate;
	Task_Compute first_compute;
	Task_Communicate first_communicate;
public:
	int count;		// total number of node iterations that had an update
	int loops; 		// number of blocks evaluated
	long loopnodes; // number of nodes that are evaluated in a block (first)
	long iternodes; // number of nodes that are iterated in a block (non-first)
	long iterprod;  // number of iterations node is iterated (non-first)
public:
//	Probe p;
public:
	Task_Loop(nodej* n)
	: n(n),
	  compute(n), communicate(n), first_compute(n), first_communicate(n),
	  count(0), loops(0), loopnodes(0), iternodes(0), iterprod(0) {}
	Task_Loop(Task_Loop& t, split)
	: n(t.n),
	  compute(t.n), communicate(t.n), first_compute(t.n), first_communicate(t.n),
	  count(0), loops(0), loopnodes(0), iternodes(0), iterprod(0) {}
	void operator() (const blocked_range<int>& r) {
		// Loop section (load)
		first_compute(r);
		first_communicate(r);

		count+=first_compute.count;
		loops++;
		loopnodes+=r.end()-r.begin();

		// Iter section (split loop)
		long _iter=0;
		compute.count=first_compute.count;
		long compute_total=0;
		long communicate_total=0;
		while(compute.count){
			compute(r);
			communicate(r);
			count+=compute.count;
			if(STAT2){
				compute_total+=compute.p.interval;
				communicate_total+=communicate.p.interval;
			}
			_iter++;
		}
		if(_iter>0){
			iternodes+=(r.end()-r.begin());
			iterprod+=_iter*(r.end()-r.begin());
		}
		if(STAT2){
			Statistic::global->store("eval",_iter+1,r.begin());
			Statistic::global->store("size",r.end()-r.begin(),r.begin());
			Statistic::global->store("loadcomp",first_compute.p.interval,r.begin());
			Statistic::global->store("itercomp",compute_total,r.begin());
			Statistic::global->store("loadcomm",first_communicate.p.interval,r.begin());
			Statistic::global->store("itercomm",communicate_total,r.begin());
		}
	}
	void join(const Task_Loop& t) {
		// Join profilers
//		p.join(t.p);
		compute.p.join(t.compute.p);
		communicate.p.join(t.communicate.p);
		first_compute.p.join(t.first_compute.p);
		first_communicate.p.join(t.first_communicate.p);

		count+=t.count;
		loops+=t.loops;
		loopnodes+=t.loopnodes;
		iternodes+=t.iternodes;
		iterprod+=t.iterprod;
	}
	void print(){
		cout << "*** Compute" << endl;
		first_compute.p.names(probe_names);
		first_compute.p.print();
		compute.p.names(probe_names);
		compute.p.print();

		cout << "*** Communicate" << endl;
		first_communicate.p.names(probe_names);
		first_communicate.p.print();
		communicate.p.names(probe_names);
		communicate.p.print();
	}
private: // Hidden default methods
	Task_Loop(const Task_Loop&);
};


int main(int argc, char** argv){

	cout << "main> bf-dist-tbb-task.cc " << endl;

	if(STAT==false) // disable event collection
		Profile::default_events=0;

	Experiment::init();
	Experiment experiment(argc,argv);
	task_scheduler_init tbb(experiment.threads);

	// Read data
	Data data("../data/"+experiment.problem);
	const int N=data.nodes;
	if(experiment.chunk<0)
		experiment.chunk=data.nodes;

	// Timer only profiler
	Timer timer;
	timer.start();

	// TODO: integrate into Timer (use a parallel/serial method and track both)
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	double utime=usage.ru_utime.tv_sec*1e6+usage.ru_utime.tv_usec;
	double stime=usage.ru_stime.tv_sec*1e6+usage.ru_stime.tv_usec;

	// Setup for solve
	const int _chunk=experiment.chunk; // suggested blocksize (configuration)
	long steps=0;
	int done=0;

	// Root task
	Task_Loop main(data.node);
	Task_Communicate global(data.node);

	// Main loop.
	while(done<2){ // loop twice with zero communication to terminate
		Task_Loop loop(data.node);

		global(blocked_range<int>(1,N+1,N+1));

		experiment.next();
		parallel_reduce(blocked_range<int>(1,N+1,_chunk),loop,simple_partitioner());
		steps++;
		done++;

		cout << "*** " << loop.iternodes << " " << loop.count <<  endl;

		if(loop.count>0)
			done=0; // reset counter

		// Record Stats
		main.join(loop);

		if(STAT){
			timer.stop();
			long long total;
			total=main.compute.p.value[0] +
				main.communicate.p.value[0] +
				main.first_compute.p.value[0] +
				main.first_communicate.p.value[0] +
				global.p.value[0];
			experiment.record("PAPI_TOT_CYC",total);
			total=main.compute.p.value[1] +
				main.communicate.p.value[1] +
				main.first_compute.p.value[1] +
				main.first_communicate.p.value[1] +
				global.p.value[1];
			experiment.record("PAPI_TOT_INS",total);
			experiment.record(); // flush out stored data
			timer.start();
		}
	}
	timer.stop();

	// Gather delta resource usage
	getrusage(RUSAGE_SELF, &usage);
	utime=usage.ru_utime.tv_sec*1e6+usage.ru_utime.tv_usec - utime;
	stime=usage.ru_stime.tv_sec*1e6+usage.ru_stime.tv_usec - stime;

	// Solution hash
	for(int j=1;j<=N;j++)
		experiment.stat.hash(data.node[j].Dj);

	// Display results
	cout << "*** Main " << endl;
	global.p.print();
	cout << " total msec:";
	cout << main.compute.p.usec()/1000.0 +
			main.communicate.p.usec()/1000.0 +
			main.first_compute.p.usec()/1000.0 +
			main.first_communicate.p.usec()/1000.0 +
			global.p.usec()/1000.0 << endl;

	main.print();
	cout << "*** Global " << endl;
	global.p.print();

	cout << "*** Record " << endl;
	experiment.record();

	experiment.record("threads",experiment.threads);
	experiment.record("chunk",experiment.chunk);
	experiment.record("nodes",data.nodes);

	experiment.record("steps",steps);
	experiment.record("loops",(main.loops));
	experiment.record("loopnodes",main.loopnodes);
	experiment.record("iternodes",main.iternodes);
	experiment.record("iterprod",main.iterprod);

	experiment.record("compute-time",main.compute.p.usec());
	experiment.record("communicate-time",main.communicate.p.usec());
	experiment.record("loadcomp-time",main.first_compute.p.usec());
	experiment.record("loadcomm-time",main.first_communicate.p.usec());
	experiment.record("global-time",global.p.usec());

	experiment.record("utime",utime);
	experiment.record("stime",stime);
	experiment.record("vtime",timer.usec());

	// Shutdown
	tbb.terminate();
	cout << "main> done." << endl;
	return 0;
}
