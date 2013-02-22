// PerfTools Database and Statistics.
// Copyright (c) 2008-2010 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#ifndef _mtim_database_hh_
#define _mtim_database_hh_

#include <string>
#include <sqlite3.h>

#include "getopt_pp.hh"
#include <tbb/concurrent_vector.h>
#include <openssl/sha.h>

using namespace std;

namespace mtim {
namespace database {

namespace debug {
	const int DEBUG=false;
}

class Result {
private:
	sqlite3_stmt* stmt;
	int column;
public:
	Result(sqlite3_stmt* stmt, int column) : stmt(stmt), column(column) {};
	operator int ();
	operator string ();

};

class Statement {
private:
	sqlite3* db;
	sqlite3_stmt* stmt;
	int bind_index;
	int result_index;
public:
	Statement(sqlite3 *db, string sql);
	~Statement();
	bool bind(string value,int parameter=0);
	bool bind(int value,int parameter=0);
	bool bind(long value,int parameter=0);
	bool bind(double value, int parameter=0);
	bool fetch();
	bool execute() { return this->fetch(); };
	Result get(int column=0);
};

class Database {
private:
	sqlite3* db;
public:
	Database() : db(NULL) {};
	Database(string filename);
	sqlite3* open(string filename);
	~Database();
	Statement prepare(string sql) { return Statement(this->db, sql); };
	bool execute(string sql);
};

class Stat {
public:
	const char* stat;
	double value;
	int id;
	int ref;
public:
	Stat(const char* stat,double value, int id=0,int ref=0) : stat(stat), value(value), id(id), ref(ref) {};
};

class Statistic {
public:
	static Statistic* global;
private:
	sqlite3* db;
	sqlite3_stmt* stmt;
	pthread_mutex_t lock;

	int eid;
	int run;
	int ref;

	tbb::concurrent_vector<Stat> save;
	typedef tbb::concurrent_vector<Stat>::iterator save_iter;

	SHA_CTX sha;
	unsigned char hash_value[20];

public:
	Statistic() : db(NULL), eid(eid), run(0), ref(0) {};
	~Statistic();
	void init(int eid, sqlite3* db=NULL);
	int getRun(){ return run;};
	int next(){ return ++ref;};
	inline void store(const char* stat,double value, int id,int ref) {save.push_back(Stat(stat,value,id,ref));};
	inline void store(const char* stat,double value, int id=0) {save.push_back(Stat(stat,value,id,ref));};
	void record();
	int record(const string& stat,double value,int id=0,int ref=0);
	template <typename T> void hash(T value){ SHA1_Update(&sha,&value,sizeof(T)); }
	template <typename T> void hash(T* value){ SHA1_Update(&sha,value,sizeof(T)); }
private:
};

}}//mtim::database
#endif
