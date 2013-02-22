// Performance Profile Library
// Copyright (c) 2008, 2009 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// WARNING: Turn off frequency scaling.

#include <iostream>
#include <stdexcept>
#include <papi.h>

#include "profile.hh"

using namespace std;
using namespace mtim::profile;

// Defaults (PAPI_TOT_CYC, PAPI_TOT_INS, PAPI_L2_TCM, PAPI_L2_TCA)
const int Profile::static_default_eventlist[]={
		PAPI_TOT_CYC, PAPI_TOT_INS,
//		PAPI_RES_STL,
};

// Configurable Defaults
int const * Profile::default_eventlist=Profile::static_default_eventlist;
int Profile::default_events=int(sizeof(Profile::static_default_eventlist)/sizeof(int));

// Static allocation
Profilers* Profilers::global=NULL;
