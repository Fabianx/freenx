/***************************************************************************
   notQttest.cpp - A test of some of the features in ../lib/notQt.cpp
                             -------------------
    begin                : June 2007
    copyright            : (C) 2007 Embedded Software Foundry Ltd. (U.K.)
                         :     Author: Sebastian James
    email                : seb@esfnet.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <iostream>
#include <string>
#include <list>

#include "notQt.h"

using namespace std;
using namespace nxcl;

ofstream debugLogFile;

notQProcess p2;

void processParseStdout()
{
	string message = p2.readAllStandardOutput();
	cout << "processParseStdout called, message is: " << message << endl;
}
void processParseStderr()
{
	string message = p2.readAllStandardError();	
	cout << "processParseStderr called, message is: " << message << endl;
}

int main()
{

	debugLogFile.open("/tmp/notQttest.log", ios::out|ios::trunc);

	// Test temporary files
	notQTemporaryFile tf;
	tf.open();
	tf.write ("this is a temporary file, jim");
	cout << "tmp filename is " << tf.fileName() << endl;
	tf.close();

	// Test utilities
	string tstring = "\n\n  this \t  is\n a\t\t   test string    ";
	cout << "test string is '" << tstring << "'" << endl;
	cout << "simplify returns '" << notQtUtilities::simplify (tstring) << "'" << endl;

        tstring = "Nowhitespaceatall";
	cout << "test string is '" << tstring << "'" << endl;
	cout << "simplify returns '" << notQtUtilities::simplify (tstring) << "'" << endl;

	tstring = "one two three\tfour";
	vector<string> v;
	notQtUtilities::splitString (tstring, ' ', v);
	cout << "v.size() = " << v.size() << endl;
	for (unsigned int i=0; i<v.size(); i++) {
		cout << "v["<<i<<"]='" << v[i] << "'" << endl;
	}
	/*
	vector<string> v2;
	notQtUtilities::splitString (tstring, 'h', v2);
	cout << "v2.size() = " << v2.size() << endl;
	for (unsigned int i=0; i<v2.size(); i++) {
		cout << "v2["<<i<<"]='" << v2[i] << "'" << endl;
	}
	*/

	// Test processes to read some input
	notQProcess p;
	string program = "/usr/bin/tee";
	list<string> args;       
	// Always push_back the program first.
	args.push_back(program);
	args.push_back("teeout");
	p.start (program, args);
	cout << "p.getPid=" << p.getPid() << endl;
	if (p.waitForStarted() == true) {
		string data = "Some input text";
		p.writeIn (data);

	} else { 
		cout << "not started" << endl;
		return -1;
	}

	// Test process that generates output
	bool finished = false;
//	program = "/usr/bin/eo";
	program = "/usr/bin/tee";
	args.clear();
	// Always push_back the program first.
	args.push_back(program);
	args.push_back("hello");

	// p2 is a global in this test

/* FIXME: This needs to be changed, now that we got rid of boost signals.
	p2.readyReadStandardOutputSignal.connect (&processParseStdout);
	p2.readyReadStandardErrorSignal.connect (&processParseStderr);
*/
	p2.start (program, args);
	cout << "p2.getPid=" << p2.getPid() << endl;
	if (p2.waitForStarted() == true) {
		string output;
		string errout;
		string instring = "data, data";
		while (finished == false) {
			p2.probeProcess();
/* You can get the output without signals:
			output = p2.readAllStandardOutput();
			errout = p2.readAllStandardError();
			if (output.size()>0) {
				cout << program << " generated this output: " << output << endl;
				finished = true;
			}
			if (errout.size()>0) {
				cout << program << " generated this error: " << errout << endl;
				finished = true;
			}
			cout << "sleeping" << endl;
*/	
			usleep (1000000);
			p2.writeIn (instring);
			usleep (1000000);
		}

	} else { 
		cout << "not started" << endl;
		return -1;
	}

	debugLogFile.close();

	return 0;
}
