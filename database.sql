-- Copywrite 2009, 2010 Timothy Middelkoop.  All Rights Reserved.  Public Domain.
-- sqlite3 experiments.sqlite < database.sql
-- sqlite3 experiments.sqlite .dump

-- DROP TABLE Experiments;
CREATE TABLE Experiments (
    eid INTEGER PRIMARY KEY AUTOINCREMENT,
    problem TEXT,
    threads INTEGER,
    chunk INTEGER
);

-- DROP TABLE Options;
CREATE TABLE Options (
	eid INTEGER,
	option TEXT,
	value REAL
);

