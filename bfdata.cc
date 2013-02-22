// Bellman-Ford Multi-Core test platform
// Copyright (c) 2008 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.
//
// Data Import

#include <iostream>
#include <fstream>
#include <limits>

#include "bf.hh"
#include "bfdata.hh"

using namespace std;
using namespace mtim;
using namespace mtim::bfmc;

static const int INFO=1;
static const int DEBUG=1;
static const int TRACE=1;
static const int CHECK=0;

static const int INFINITY=std::numeric_limits<int>::max();

void mtim::bfmc::Data::read(const string filename){

	if(TRACE) cout << "Data.read> "
		<< " nodei:" <<sizeof(nodei)
		<< " nodej:" <<sizeof(nodej)
		<< " nodek:" <<sizeof(nodek)
		<< endl;

	bfmc::network network;

	ifstream file;
	file.open(filename.c_str());

	file.read((char*)&network,sizeof(network));
	cout << "Data.read> ";
	cout << "nodes,arcs: " << network.nodes << "," << network.arcs << " ";
	cout << "source: " << network.source << endl;

	nodej* n= new nodej[network.nodes+1]; // node j
	const int s=network.source;
	const int N=network.nodes;

	for(int j=1;j<=N;j++){

		nodej& nj=n[j];

		nj.Dj=(j==s?0:INFINITY);
		nj.predj=0;

		// A then B in pairs.
		bfmc::node node;
		file.read((char*)&node,sizeof(node));
		nj.A=new nodek[node.output_arcs];
		nj.An=node.output_arcs;

		if(TRACE)
			cout <<"\n" << node.node << " " << node.output_arcs << " " << node.input_arcs << "\n";

		for(int a=0;a<node.output_arcs;a++){
			bfmc::arc arc;
			file.read((char*)&arc,sizeof(arc));
			if(TRACE)
				cout << "A (" << node.node  << "->" << arc.peer << "):" << arc.cost << endl;
			nj.A[a].k=arc.peer;
//			nj.A[a].c=arc.cost;
		}

		nj.B=new nodei[node.input_arcs];
		nj.Bn=node.input_arcs;

		for(int b=0;b<node.input_arcs;b++){
			bfmc::arc arc;
			file.read((char*)&arc,sizeof(arc));
			if(TRACE)
				cout << "B (" << arc.peer << "->" << node.node << "):" << arc.cost << endl;
			nj.B[b].i=arc.peer;
			nj.B[b].c=arc.cost;
		}
	}

	file.close();

	this->node=n;
	this->nodes=network.nodes;

}

void mtim::bfmc::Data::write(const string filename,const bool d){
	ofstream file;
	file.open(filename.c_str());

	nodej* n=this->node;
	const int N=this->nodes;

	for(int j=1;j<=N;j++){
		for(int b=0;b<n[j].Bn;b++){
			file << "a " << n[j].B[b].i << " " << j << " " << n[j].B[b].c;
			if(d)
				file << " " << n[j].Dj;
			file << endl;
		}
	}
	file.close();
}

void mtim::bfmc::Data::link(){
	// set &Dk (serial)

	nodej* n=this->node;
	const int N=this->nodes;

	for(int j=1;j<=N;j++){
		n[j].j=j;
		for(int a=0;a<n[j].An;a++){
			int k=n[j].A[a].k;
			for(int b=0;b<n[k].Bn;b++){
				int i=n[k].B[b].i;
				if(i==j){
					int* Di=&n[k].B[b].Di;
					n[j].A[a].Dk=Di;
				}
			}
		}
	}

}

void mtim::bfmc::Data::test(){

  // check communication

  const int N=this->nodes;
  nodej* n=this->node;

  // communicate
//  for(int j=1;j<=N;j++)
//	  for(int a=0;a<n[j].An;a++)
//		  *n[j].A[a].Dk=n[j].Dj;

  // check
  for(int j=1;j<=N;j++)
	  for(int b=0;b<n[j].Bn;b++)
		  if(n[j].B[b].Di!=INFINITY)
			  cout << "** j,Di,i " << j << "," << n[j].B[b].Di << "," << n[j].B[b].i << endl;

}
