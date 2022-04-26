#pragma once

#include "iopipe.hh"
#include <vector>
#include <string>



class SubProcess {

    IOPipe _stdin;

    IOPipe _stdout;

    IOPipe _stderr;

    std::string _cwd;

    std::vector <std::string> _args;

    std::string _cmd;

    /// The pid of the child process
    int _pid;
	    
public:

    /**
     * Create a sub process but does not launch it
     * @params: 
     *   - cmd: the command to run (bash)
     *   - args: the list of argument of the cmd
     *   - cwd: the current path of the command
     */
    SubProcess (const std::string & cmd, const std::vector <std::string> & args, const std::string & cwd);

    /**
     * ================================================================================
     * ================================================================================
     * =========================           RUNNING            =========================
     * ================================================================================
     * ================================================================================
     */
	    
    /**
     * Start the sub process for running in the background
     */
    void start ();
	    
    /**
     * Wait the completion of the sub process
     * @returns: the return code of the process
     */
    int wait ();


	    
    /**
     * ================================================================================
     * ================================================================================
     * =========================           GETTERS            =========================
     * ================================================================================
     * ================================================================================
     */

    /**
     * @returns: the input stream of the process
     */
    OPipe & stdin ();

    /**
     * @returns: the output stream of the sub process
     */
    IPipe & stdout ();

    /**
     * @returns: The error stream of the sub process
     */
    IPipe & stderr ();	    

	    
    /**
     * Close all the pipes
     */
    ~SubProcess ();
	    
private :

    void run ();

    void child ();
	    
};	
	
