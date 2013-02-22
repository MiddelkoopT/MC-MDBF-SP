// C++ Thread stub
// Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#ifndef _mtim_thread_hh_
#define _mtim_thread_hh_

#include <iostream>
#include <pthread.h>

namespace mtim {

// run function must be in the form void* method(void);
class Thread {
public:
	static int threads;
private:
	static pthread_barrier_t _barrier;

public:
	static void init(int threads=0) {
		Thread::threads=threads;
		pthread_barrier_init(&_barrier, NULL, threads);
	}
	Thread() : pthread(0), object(0), method(0) {}
	~Thread() { std::cout << "Thread Destroyed" << std::endl; }

	template<class T>
	Thread(T* object) : pthread(0) {create<T,&T::run>(object);}

	// new thread with existing instance of T in "object" started with "method"
	template<class T, void* (T::*TMethod)()>
	void create(void* object) {
		this->object=object;
		this->method=&stub<T,TMethod>;
	}

	// new thread with existing instance started with T::run
	template<class T>
	void create(T* object) {create<T,&T::run>((void*) object);}

	// new thread with new instance started with T::run
	template<class T>
	void create() {create<T,&T::run>(new T());}

	// new thread with new instance of T started with "method"
	template<class T, void* (T::*TMethod)()>
	void create() {create<T,TMethod>(new T());}

	// Thread manipulation
	int run() {return pthread_create(&pthread, NULL, Thread::run, this);}
	int join(){return pthread_join(pthread,NULL);}
	static void barrier(){pthread_barrier_wait(&_barrier);}

	static long unsigned int (*id)();

private:
	pthread_t pthread;
	void* object;
	void* (*method)(void* object);

	// stub pointer to call appropriate method.
	template<class T, void* (T::*TMethod)()>
	static void* stub(void* object) {return (static_cast<T*>(object)->*TMethod)();}

	// pthread callable object that uses a stub created and placed in "method"
    static void* run(void* arg); // pthread_create &run

};


}//mtim

#endif//_mtim_thread_hh_
