// C++ PAPI library
// Copyright (c) 2008, 2009 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// WARNING: Turn off frequency scaling.

#ifndef _mtim_profile_papi_hh_
#define _mtim_profile_papi_hh_

#include <iostream>
#include <stdexcept>
using namespace std;

#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <papi.h>

#include <tbb/enumerable_thread_specific.h>

namespace mtim {
namespace profile {

namespace debug {
	const int DEBUG=true;
	const int TRACE=false;
}

inline void papi_ok(int retval) {
	if(retval==PAPI_OK)
		return;
	else if(retval==PAPI_ESYS)
		throw std::runtime_error(strerror(errno));
	else{
		char error[PAPI_MAX_STR_LEN];
		PAPI_perror(retval,error,PAPI_MAX_STR_LEN);
		throw std::runtime_error(error);
	}
}

class Profile {
public:
	static const int max_events=5; //PAPI_MAX_HWCTRS;
	static const int static_default_eventlist[];
	static int const * default_eventlist;
	static int  default_events;
	typedef long long values_t[max_events];
private:
	int 		eventset;
	int 		events;
	int			event[max_events];
	long long	usec_start;
public:
	// Called once to initialize the profile system.
	static void init() {
		if(debug::TRACE) std::cout << "Profile::init>" << std::endl;
		if(PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
			throw std::runtime_error("PAPI_library_init");
		if(PAPI_thread_init(pthread_self) != PAPI_OK)
			throw std::runtime_error("PAPI_thread_init");
	}
	static void init(int* eventlist, int events){
		default_eventlist=eventlist;
		default_events=events;
		init();
	}
	Profile(const int* eventlist=default_eventlist, int events=0)
		: eventset(PAPI_NULL), events(0), usec_start(0) {
		if(eventlist==default_eventlist)
			events=default_events;
		papi_ok(PAPI_create_eventset(&eventset));
		for(int i=0;i<events;i++)
			add(eventlist[i]);
	}
	Profile(const Profile& p)
		: eventset(PAPI_NULL), events(p.events), usec_start(0) {
		papi_ok(PAPI_create_eventset(&eventset));
		for(int i=0;i<p.events;i++)
			add(p.event[i]);
	}
	~Profile(){
		papi_ok(PAPI_cleanup_eventset(eventset));
		papi_ok(PAPI_destroy_eventset(&eventset));
	}
public:
	void add(int event){
		papi_ok(PAPI_add_event(eventset,event));
		this->event[events++]=event;
	}
	void start(){
		// Start the counters from zero
		papi_ok(!(usec_start=PAPI_get_virt_usec()));
		if(events)
			papi_ok(PAPI_start(eventset));
	}
	long long stop(values_t& value){
		long long usec_stop;
		papi_ok(!(usec_stop=PAPI_get_virt_usec()));
		// stops the counters (reset to zero) adding the result to value.
		if(events){
			long long _value[events];
			papi_ok(PAPI_stop(eventset,_value)!=PAPI_OK);
			for(int i=0;i<events;i++)
				value[i]+=_value[i];
		}
		return (usec_stop-usec_start);
	}
	long long mem(){
		PAPI_dmem_info_t dmem;
		papi_ok(PAPI_get_dmem_info(&dmem));
		return dmem.size;
	}
	void print(const values_t& value) const {
		std::cout << "Profile.print>";
		char event_string[PAPI_MAX_STR_LEN];
		for(int i=0;i<events;i++){
			PAPI_event_code_to_name(event[i],event_string);
			std::cout << " " << event_string << ": " << value[i];
		}
		std::cout << std::endl;
	}
	/***
	 * Get the value of a specific event
	 * @param event event code
	 * @param values Values array
	 * @return return value of event code
	 */
	long long get(const int event,const Profile::values_t& value) const {
		for(int i=0;i<events;i++)
			if(event==this->event[i])
				return value[i];
		throw std::runtime_error("Event not found");
	}
};

class Timer {
private:
	long long	usec_start;
	long long	usec_stop;
	long long	usec_value;
	long 		count;
public:
	Timer() : usec_value(0), count(0) {};
	~Timer() {}
public:
	void start(){
		// Start the counters from zero
		count++;
		papi_ok(!(usec_start=PAPI_get_virt_usec()));
	}
	void stop(){
		papi_ok(!(usec_stop=PAPI_get_virt_usec()));
		usec_value+=(usec_stop-usec_start);
	}
	void reset(){
		usec_value=0;
	}
	inline long long usec() const {
		return usec_value;
	}
	inline long long interval() const {
		return usec_stop-usec_start;
	}
	long long mem(){
		PAPI_dmem_info_t dmem;
		papi_ok(PAPI_get_dmem_info(&dmem));
		return dmem.size;
	}
	void join(const Timer& p){
		count+=p.count;
		usec_value+=p.usec_value;
	}
	void print() const {
		std::cout << "Timer.print>";
		std::cout << " msec:" << (usec_value)/1000.0;
		std::cout << " count:" << count;
		std::cout << std::endl;
	}
private: // Hidden default methods
	Timer(const Timer& p);
};

class Profilers {
private:
	typedef tbb::enumerable_thread_specific<Profile,tbb::cache_aligned_allocator<Profile>,tbb::ets_key_per_instance> ProfileLocal;
	ProfileLocal profilers;
public:
	static Profilers* global;
public:
	Profilers(){
		if(debug::TRACE) cout << "Profilers>" << endl;
	}
	~Profilers(){
		if(debug::TRACE) cout << "Profilers~>" << endl;
	}
	inline Profile& profile(){
		return profilers.local();
	}
};


}}//mtim::profile

#endif//_mtim_profile_papi_hh_
