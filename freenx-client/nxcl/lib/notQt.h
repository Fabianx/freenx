/* -*-c++-*- */
/***************************************************************************
           notQt.h: A set of Qt like functionality, especially related
                       to the starting of processes.
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

/*!
 * \file notQt.h Simple re-implementations of some Qt-like
 * functionality.  In particular, there's a QProcess-like (though much
 * simplified) class, a QTemporaryFile like class and a couple of the
 * methods that you get with QString.
 */

#ifndef _NOTQT_H_
#define _NOTQT_H_

#include <list>
#include <vector>
#include <string>
#include <fstream>
extern "C" {
#include <sys/poll.h>
}
#define NOTQTPROCESS_MAIN_APP 0
#define NOTQTPROCESS_FAILURE -1

// Possible errors to be generated
#define NOTQPROCNOERROR       0
#define NOTQPROCFAILEDTOSTART 1
#define NOTQPROCCRASHED       2
#define NOTQPROCTIMEDOUT      3
#define NOTQPROCWRITEERR      4
#define NOTQPROCREADERR       5
#define NOTQPROCUNKNOWN       6

using namespace std;

#ifdef DEBUG
extern ofstream debugLogFile;
# define dbgln(msg)  debugLogFile << __FUNCTION__ << ": " << msg << endl;
# define dbglln(msg) debugLogFile << __PRETTY_FUNCTION__ << ": " << msg << endl;
# define dbg(msg)    debugLogFile << msg;
#else
# define dbgln(msg)
# define dbglln(msg)
# define dbg(msg)
#endif

namespace nxcl {

	/*!
	 * A set of virtual callbacks. These should be derived in the
	 * client code. They're called by notQProcess via the
	 * notQProcessCallbacks* callbacks member variable.
	 */
	class notQProcessCallbacks
	{
	public:
		notQProcessCallbacks() {}
		virtual ~notQProcessCallbacks() {}
		virtual void startedSignal (string) {}
		virtual void errorSignal (int) {}
		virtual void processFinishedSignal (string) {}
		virtual void readyReadStandardOutputSignal (void) {}
		virtual void readyReadStandardErrorSignal (void) {}
	};

	/*!
	 * notQProcess is a simple replacement for the Qt class QProcess.
	 */
	class notQProcess
	{
	public:
		notQProcess();
		~notQProcess();
		/*!
		 * Write \arg input to the stdin of the process.
		 */
		void writeIn (string& input);
		/*!
		 * fork and exec the process.
		 */
		int start (const string& program, const list<string>& args);
		/*!
		 * Send a TERM signal to the process.
		 */
		void terminate (void);

		/*!
		 * poll to see if there is data on stderr or stdout
		 * and to see if the process has exited.
		 *
		 * This must be called on a scheduled basis. It checks
		 * for any stdout/stderr data and also checks whether
		 * the process is still running.
		 */
		void probeProcess (void);

		/*!
		 * Accessors
		 */
		//@{
		pid_t getPid (void) { return this->pid; }
		int getError (void) { return this->error; }
		void setError (int e) { this->error = e; }
		
		int getParentFD() 
		{ 
			this->parentFD = this->parentToChild[1];
			close(this->childToParent[0]);

			// Create new pipes
			pipe(this->parentToChild);
			pipe(this->childToParent);

			return this->parentFD;
		}

		/*!
		 * Setter for the callbacks.
		 */
		void setCallbacks (notQProcessCallbacks * cb) { this->callbacks = cb; }
		//@}

		/*! 
		 * Slots
		 */
		//@{
		string readAllStandardOutput (void);
		string readAllStandardError (void);
		/*!
		 * Wait for the process to get itself going. Do this
		 * by looking at pid.  If no pid after a while,
		 * return false.
		 */
		bool waitForStarted (void);
		//@}
	private:
		/*!
		 * The name of the program to execute
		 */
		string progName;
		/*!
		 * The environment and arguments of the program to execute
		 */
		list<string> environment;
		/*!
		 * Holds a notQProcess error, defined above. NOTQPROCNOERROR, etc.
		 */
		int error;
		/*!
		 * Process ID of the program
		 */
		pid_t pid;
		/*!
		 * Set to true if the fact that the program has been
		 * started has been signalled using the callback
		 * callbacks->startedSignal
		 */
		bool signalledStart;
		/*!
		 * stdin parent to child
		 */
		int parentToChild[2];
		/*!
		 * stdout child to parent
		 */
		int childToParent[2];
		/*!
		 * stderr child to parent
		 */
		int childErrToParent[2];
		/*!
		 * Used in the poll() call in probeProcess()
		 */
		struct pollfd * p;
		/*!
		 * Pointer to a callback object
		 */
		notQProcessCallbacks * callbacks;

		/*! 
		 * old parent FD for comm with child
		 */
		int parentFD;
	};

	/*!
	 * A simple replacement for the Qt Class QTemporaryFile.
	 */
	class notQTemporaryFile
	{
	public:
		notQTemporaryFile();
		~notQTemporaryFile();
		/*!
		 * Open a file with a (not really) random name. The
		 * filename will be /tmp/notQtXXXXXX where XXXXXX will
		 * be the time in seconds since the unix epoch.
		 */
		void open (void);
		/*!
		 * Write \arg input to the temporary file.
		 */
		void write (string input);
		/*!
		 * Close the temporary file's stream.
		 */
		void close (void);
		/*!
		 * A getter for the file name of the temporary file
		 */
		string fileName (void);
		/*!
		 * Remove the temporary file
		 */
		void remove (void);

	private:
		/*!
		 * The file name of the temporary file
		 */
		string theFileName;
		/*!
		 * The file stream for the temporary file
		 */
		fstream f;
	};

	/*!
	 * A few useful utility functions.
	 */
	class notQtUtilities
	{
	public:
		notQtUtilities();
		~notQtUtilities();

		/*! The same (more or less) as Qt QString::simplified */
		static string simplify (string& input);
		/*!
		 * Split a string 'line' based on token, placing the portions in the vector rtn
		 */
		static void splitString (string& line, char token, vector<string>& rtn);
		/*!
		 * Split a string 'line' based on token, placing the portions in the list rtn
		 */
		static void splitString (string& line, char token, list<string>& rtn);
		/*!
		 * Run through input and replace any DOS newlines with unix newlines.
		 */
		static int ensureUnixNewlines (std::string& input);
	};

} // namespace
#endif
