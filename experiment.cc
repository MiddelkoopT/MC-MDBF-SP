// PerfTools Experimental Control
// Copyright (c) 2008-2010 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#include <string>
#include <iostream>

#include "database.hh"
#include "profile.hh"
#include "probe.hh"
#include "experiment.hh"

using namespace std;
using namespace mtim::probe;
using namespace mtim::database;
using namespace mtim::experiment;
using namespace mtim::experiment::debug;

// Init managed subsystems
void Experiment::init(){
	Profile::init();
}

Experiment::Experiment(int argc, char** argv)
	: eid(0), problem("USA-road-d.NY.gr.bin"), threads(2), chunk(5000),
	  usec_start(0), usec_stop(0), usec_value(0)
{
	// Options
	GetOpt::GetOpt_pp options(argc, argv);

	string database("experiments.sqlite");

	options >> GetOpt::Option('d',database);
	options >> GetOpt::Option('e',eid);
	options >> GetOpt::Option('p',problem);
	options >> GetOpt::Option('t',threads);
	options >> GetOpt::Option('c',chunk);

	// Database tools.
	sqlite3* _db=db.open(database);

	// Experiment Database
	if(eid){
		Statement stmt=db.prepare("SELECT problem, threads, chunk FROM Experiments WHERE eid=?");
		stmt.bind(eid);
		stmt.fetch();
		problem=(string)stmt.get();
		threads=stmt.get();
		chunk=stmt.get();
	}

	if(DEBUG) cout << "Experiment> " << database
			<< " e:" << eid << " p:" << problem
			<< " t:" << threads << " c:" << chunk << endl;

	// Statistics
	stat.init(eid,_db); // Shared database.
	run=stat.getRun();

	// Register global instances
	Statistic::global=&stat;
//	Probes::global=&probes;
	Profilers::global=&profilers;

	start();

}

Experiment::~Experiment()
{
	stop();
	record("wtime",wtime());
	if(DEBUG) cout << "Experiment~> " << wtime()/1000000.0 << endl;
}

double Experiment::getOption(const char* option,double value)
{
	if(eid==0)
		return value;
	Statement stmt=db.prepare("SELECT value FROM Options WHERE eid=? and option=?");
	stmt.bind(eid);
	stmt.bind(option);
	stmt.fetch();
	value=(double)stmt.get();
	if(DEBUG) cout << "Experiment.getOption> " << option << " " << value << endl;
	return value;
}
// Global static log lock
Log::Mutex Log::mutex=Log::Mutex();


