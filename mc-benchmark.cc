// Multi-Core CPU Performance Benchmark
// Copyright (c) 2009 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// Compile with -O0 otherwise large portions will be optimized away

#include <iostream>
#include <algorithm>

#include <xmmintrin.h>
//#include <smmintrin.h>

#include "getopt_pp.hh"

#include "profile.hh"
#include "thread.hh"

#include "numaif.h"

#include "mc-benchmark-tests.hh"

using namespace std;
using namespace mtim;


class MemTest {
private:
	string name;
	int threads;
	int rank;
	int runs;
	size_t size;

	Thread& thread;

public:
	MemTest(string name,int threads,int rank,int runs,size_t size,Thread* thread)
	: name(name), threads(threads), rank(rank), runs(runs), size(size), thread(*thread){}

	void* run(){

	  //set_mempolicy(MPOL_BIND,NULL,MPOL_PREFERRED);

		MCBenchmark test(name,threads,rank,size);

		thread.barrier();
		test.mcbstz(2);
		test.mcbstz(runs);
		thread.barrier();
		test.report("STZ",size,runs);

		thread.barrier();
		test.mcbstr(2);
		test.mcbstr(runs);
		thread.barrier();
		test.report("STR",size,runs);

		thread.barrier();
		test.mcbstw(2,0);
		test.mcbstw(runs,0);
		thread.barrier();
		test.report("STW",size,runs);

		thread.barrier();
		test.mcbssr(2);
		test.mcbssr(runs);
		thread.barrier();
		test.report("SSR",size,runs);

		thread.barrier();
		test.mcbssw(2,0);
		test.mcbssw(runs,0);
		thread.barrier();
		test.report("SSW",size,runs);

		return NULL;
	}


};

void threadtest(string name,int threads,int runs,size_t size){

	Thread::init(threads);

	Thread thread[threads];
	MemTest* test[threads];

	for(int t=0;t<threads;++t){
		MemTest* tp=new MemTest(name,threads,t,runs,size,&thread[t]);
		test[t]=tp;
		thread[t].create(tp);
	}

	for(int t=0;t<threads;++t){
		thread[t].run();
	}

	for(int t=0;t<threads;++t){
		thread[t].join();
	}

	for(int t=0;t<threads;++t){
		delete test[t];
	}

}


int main(int argc,char** argv)
{
	cout << "mc-benchmark" << endl;
	//cout.imbue(locale(""));

	// Options
	GetOpt::GetOpt_pp options(argc, argv);

	int threads=1;
	options >> GetOpt::Option('n',threads);

	//int eventlist[]={0x4000207c, 0x4000407c};
	//int events=int(sizeof(eventlist)/sizeof(int));

	Profile::init();

	for(int t=threads;t;--t){

	  cout << "D,name,threads,thread,test,is_serial,size,size_of,stride,runs,bw,cycles" << endl;

	  int mul=1;

	  threadtest("D,L1",t,100000*mul,L1d_size);

	  if(L2_size)
		  threadtest("D,L2",t,10000*mul,L2_size);

	  if(L3_size) // Dual core
		  threadtest("D,L3",t,10000*mul,t>CPU_sockets?L3_size/((t-1)/CPU_sockets+1):L3_size);

	  threadtest("D,DRAM",t,10*mul,DRAM_size);

	}
	cout << "/mc-benchmark" << endl;

	return 0;
}
