// Bellman-Ford Multi-Core test platform
// Copyright © 2008,2009 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

// Distributed Bellman-Ford

#include <iostream>
#include <fstream>
#include <limits>

#include "bf.hh"

#include "experiment.hh"
using namespace mtim::experiment;

using namespace std;
using namespace mtim;

static const int INFO=1;
static const int DEBUG=1;
static const int TRACE=0;
static const int CHECK=0;

static const int MAXMEM=2000000LL;
static const int INFINITY=std::numeric_limits<int>::max();

// inline helper functions
inline int print_infinity(int x){
	return (x==INFINITY?0:x);
}

// Classes
struct nodej {
	int j;
	int pred;
	int D;

	// input nodes
	struct nodei* B;
	int Bn;

	// output nodes
	struct nodek* A;
	int An;

};

struct nodei { // B (input)
	int i;
	int c;
	int D;
};

struct nodek { // A (output)
	int k;
	int b; // index of D(i) in B
};

void dump(nodej n[],int N){
	cout << endl;
	for(int j=1;j<=N;j++){
		cout << "Node: " << j << endl;
		cout << " D:" << n[j].D << endl;
		for(int b=0;b<n[j].Bn;b++){
			cout << "  D(" << n[j].B[b].i << ") " << n[j].B[b].D << endl;
		}
	}
	cout << "   D:";
	for(int j=1;j<=N;j++){
		cout << " " << print_infinity(n[j].D);
	}
	cout << endl;

	cout << "pred:";
	for(int j=1;j<=N;j++){
		cout << " " << n[j].pred;
	}
	cout << endl;
}


int main(int argc, char** argv){

	cout << "bf-dist.cc" << endl;

	Experiment::init();
	Experiment experiment(argc,argv);
	mtim::profile::Timer ps;

	ifstream file;
	string problem="../data/" + experiment.problem;
	file.open(problem.c_str());
	if(file.is_open()==false){
		perror("Unable to open file");
		return 1;
	}

	cout << "Read " << experiment.problem << endl;
	bfmc::network network;
	file.read((char*)&network,sizeof(network));
	cout << "nodes,arcs: " << network.nodes << "," << network.arcs << " ";
	cout << "source: " << network.source << endl;

	struct nodej n[network.nodes+1]; // node j
	int s=network.source;
	int N=network.nodes;
	int count=1;

	for(int j=1;j<=network.nodes;j++){
		// Node
		bfmc::node node;
		file.read((char*)&node,sizeof(node));

		n[j].D=(j==s?0:INFINITY);
		n[j].pred=0;

		if(TRACE)
			cout <<"\n" << node.node << " " << node.output_arcs << " " << node.input_arcs << "\n";

		// Output arcs
		n[j].A=new nodek[node.output_arcs];
		n[j].An=node.output_arcs;
		for(int a=0;a<node.output_arcs;a++){
			bfmc::arc arc;
			file.read((char*)&arc,sizeof(arc));
			if(TRACE)
				cout << "A (" << node.node  << "->" << arc.peer << "):" << arc.cost << endl;
			n[j].A[a].k=arc.peer;
			n[j].A[a].b=0;
		}

		// Input arcs
		n[j].B=new nodei[node.input_arcs];
		n[j].Bn=node.input_arcs;
		for(int b=0;b<node.input_arcs;b++){
			bfmc::arc arc;
			file.read((char*)&arc,sizeof(arc));
			if(TRACE)
				cout << "B (" << arc.peer << "->" << node.node << "):" << arc.cost << endl;
			n[j].B[b].i=arc.peer;
			n[j].B[b].c=arc.cost;
		}

	}

	file.close();

	// Link up A[a].b with B[b]
	for(int j=1;j<=network.nodes;j++){
		for(int a=0;a<n[j].An;a++){
			int k=n[j].A[a].k;
			for(int b=0;b<n[k].Bn;b++){
				if(j==n[k].B[b].i){
					n[j].A[a].b=b;
				}
			}
		}

	}


	cout << endl << "Solve " << endl;
	ps.start();

	int steps=0;
	while(count>0){
		count=0;

		// Communicate
		for(int j=1;j<=N;j++){
			for(int a=0;a<n[j].An;a++){
				int k=n[j].A[a].k;
				int b=n[j].A[a].b;
				n[k].B[b].D=n[j].D;
			}
		}

		// Compute labels
		for(int j=1;j<=N;j++){
			if(j==s) continue;
			nodej& nj=n[j];
			int Dp=INFINITY;
			int pred=0;
			for(int b=0;b<nj.Bn;b++){
				nodei& ni=nj.B[b]; // node i in B(j)
				if(ni.D < Dp - ni.c){
					Dp=ni.D + ni.c;
					pred=ni.i;
				}
			}
			if(Dp<nj.D){
				nj.D=Dp;
				nj.pred=pred;
				count++;
			}
		}

		steps++;

	}

	ps.stop();
	ps.print();

	cout << "Solution " << steps << " " << endl;
	for(int j=1;j<=N;j++){
		experiment.stat.hash(n[j].D);
	}

	experiment.stat.record("rtime",ps.usec(),0,-1);
	experiment.stat.record("steps",steps,0,-1);

	cout << "bf-dist.cc done." << endl;
	return 0;

}
