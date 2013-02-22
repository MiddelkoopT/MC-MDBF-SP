// PerfTools probe.hh
// Copyright (c) 2010 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#ifndef _mtim_profile_probe_
#define _mtim_profile_probe_

#include <iostream>
#include <tbb/enumerable_thread_specific.h>

#include "profile.hh"
#include "database.hh"

using namespace std;
using namespace mtim::profile;
using namespace mtim::database;

namespace mtim {
namespace probe {

namespace debug {
	const bool DEBUG=true;
	const bool TRACE=false;
	const bool CHECK=true;
}

class Probe {
public:
	static const int COUNTERS=4;
	typedef char const* counter_names_t[COUNTERS];
public:
	Profile* profile; // Probe uses local profile
	long count;
	long joins;
	long long interval;
	long long total;
	long counter[COUNTERS];
	Profile::values_t value;
private:
	counter_names_t counter_name;
public:
	Probe() : count(0), joins(0), interval(0), total(0) {
		if(debug::TRACE) cout << "Probe> " << this << endl;
		for(int i=0;i<COUNTERS;++i){
			counter[i]=0;
			counter_name[i]=NULL;
		}
		for(int i=0;i<Profile::max_events;++i){
			value[i]=0;
		}
	}
	Probe(const Probe& source) {
		if(debug::TRACE) cout << "Probe+> " << this << endl;
	}
	~Probe(){
		if(debug::TRACE) cout << "Probe~> " << this << endl;
	}

	inline void local(){ count++; profile=&Profilers::global->profile(); }
	inline void start(){ profile->start(); }
	inline void stop(){ total+=interval=profile->stop(value); }
	inline long long usec() { return total; }

	void join(const Probe& p){
		count+=p.count;
		joins+=p.joins+1;
		total+=p.total;
		for(int i=0;i<COUNTERS;++i){
			counter[i]+=p.counter[i];
		}
		for(int i=0;i<Profile::max_events;++i){
			value[i]+=p.value[i];
		}
	}

	void name(int counter, char const* name){
		if(debug::CHECK && counter>=COUNTERS) std::runtime_error("Counter bounds error");
		counter_name[counter]=name;
	}
	void names(const counter_names_t& counter_names){
		for(int i=0;i<COUNTERS;++i){
			counter_name[i]=counter_names[i];
		}
	}

	void print() const {
		cout << "Probe.print> msec:" << total/1000.0
				<< " count: " << count
				<< " joins: " << joins << "\n ";
		for(int i=0;i<COUNTERS;++i){
			if(counter_name[i]!=NULL && counter[i])
				cout << counter_name[i] << " ";
			if(counter[i])
				cout << counter[i] << "\n ";
		}
		Profilers::global->profile().print(value);

		cout << endl;
	}

	long long get(const int event) const {
		return Profilers::global->profile().get(event,value);
	}
};

// Old code
class Probes {
private:
	typedef tbb::enumerable_thread_specific<Probe,tbb::cache_aligned_allocator<Probe>,tbb::ets_key_per_instance> ProbeLocal;
	ProbeLocal probes;
public:
	static Probes* global;
public:
	Probes(){
		if(debug::TRACE) cout << "Probes>" << endl;
	}
	~Probes(){
		if(debug::TRACE) cout << "Probes~>" << endl;
	}
	/***
	 * Return a thread local probe to keep track of the core performance.
	 * @return Probe&
	 */
	inline Probe& probe(){
		return probes.local();
	}
	void print(){
		long long total=0;
		for(ProbeLocal::const_iterator i = probes.begin(); i!=probes.end(); ++i){
			cout << "Probe> " << i->total << endl;
			total+=i->total;
			i->print();
		}
		cout << "Probes> " << total/1000000.0 << " total core time" << endl;
	}
	void record(){
		int core=1;
		for(ProbeLocal::const_iterator i = probes.begin(); i!=probes.end(); ++i){
			Statistic::global->record("core",i->total,core++,-1);
		}
	}
};

}}//mtim:profile
#endif
