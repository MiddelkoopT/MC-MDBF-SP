// Common Thread Platform
// Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#include <pthread.h>
#include "thread.hh"

using namespace std;
using namespace mtim;

// pthread_create public stub
extern "C"
void* Thread::run(void* arg) {
  Thread* thread=static_cast<Thread*>(arg);
  return (*thread->method)(thread->object);
}

// pthread id
long unsigned int (*pthread_thread_id)()=pthread_self;

// Thread::id method
long unsigned int (*Thread::id)()=pthread_thread_id;

// Static initializers
int Thread::threads=0;
pthread_barrier_t Thread::_barrier;
