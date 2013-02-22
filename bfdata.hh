// Bellman-Ford Multi-Core test platform
// Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// Data structures

#ifndef _mtim_bfdata_hh_
#define _mtim_bfdata_hh_

#include <iostream>
#include <limits>

using namespace std;

namespace mtim {
namespace bfmc {


// in (B)
struct nodei {
  int Di;
  int i;  // nodei index
  int c;  // cost of nodei
};

// out (A)
struct nodek {
  int* Dk;
  int  k;
//  int  c;
};

struct nodej {
	// communication
	int j;
	int Dj;

	int An;
	nodek* A;
	int Bn;
	nodei* B;

	// internal
	int predj;
};


class Data{
public:
  nodej* node;
  int nodes;
public:
  Data(const string filename) : node(NULL) { this->read(filename);this->link(); }
  void read(const string filename);
  void write(const string filename, const bool d=false);
  void test();
private:
  void link();
};


}//bfmc
}//mtim


#endif//_mtim_bfdata_hh_
