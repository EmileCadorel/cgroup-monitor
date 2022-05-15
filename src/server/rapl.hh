#pragma once

#include <fstream>
#include <monitor/concurrency/_.hh>
#include <monitor/libvirt/_.hh>

namespace server {


    class RaplReader {

	/// True if the current machine supports rapl
	bool _raplEnabled;

	/// The path of the rapl energy file
	std::filesystem::path _raplPath0;

	/// The path of the rapl energy file
	std::filesystem::path _raplPath1;

	/// The value of rapl last tick
	unsigned long _raplValue0;

	/// The value of rapl last tick
	unsigned long _raplValue1;

	/// The file handle of rapl energy 
	std::ifstream _raplFile0;
	
	/// The file handle of rapl energy 
	std::ifstream _raplFile1;
	
    public:

	RaplReader ();
	
	/**
	 * Read the value of rapl package 0
	 */
	unsigned long readPP0 ();

	/**
	 * Read the value of rapl package 1
	 */
	unsigned long readPP1 ();

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
