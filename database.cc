// PerfTools Database and Statistics.
// Copyright (c) 2008-2010 Timothy Middelkoop. All rights reserved.
// Licensed under the GPL 2.0 or later.

#include <string>
#include <iostream>
#include <pthread.h>
#include <sqlite3.h>

#include "database.hh"
#include "profile.hh"

using namespace std;
using namespace mtim::database;
using namespace mtim::database::debug;

// Database

Database::Database(string filename)
{
	open(filename.c_str());
}

Database::~Database()
{
	if(DEBUG) cout << "~Database" << endl;
	sqlite3_close(this->db);
}

sqlite3* Database::open(string filename)
{
   sqlite3_open(filename.c_str(), &this->db);
   return db;
}

bool Database::execute(string sql){
	sqlite3_stmt* stmt;
	int result;
	sqlite3_prepare_v2(db,sql.c_str(),sql.length(),&stmt,NULL);
	result=sqlite3_step(stmt);
	if(result==SQLITE_OK)
		return true;
	if(DEBUG) sqlite3_errmsg(db);
	return false;
}

// Statements

Statement::Statement(sqlite3 *db, string sql)
: db(db), bind_index(0), result_index(0)
{
	sqlite3_prepare_v2(db,sql.c_str(),sql.length(),&this->stmt,NULL);
}

Statement::~Statement()
{
	if(DEBUG) cout << "~Statement" << endl;
	sqlite3_finalize(this->stmt);
}

bool Statement::bind(string value, int parameter)
{
	int rv;
	if(parameter==0)
		parameter=++this->bind_index;
	rv=sqlite3_bind_text(this->stmt, parameter, value.c_str(), value.length(), SQLITE_TRANSIENT);
	if(rv==SQLITE_OK)
		return true;
	if(DEBUG) sqlite3_errmsg(db);
	return false;
}

bool Statement::bind(int value, int parameter)
{
	int rv;
	if(parameter==0)
		parameter=++this->bind_index;
	rv=sqlite3_bind_int(this->stmt, parameter, value);
	if(rv==SQLITE_OK)
		return true;
	if(DEBUG) sqlite3_errmsg(db);
	return false;
}

bool Statement::bind(long value, int parameter)
{
	int rv;
	if(parameter==0)
		parameter=++this->bind_index;
	rv=sqlite3_bind_int64(this->stmt, parameter, value);
	if(rv==SQLITE_OK)
		return true;
	if(DEBUG) sqlite3_errmsg(db);
	return false;
}

bool Statement::bind(double value, int parameter)
{
	int rv;
	if(parameter==0)
		parameter=++this->bind_index;
	rv=sqlite3_bind_double(this->stmt, parameter, value);
	if(rv==SQLITE_OK)
		return true;
	if(DEBUG) sqlite3_errmsg(db);
	return false;
}

bool Statement::fetch()
{
	int rv;
	rv=sqlite3_step(this->stmt);
	if(rv==SQLITE_ROW)
		return true;
	if(DEBUG) sqlite3_errmsg(db);
	return false;
}

Result Statement::get(int column)
{
	if(column==0)
		column=this->result_index++;
	return Result(this->stmt,column);
}

// Results

Result::operator string()
{
	const unsigned char *text;
	text=sqlite3_column_text(stmt,column);
	return string((const char*)text);
}

Result::operator int()
{
	int value;
	value=sqlite3_column_int(stmt,column);
	return value;
}

//
// Statistics (complete Database implementation)
//

Statistic* Statistic::global=NULL;

void Statistic::init(int eid,sqlite3* db)
{
	this->db=db;
	pthread_mutex_init(&lock,NULL);

	sqlite3_exec(db,"CREATE TABLE Statistics (stat TEXT, value REAL, id INTEGER, ref INTEGER, run INTEGER);",NULL,NULL,NULL); // ignore error
	sqlite3_exec(db,"CREATE TABLE Runs (run INTEGER PRIMARY KEY, eid INTEGER, start TEXT, stop TEXT, hash TEXT);",NULL,NULL,NULL);

	// Update run.
	{
		sqlite3_stmt* stmt;
		static const string sql("INSERT INTO Runs (eid,start) VALUES (?,DATETIME('NOW'));");
		sqlite3_prepare_v2(db,sql.c_str(),sql.length(),&stmt,NULL);
		sqlite3_bind_int64(stmt,1,eid);
		sqlite3_step(stmt);
		run=sqlite3_last_insert_rowid(db);
		sqlite3_finalize(stmt);

	}

	// Transaction incase of failure
	sqlite3_exec(db, "BEGIN;",NULL,NULL,NULL);

	// Stat prepared statement. statement
	static const string sql("INSERT INTO Statistics (stat,value,id,ref,run) VALUES (?,?,?,?,?);");
	sqlite3_prepare_v2(db,sql.c_str(),sql.length(),&this->stmt,NULL);

	SHA1_Init(&sha);

	cout << "Statistic> " << run << endl;

}

Statistic::~Statistic()
{
	sqlite3_finalize(stmt);

	// Generate hash
	char hash_text[41];
	SHA1_Final(hash_value,&sha);
	for(int i=0;i<20;i++){
		hash_text[i*2]  ="0123456789ABCDEF"[hash_value[i] >> 4];
		hash_text[i*2+1]="0123456789ABCDEF"[hash_value[i] & 0x0F];
	}
	hash_text[40]=0x00;

	{
		sqlite3_stmt* stmt;
		static const string sql("UPDATE Runs SET stop=DATETIME('NOW'), hash=? WHERE run=?;");
		sqlite3_prepare_v2(db,sql.c_str(),sql.length(),&stmt,NULL);
		sqlite3_bind_text(stmt,1,hash_text,sizeof(hash_text),SQLITE_TRANSIENT);
		sqlite3_bind_int64(stmt,2,run);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	// Close out system.
	sqlite3_exec(db, "END;",NULL,NULL,NULL);
	sqlite3_close(db);
	pthread_mutex_destroy(&lock);

	cout << "Statistic~> " << run << " " << hash_text << endl;
}


/**
 * Record.  Not thread safe with store(); call before next() if using ref=0;
 */
void Statistic::record()
{
	for(save_iter stat=save.begin();stat!=save.end();stat++){
		record(stat->stat,stat->value,stat->id,stat->ref);
	}
	save.clear();
}


int Statistic::record(const string& stat, double value, int id, int ref)
{
	pthread_mutex_lock(&lock);
	int rowid=-1;
	sqlite3_bind_text(stmt,1,stat.c_str(),stat.length(),SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt,2,value);
	sqlite3_bind_int64(stmt,3,id);
	sqlite3_bind_int64(stmt,4,ref?ref:this->ref);
	sqlite3_bind_int64(stmt,5,run);
	sqlite3_step(stmt);
	rowid=sqlite3_last_insert_rowid(db);
	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);
	pthread_mutex_unlock(&lock);
	return rowid;
}

