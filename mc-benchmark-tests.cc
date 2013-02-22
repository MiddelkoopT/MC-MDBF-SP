// Multi-Core CPU Performance Benchmark
// Copyright (c) 2009 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// Compile with -O0 otherwise large portions will be optimized away

#include <iostream>
#include <algorithm>

#include "getopt_pp.hh"

#include "profile.hh"
#include "thread.hh"

#include "mc-benchmark-tests.hh"

using namespace std;
using namespace mtim;

pthread_mutex_t MCBenchmark::lock = PTHREAD_MUTEX_INITIALIZER;

void MCBenchmark::report(string test,size_t size,int runs,string extra,long long overhead)
{
	//p.print();
	pthread_mutex_lock(&lock);
	cout << name << "," << threads << "," << rank << "," << test << "," << is_serial << ",";
	cout << size*sizeof(scratch_t)/1024 << "," <<sizeof(scratch_t) << "," << stride << ",";
	cout << runs << "," << (sizeof(scratch_t)*size*(long)runs)/(p.interval()-overhead);
	cout << "," << (p.interval()/(size*runs/CPU_speed/1000))/1000.0;
	cout << extra;
	cout << endl;
	pthread_mutex_unlock(&lock);
}


// Stride based tests that defeat the prefetcher

/**
 * stride nothing
 * @param runs
 * @return
 */
scratch_t MCBenchmark::mcbstz(const int runs)
{
	const size_t size=this->size;
	scratch_t d=0;
	p.start();
	for(int r=0;r<runs;++r){
		for(size_t s=0;s<stride;s++){
			scratch_t* scratch=this->scratch+s;
			scratch_t* l=&d;
			for(size_t i=0;i<size;i+=(stride*8)){
				l=scratch+i+stride*0;
				l=scratch+i+stride*1;
				l=scratch+i+stride*2;
				l=scratch+i+stride*3;
				l=scratch+i+stride*4;
				l=scratch+i+stride*5;
				l=scratch+i+stride*6;
				l=scratch+i+stride*7;
			}
			d=*l;
		}
	}
	p.stop();
	return d;
}

/**
 * stride read
 * @param runs
 * @return
 */
scratch_t MCBenchmark::mcbstr(const int runs)
{
	const size_t size=this->size;
	scratch_t d=0;
	p.start();
	for(int r=0;r<runs;++r){
		for(size_t s=0;s<stride;s++){
			scratch_t* scratch=this->scratch+s;
			scratch_t l=0;
			for(size_t i=0;i<size;i+=(stride*8)){
				l=scratch[i+stride*0];
				l=scratch[i+stride*1];
				l=scratch[i+stride*2];
				l=scratch[i+stride*3];
				l=scratch[i+stride*4];
				l=scratch[i+stride*5];
				l=scratch[i+stride*6];
				l=scratch[i+stride*7];
			}
			d=l; // cheat the optimizer
		}
	}
	p.stop();
	return d;
}

/**
 * stride write
 * @param runs
 * @param d
 */
void MCBenchmark::mcbstw(const int runs, scratch_t d)
{
	const size_t size=this->size;
	p.start();
	for(int r=0;r<runs;++r){
		for(size_t s=0;s<stride;s++){
			scratch_t* scratch=this->scratch+s;
			scratch_t l=d;
			for(size_t i=0;i<size;i+=(stride*8)){
				scratch[i+stride*0]=l;
				scratch[i+stride*1]=l;
				scratch[i+stride*2]=l;
				scratch[i+stride*3]=l;
				scratch[i+stride*4]=l;
				scratch[i+stride*5]=l;
				scratch[i+stride*6]=l;
				scratch[i+stride*7]=l;
			}
		}
	}
	p.stop();
	return;
}

/**
 * Serial Sequential Read
 */
scratch_t MCBenchmark::mcbssr(const int runs)
{
	const size_t size=this->size;
	scratch_t* scratch=this->scratch;
	scratch_t d=0;

	p.start();
	for(int r=0;r<runs;++r){
		for(size_t i=0;i<size;++i){
			d+=scratch[i];
		}
	}
	p.stop();
	return d;
}

/**
 * Serial Sequential Write
 */
void MCBenchmark::mcbssw(const int runs,const scratch_t d)
{
	const size_t size=this->size;
	scratch_t* scratch=this->scratch;

	p.start();
	for(int r=0;r<runs;++r){
		for(size_t i=0;i<size;++i){
			scratch[i]=d;
		}
	}
	p.stop();
}

// Legacy codes

/**
 * Serial Sequential Loop Overhead
 */
long long MCBenchmark::mcbssl(const int runs)
{
	const size_t _size=size;

	p.start();
	for(int r=0;r<runs;++r){
		for(size_t i=0;i<_size;++i){
			// Empty
		}
	}
	p.stop();
	return p.interval();
}


/**
 * Serial Sequential Local Copy
 */
scratch_t MCBenchmark::mcbssi(const int runs)
{
	const size_t _size=size;
	scratch_t d=0;
	p.start();
	for(int r=0;r<runs;++r){
		for(size_t i=0;i<_size;++i){
			d=i;// Empty
		}
	}
	p.stop();
	return d;
}


/**
 * Serial Random Read
 */
scratch_t MCBenchmark::mcbsrr(const int runs)
{
	const size_t _size=size;
	scratch_t* scratch=this->scratch;
	scratch_t d=0;

	p.start();
	for(int r=0;r<runs;++r){
		size_t j=scratch[0];
		for(size_t i=0;i<_size;++i){
			j=scratch[j];
		}
		d=j;
	}
	p.stop();
	return d;

}

/**
 * Serial Random Write
 */
scratch_t MCBenchmark::mcbsrw(const int runs)
{
	const size_t _size=size;
	scratch_t* scratch=this->scratch;
	scratch_t d=0;

	p.start();
	for(int r=0;r<runs;++r){
		size_t j=scratch[0];
		for(size_t i=0;i<_size;++i){
			size_t l=j;
			j=scratch[j];
			scratch[l]=j;
		}
		d=j;
	}
	p.stop();

	return d;

}

void MCBenchmark::set_serial()
{
	scratch_t* scratch=this->scratch;
	for(size_t i=0;i<size;++i)
		scratch[i]=i+1;
	scratch[size-1]=0;
	is_serial=true;
}

void MCBenchmark::random()
{
	scratch_t* scratch=this->scratch;
	random_shuffle(scratch,scratch+size);

	// convert numbers to next pointers.
	scratch_t* lscratch=new scratch_t[size];
	for(size_t i=0;i<size;++i)
		lscratch[(int)(scratch[i])]=i;
	for(size_t i=0;i<size;++i)
		scratch[i]=lscratch[((int)(scratch[i]+1)%size)];
	delete lscratch;

	is_serial=false;
}

