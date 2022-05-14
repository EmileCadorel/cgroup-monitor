#pragma once

#include <fstream>
#include <monitor/concurrency/_.hh>
#include <monitor/libvirt/_.hh>

namespace server {


    class RaplReader {

	/// True if the current machine supports rapl
	bool _raplEnabled;

	/// The path of the rapl energy file
	std::filesystem::path _raplPath;

	/// The value of rapl last tick
	unsigned long _raplValue;

	/// The file handle of rapl energy 
	std::ifstream _raplFile;
	
    public:

	RaplReader ();
	
	/**
	 * Read the value of rapl
	 */
	unsigned long read ();

	/**
	 * @returns: true if rapl is readable
	 */
	bool isEnabled () const;

    private:

	/**
	 * Check that rapl is enabled on the machine
	 */
	void checkRaplCapabilities ();
	
    };
    

}
