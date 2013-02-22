// Bellman-Ford Multi-Core test platform
// Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// Data structures

#ifndef _mtim_bfmc_bf_hh_
#define _mtim_bfmc_bf_hh_

namespace mtim {
  namespace bfmc {

    // Global RO configuration

	// Network configuration followed by nodes.
    struct network {
    	int nodes;
    	int arcs;
    	int source;
    };

    // node is followed by output then input arc structures.
    struct node {
      int node;
      int output_arcs; // A
      int input_arcs;  // B
    };

    // read output arc first then input.
    struct arc {
    	int peer;
    	int cost;
    };

}}//mtim::bfmc

#endif//_mtim_bfmc_bf_hh_
