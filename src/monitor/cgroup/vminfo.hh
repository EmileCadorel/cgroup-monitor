#pragma once

#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <monitor/concurrency/timer.hh>
#include <fstream>

namespace monitor {       
    namespace cgroup {

	class VMInfo {

	    /// The name of the group
	    std::string _name;

	    /// The path of the group
	    std::filesystem::path _path;

	    /// The consumption in the last tick
	    unsigned long _lastConso;
	    
	    /// The cpu consumption of the group in microsecond
	    unsigned long _conso;

	    /// The cpu capping of the group in microsecond (-1 if uncapped)
	    long _cap; 

	    /// The period of the group in microsecond
	    unsigned long _period;

	    /// The delta time in second between the two last ticks
	    float _delta;

	    /// The timer that compute the frame durations
	    concurrency::timer _t;

	    /// The number of vcpus;
	    unsigned long _vcpus;
	    
	    /// the history of consumption of the VM
	    std::vector <double> _history;

	    /// The maximum number of element in the history
	    unsigned long _maxhistory;

	    /// The slope of the history
	    double _slope;

	    /// The minimal guaranteed frequency of the VM (in Mhz)
	    unsigned long _baseFreq;

	    /// cgroup are v2
	    bool _cgroupV2; 

	    /// The quantity of memory consumption
	    unsigned long _usedMemory;

	    /// The quantity of allocated memory
	    unsigned long _allocatedMemory;
	    
	    /// The history of the memory usage of the VM
	    std::vector <double> _memoryHistory;

	    /// The maximum memory size of the VM
	    unsigned long _maxMemory;
	    
	public:

	    /**
	     * Create a new cgroup from a path
	     * @params: 
	     *  - path: the path of the cgroup in the file system (e.g. /sys/fs/cgroup/my_group)
	     */
	    VMInfo (const std::string & name, const std::filesystem::path & path, unsigned long maxhistory, unsigned long freq, bool v2);	    

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           GETTERS            =========================
	     * ================================================================================
	     * ================================================================================
	     */

	    /**
	     * @returns: the path of the cgroup
	     */
	    const std::filesystem::path & getPath () const;

	    /**
	     * @returns: the maximal number of microseconds that can be consumed by the VM in one second
	     */
	    unsigned long getMaximumConso () const;

	    /**
	     * @returns: the number of microseconds that are consumed by the VM relate to the delta duration of the last tick
	     */
	    unsigned long getAbsoluteConso () const;

	    /**
	     * @returns: the number of microseconds authorized for the VM in the current second
	     */
	    unsigned long getAbsoluteCapping () const;
	    

	    /**
	     * @returns: the percentage consumption of the last tick
	     */
	    float getPercentageConso () const;

	    /**
	     * @returns: the percentage of consumption of the last tick in comparison to the capping
	     */
	    float getRelativePercentConso () const;
	    
	    /**
	     * @returns: the name of the cgroup
	     */
	    const std::string & getName () const;

	    /**
	     * @returns: the cpu conso (in microsecond) 
	     */
	    unsigned long getConso () const;

	    /**
	     * @returns: the cpu capping (in microsecond) -1 if not capped
	     */
	    long getCapping () const;

	    /**
	     * @returns: the coeff of the curve of the consumption
	     */
	    double getSlope () const;
	    
	    /**
	     * @returns: the cpu period, that rules the cgroup capping policy in microsec
	     */
	    unsigned long getPeriod () const;

	    /**
	     * @returns: the number of vcpus of the machine
	     */
	    unsigned long getNbVCpus () const;

	    /**
	     * @returns: the base frequency to guarantee for the VM in Mhz
	     */
	    unsigned long getBaseFreq () const;	   

	    /**
	     * @returns: the used memory in MB
	     */
	    unsigned long getUsedMemory () const;

	    /**
	     * @returns: the size of the allocated memory in the VM 
	     */
	    unsigned long getAllocatedMemory () const;

	    /**
	     * @returns: the maximal size of the memory of the VM
	     */
	    unsigned long getMaxMemory () const;
	    
	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================          MONITORING          =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Update the information about the group (read this in the filesystem)
	     * @params: 
	     *   - delta: the elapsed time since last tick in second
	     */
	    bool update ();

	    /**
	     * Apply the capping on a vm
	     * @params:
	     *  - nbCycle: the number of authorized cycles for the VM in microseconds that does taking into account the period
	     */
	    void applyCapping (unsigned long nbCycle);
	    
	private: 

	    /**
	     * ================================================================================
	     * ================================================================================
	     * =========================           UPDATING           =========================
	     * ================================================================================
	     * ================================================================================
	     */
	    
	    /**
	     * Read the cpu consumption of the cgroup from the sys file
	     * @returns: the consumption in microsecond
	     */
	    unsigned long readConso (bool &);

	    /**
	     * Read the cpu capping of the cgroup from the sys file
	     * @returns: the maximum number of microsecond of cpu allocation in the time period (-1 if uncapped)
	     */
	    long readCap () const;

	    /**
	     * Read the cpu period, that rules the cgroup capping policy 
	     * @returns: the period in microsecond
	     */
	    unsigned long readPeriod () const;	    

	    /**
	     * Compute the slope of the history
	     * @params: 
	     *   - v: the history 
	     */
	    double computeSlope (const std::vector <double> & v) const;

	    /**
	     * Read the memory usage
	     * @returns: 
	     *   - used: the used memory in the VM (that actually correspond to allocated pages)
	     *   - allocated: the quantity of memory allocated by the host to the VM
	     */
	    void readMemory (unsigned long & used, unsigned long & allocated) const;

	    /**
	     * Configure virsh to acquire memory metrics
	     */
	    void configureMemory () const;
	    
	};


    }
}


std::ostream & operator<< (std::ostream & s, const monitor::cgroup::VMInfo & info);
