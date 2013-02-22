// Multi-Core CPU Performance Benchmark
// Copyright (c) 2009 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// Compile with -O0 otherwise large portions will be optimized away

#include <iostream>
#include <string>

#include <pthread.h>

#include "profile.hh"
#include "config.h"

using namespace std;
using namespace mtim;
using namespace profile;

static const int pagesize=4096;

//typedef __uint128_t scratch_t;
typedef long scratch_t;

static const size_t stride=CONFIG_STRIDE;

static const int k_size=1024/sizeof(scratch_t);

static const int CPU_speed=CONFIG_SPEED;
static const int CPU_sockets=CONFIG_SOCKETS;
static const int CPU_cores=CONFIG_CORES;
static const int DRAM_size=CONFIG_DRAM*k_size;
static const int L3_size=CONFIG_L3*k_size;
static const int L2_size=CONFIG_L2*k_size;
static const int L1d_size=CONFIG_L1d*k_size;


class MCBenchmark {
private:
	string name;
	int threads;
	scratch_t* scratch;
	const size_t size;
	const int rank;

	bool is_serial;
	Timer p;

	static pthread_mutex_t lock;

public:
	MCBenchmark(string name,int threads,int rank,size_t size)
	: name(name), threads(threads), size(size), rank(rank), is_serial(false)
	{
		scratch=new scratch_t[size];
	}
	~MCBenchmark(){ delete scratch;}

	void report(string test,size_t size,int runs,string extra="",long long overhead=0);
	void set_serial();
	void random();

	scratch_t mcbstz(const int runs);
	scratch_t mcbstr(const int runs);
	void mcbstw(const int runs, scratch_t d);

	scratch_t mcbssr(const int runs) __attribute__((optimize(3)));
	void mcbssw(const int runs,const scratch_t d) __attribute__((optimize(3)));

	long long mcbssl(const int runs);
	scratch_t mcbssi(const int runs);
	scratch_t mcbsrr(const int runs);
	scratch_t mcbsrw(const int runs);


};

