// PerfTools Experiment
// Copyright (c) 2008-2010 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#ifndef _mtim_experiment_hh_
#define _mtim_experiment_hh_

#include <string>
#include <sys/time.h>

#include "database.hh"
#include "profile.hh"
#include "probe.hh"

#include <tbb/spin_mutex.h>

namespace mtim {
namespace experiment {

namespace debug{
	const int DEBUG=true;
}

class Experiment {
public:
	 int eid;
	 int run;
	 string problem;
	 int threads;
	 int chunk;
private:
	 database::Database db;
	 long long	usec_start;
	 long long	usec_stop;
	 long long	usec_value;
public:
	 database::Statistic stat;
//	 probe::Probes probes;
	 profile::Profilers profilers;
public:
	 static void init();
	 Experiment(int argc, char** argv);
	 ~Experiment();
	 double getOption(const char* option,double value);
	 int record(const string& statname, double value){ return stat.record(statname,value,0,-1); }
	 void record(){ stat.record(); }
	 int next(){ return stat.next(); }

	 // wall clock timers
	 void start(){
		 timeval tv;
		 gettimeofday(&tv,NULL);
		 usec_start=tv.tv_sec * 1000000L + tv.tv_usec;
	 }
	 void stop(){
		 timeval tv;
		 gettimeofday(&tv,NULL);
		 usec_stop=tv.tv_sec * 1000000L + tv.tv_usec;
		 usec_value+=(usec_stop-usec_start);
	 }
	 long long interval() const {
		return usec_stop-usec_start;
	 }
	 long long wtime() const {
		return usec_value;
	 }

};

class Log {
private:
	typedef tbb::spin_mutex Mutex;
	static Mutex mutex;
	ostringstream out;
public:
	Log() {};
	Log(string message){
		out << message;
	}
    template <class T>
    friend Log& operator<<(Log& l, const T& val) {
          l.out << val;
          return l;
    }
	~Log(){
		Mutex::scoped_lock lock(mutex);
		cout << out.str() << endl;
	}
};

}}//mtim::experiment
#endif
