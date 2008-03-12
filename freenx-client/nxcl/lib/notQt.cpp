/***************************************************************************
          notQt.cpp: A set of Qt like functionality, especially related
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

#include <iostream>
#include <sstream>
extern "C" {
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/poll.h>	
#include <sys/socket.h>	
#include <signal.h>
}

#include "../config.h"
#include "notQt.h"

using namespace std;
using namespace nxcl;

/*!
 * Implementation of the notQProcess class
 */
//@{

// Used when dealing with pipes
#define READING_END 0
#define WRITING_END 1
#define STDIN  0
#define STDOUT 1
#define STDERR 2

// Constructor
notQProcess::notQProcess () :
    progName("unknown"),
    error (NOTQPROCNOERROR),
    pid(0),
    signalledStart(false),
    parentFD(-1)
{
    // Set up the polling structs
    this->p = static_cast<struct pollfd*>(malloc (2*sizeof (struct pollfd)));	
}

// Destructor
notQProcess::~notQProcess ()
{
    free (this->p);
    if (parentFD != -1)
    {
    	close(parentFD);
	parentFD=-1;
    }
    // FIXME: this should be closed here
   // close (parentToChild[READING_END]);
   // close (childToParent[WRITING_END]);
   // close (childErrToParent[WRITING_END]);
}

    void
notQProcess::writeIn (string& input)
{
    write (this->parentToChild[WRITING_END], input.c_str(), input.size());
}

// fork and exec a new process using execv, which takes stdin via a
// fifo and returns output also via a fifo.
    int
notQProcess::start (const string& program, const list<string>& args)
{
    char** argarray;
    list<string> myargs = args;
    list<string>::iterator i;
    unsigned int j = 0;
    int theError;

    // NB: The first item in the args list should be the program name.
    this->progName = program;

#ifdef NXCL_USE_NXSSH
    // Set up our pipes
    if (pipe(parentToChild) == -1 || pipe(childToParent) == -1 || pipe(childErrToParent) == -1) {
        return NOTQTPROCESS_FAILURE;
    }
#else /* We need a socketpair for that to work */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, parentToChild) == -1 || pipe(childErrToParent) == -1)
        return NOTQTPROCESS_FAILURE;
    
    childToParent[READING_END]=dup(parentToChild[WRITING_END]);
    childToParent[WRITING_END]=dup(parentToChild[READING_END]);
#endif

    this->pid = fork();

    switch (this->pid) {
        case -1:
            return NOTQTPROCESS_FAILURE;
        case 0:
            // This is the CHILD process

            // Close unwanted ends of the pipes
            close (parentToChild[WRITING_END]);
            close (childToParent[READING_END]);
            close (childErrToParent[READING_END]);

            // Now all we have to do is make the writing file
            // descriptors 0,1 or 2 and they will be used instead
            // of stdout, stderr and stdin.
            if ((dup2 (parentToChild[READING_END], STDIN)) == -1  ||
                    (dup2 (childToParent[WRITING_END], STDOUT)) == -1 || 
                    (dup2 (childErrToParent[WRITING_END], STDERR)) == -1) {
                theError = errno;
                cout << "ERROR! Couldn't get access to stdin/out/err! errno was " << theError << endl;
                return NOTQTPROCESS_FAILURE;	
            }

            // Allocate memory for the program arguments
            // 1+ to allow space for NULL terminating pointer
            argarray = static_cast<char**>(malloc ((1+args.size()) * sizeof (char*))); 
            for (i=myargs.begin(); i!=myargs.end(); i++) {
                argarray[j] = static_cast<char*>(malloc ( (1+(*i).size()) * sizeof (char) ));
                snprintf (argarray[j++], 1+(*i).size(), "%s", (*i).c_str());
                dbgln(*i);
            }
            argarray[j] = NULL;

            dbgln ("About to execute '" + program + "' with those arguments..");

            execv (program.c_str(), argarray);

            // If process returns, error occurred
            theError = errno; 
            // This'll get picked up by parseResponse
            cout << "notQProcess error: " << this->pid << " crashed. errno:" << theError << endl;

            // This won't get picked up by the parent process.
            this->error = NOTQPROCCRASHED;

            // In this case, we close the pipes to signal to the parent that we crashed
            close (parentToChild[READING_END]);
            close (childToParent[WRITING_END]);
            close (childErrToParent[WRITING_END]);

            // Child should exit now.
            _exit(-1);

        default:
            // This is the PARENT process

            // Close unwanted ends of the pipes
            close (parentToChild[READING_END]);
            close (childToParent[WRITING_END]);
            close (childErrToParent[WRITING_END]);

            // Write to this->parentToChild[WRITING_END] to write to stdin of the child
            // Read from this->childToParent[READING_END] to read from stdout of child
            // Read from this->childErrToParent[READING_END] to read from stderr of child

            break;
    }
    return NOTQTPROCESS_MAIN_APP;
}


// If no pid after a while, return false.
    bool
notQProcess::waitForStarted (void)
{
    unsigned int i=0;
    while (this->pid == 0 && i<1000) {
        usleep (1000);
        i++;
    }
    if (this->pid>0) {
        dbgln ("The process started!");
        this->callbacks->startedSignal (this->progName);
        this->signalledStart = true;
        return true;
    } else {
        this->error = NOTQPROCFAILEDTOSTART;
        this->callbacks->errorSignal (this->error);
        return false;
    }

}

// Send a TERM signal to the process.
    void
notQProcess::terminate (void)
{
    kill (this->pid, 15); // 15 is TERM
    // Now check if the process has gone and kill it with signal 9 (KILL)
    this->pid = 0;
    this->error = NOTQPROCNOERROR;
    this->signalledStart = false;
    return;
}

// Check on this process
    void
notQProcess::probeProcess (void)
{
    // Has the process started?
    if (!this->signalledStart) {
        if (this->pid > 0) {
            this->callbacks->startedSignal (this->progName);
            this->signalledStart = true;
            dbgln ("notQProcess::probeProcess set signalledStart and signalled the start...");
        }
    }

    // Check for error condition
    if (this->error>0) {
        this->callbacks->errorSignal (this->error);
        dbgln ("have error in probeProcess, returning");
        return;
    }

    if (this->pid == 0) {
        // Not yet started.
        return;
    }

    // Why can't these 4 lines go in contructor?
    this->p[0].fd = this->childToParent[READING_END];
    this->p[0].events = POLLIN | POLLPRI;
    this->p[1].fd = this->childErrToParent[READING_END];
    this->p[1].events = POLLIN | POLLPRI;

    // Poll to determine if data is available
    this->p[0].revents = 0;
    this->p[1].revents = 0;

    poll (this->p, 2, 0);

    if (this->p[0].revents & POLLNVAL || this->p[1].revents & POLLNVAL) {
        dbgln ("notQProcess::probeProcess: pipes closed, process must have crashed");
        this->error = NOTQPROCCRASHED;
        this->callbacks->errorSignal (this->error);
        return;
    }

    if (this->p[0].revents & POLLIN || this->p[0].revents & POLLPRI) {
        this->callbacks->readyReadStandardOutputSignal();
    }
    if (this->p[1].revents & POLLIN || this->p[1].revents & POLLPRI) {
        this->callbacks->readyReadStandardErrorSignal();
    }

    // Is the process running? We check last, so that we get any
    // messages on stdout/stderr that we may wish to process, such
    // as error messages from nxssh key authentication.
    int theError;
    if (this->signalledStart == true) {
        int rtn = 0;
        if ((rtn = waitpid (this->pid, (int *)0, WNOHANG)) == this->pid) {
            this->callbacks->processFinishedSignal (this->progName);
            return;
        } else if (rtn == -1) {
            theError = errno;
            if (theError != 10) { // We ignore errno 10 "no child" as this commonly occurs
                cerr << "waitpid returned errno: " << theError;
            }
        } // else rtn == 0
    }

    return;
}

// Read stdout pipe, without blocking.
    string
notQProcess::readAllStandardOutput (void)
{
    string s;
    int bytes = 0;
    char c;
    struct pollfd p;

    p.fd = this->childToParent[READING_END];
    p.events = POLLIN | POLLPRI;
    // We know we have at least one character to read, so seed revents
    p.revents = POLLIN;
    while (p.revents & POLLIN || p.revents & POLLPRI) {
        // This read of 1 byte should never block
        if ((bytes = read (this->childToParent[READING_END], &c, 1)) == 1) {
            s.append (1, c);
        }
        p.revents = 0;
        poll (&p, 1, 0);
    }
    return s;
}

// Read stderr pipe without blocking
    string
notQProcess::readAllStandardError (void)
{
    string s;
    int bytes = 0;
    char c;
    struct pollfd p;

    p.fd = this->childErrToParent[READING_END];
    p.events = POLLIN | POLLPRI;
    // We know we have at least one character to read, so seed revents
    p.revents = POLLIN;
    while (p.revents & POLLIN || p.revents & POLLPRI) {
        // This read of 1 byte should never block because a poll() call tells us there is data
        if ((bytes = read (this->childErrToParent[READING_END], &c, 1)) == 1) {
            s.append (1, c);
        }
        p.revents = 0;
        poll (&p, 1, 0);
    }
    return s;
}

//@}

/*!
 * Implementation of the notQTemporaryFile class
 */
//@{

// Constructor
notQTemporaryFile::notQTemporaryFile ()
{
}

// Destructor
notQTemporaryFile::~notQTemporaryFile ()
{
    this->close();
}

    void
notQTemporaryFile::open (void)
{
    stringstream fn;
    fn << "/tmp/notQt" << time(NULL);
    this->theFileName = fn.str();
    this->f.open (this->theFileName.c_str(), ios::in|ios::out|ios::trunc);
}

    void
notQTemporaryFile::write (string input)
{
    this->f << input;
}

    void
notQTemporaryFile::close (void)
{
    if (this->f.is_open()) {
        this->f.close();
    }
}

// getter for fileName
    string
notQTemporaryFile::fileName (void)
{
    return this->theFileName;
}

    void
notQTemporaryFile::remove (void)
{
    this->close();
    unlink (this->theFileName.c_str());
}
//@}


/*!
 * Implementation of the notQtUtilities class
 */
//@{

// Constructor
notQtUtilities::notQtUtilities ()
{
}

// Destructor
notQtUtilities::~notQtUtilities ()
{
}

    string
notQtUtilities::simplify (string& input)
{
    string workingString;
    unsigned int i=0, start, end;

    // Find the first non-whitespace character.
    while (input[i] != '\0' && 
            (input[i] == ' '  || input[i] == '\t' || input[i] == '\n' || input[i] == '\r')
            && i<input.size()) {
        i++;
    }
    start=i;

    // Now find the last non-whitespace character.
    i = input.size();
    i--;
    while ((input[i] == ' '  || input[i] == '\t' || input[i] == '\n' || input[i] == '\r')
            && i>0) {
        i--;
    }
    end = ++i;

    // Copy the substring into a working string.
    if (end>start) {
        workingString = input.substr (start, end-start);
    } else {
        return "";
    }

    // Now we replace internal white spaces in workingString with single spaces.
    for (i=workingString.size(); i>1; --i) {
        if ( (workingString[i] == ' '   || workingString[i] == '\t'   
                    || workingString[i] == '\n' || workingString[i] == '\r')
                &&
                (workingString[i-1] == ' ' || workingString[i-1] == '\t'
                 ||  workingString[i-1] == '\n' || workingString[i-1] == '\r') ) {
            // ...then this is a whitespace we can remove
            workingString.erase(i,1);

        } else if ( (workingString[i] == '\t' ||  workingString[i] == '\n'
                    || workingString[i] == '\r') 
                &&
                (workingString[i-1] != ' ' && workingString[i-1] != '\t'
                 &&  workingString[i-1] != '\n' && workingString[i-1] != '\r') ) {
            // ...then this is a non-space whitespace to be replaced
            workingString.replace(i, 1, " ");
        }
    }

    return workingString;
}

// split based on token ' '
    void
notQtUtilities::splitString (string& line, char token, vector<string>& rtn)
{
    rtn.clear();
    unsigned int i=0;
    while (i < (line.size())-1) {
        string tstring;
        while (line[i] && line[i] != token) {
            tstring.push_back(line[i++]);
        }
        rtn.push_back(tstring);
        i++;
    }
    return;
}

    void
notQtUtilities::splitString (string& line, char token, list<string>& rtn)
{
    rtn.clear();
    unsigned int i=0;
    while (i < (line.size())-1) {
        string tstring;
        while (line[i] && line[i] != token) {
            //dbgln ("tstring.push_back line[i] which is '" << line[i] << "'");
            tstring.push_back(line[i++]);
        }
        //dbgln ("rtn.push_back() tstring which is '" + tstring + "'");
        rtn.push_back(tstring);
        i++;
    }
    return;
}

    int
notQtUtilities::ensureUnixNewlines (std::string& input)
{
    int num = 0;

    for (unsigned int i=0; i<input.size(); i++) {
        if (input[i] == '\r') {
            input.erase(i,1);
            num++;
        }
    }

    return num; // The number of \r characters we found in the string.
}
