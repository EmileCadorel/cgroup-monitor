#pragma once

#include <string>
#include <iostream>
#include <filesystem>

namespace monitor {
    namespace cgroup {

	class GroupInfo {

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
	   
	public:

	    /**
	     * Create a new cgroup from a path
	     * @params: 
	     *  - path: the path of the cgroup in the file system (e.g. /sys/fs/cgroup/my_group)
	     */
	    GroupInfo (const std::filesystem::path & path);	    

	    /**
	     * @returns: the path of the cgroup
	     */
	    const std::filesystem::path & getPath () const;
	    
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
	     * @returns: the cpu period, that rules the cgroup capping policy in microsec
	     */
	    unsigned long getPeriod () const;

	    /**
	     * Update the information about the group (read this in the filesystem)
	     * @params: 
	     *   - delta: the elapsed time since last tick in second
	     */
	    void update (float delta);
	    
	private: 
	    
	    /**
	     * Read the cpu consumption of the cgroup from the sys file
	     * @returns: the consumption in microsecond
	     */
	    unsigned long readConso () const;

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
	    
	    
	};
	
    }
}


std::ostream & operator<< (std::ostream & s, const monitor::cgroup::GroupInfo & info);
