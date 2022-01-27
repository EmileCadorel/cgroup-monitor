#pragma once

#include <vector>
#include <filesystem>
#include <monitor/concurrency/timer.hh>
#include <nlohmann/json.hpp>

namespace monitor {

    namespace libvirt {

	class LibvirtVM;
	
	namespace control {

	    /**
	     * The cpu controller class is used to get the cpu time of a VM, and tune it
	     */
	    class LibvirtCpuController {

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================           CONTEXT            =========================
		 * ================================================================================
		 * ================================================================================
		 */
		
		/// The vm associated to the controller
		LibvirtVM & _context;

		/// The path to the cgroup directory
		std::filesystem::path _cgroupPath;

		/// True iif the cgroup is v2
		bool _cgroupV2;

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            CGROUP            =========================
		 * ================================================================================
		 * ================================================================================
		 */
		
		/// The period of the vm cpu domain in microseconds
		unsigned long _period = 10000; // 10ms

		/// The actual quota of the vm cpu domain in microseconds (allowed micro seconds / seconds = period * quota)
		unsigned long _quota = -1;

		/// The cpu consumption of the cpu 
		unsigned long _consumption = 0;

		/// The consumption of the last interval
		unsigned long _lastConsumption = 0;

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================           HISTORY            =========================
		 * ================================================================================
		 * ================================================================================
		 */
		
		/// The history in used percentage of the maximum consumption
		std::vector <float> _history;

		/// The maximum length of the history 
		int _maxHistory;

		/// The slope of the history
		double _slope = 0;

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            TIMING            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/// The time spend between the last two ticks		
		float _delta;

		/// The timer used to compute the time spent between two frames
		concurrency::timer _t;

	    public:

		/**
		 * @params: 
		 *    - context: the context of the cpu controller
		 *    - maxHistory: the maximum length of the history		  
		 */
		LibvirtCpuController (LibvirtVM & context, int maxHistory = 5);

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================          ACQUIRING           =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/**
		 * Initialize some variable for the cpu controller
		 * @info: this function should be called once after the VM is booted
		 */
		void enable () ;

		/**
		 * Update the information of the cpu
		 * @info: this function should be called periodically
		 */
		void update () ;
		
		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================           GETTERS            =========================
		 * ================================================================================
		 * ================================================================================
		 */
		
		/**
		 * @returns: the cpu consumption conmputed in the last update tick
		 */	       
		unsigned long getConsumption () const;

		/**
		 * @returns: the number of period in one second
		 */
		int getPeriod () const;

		/**
		 * @returns: the cpu slope of the change of consumption in the latests ticks
		 */
		float getSlope () const;

		/**
		 * @returns: the actual quota of the cpu domain in one period
		 */
		int getQuota () const;

		/**
		 * @returns: the cpu consumption of the last tick scaled to the second
		 */
		unsigned long getAbsoluteConsumption () const;

		/**
		 * @returns: the maximum number of cycles the VM can consume in one second if not capped
		 */
		unsigned long getMaximumConsumption () const;

		/**
		 * @returns: the maximum number of cycles the VM can consume in one second based on the current capping
		 */
		unsigned long getAbsoluteCapping () const ;

		/**
		 * @returns: the percentage consumption of the VM in relation to the maximum consumption
		 */
		float getPercentageConsumption () const;

		/**
		 * @returns: the percentage consumption of the VM in relation to the capping
		 */
		float getRelativePercentConsumption () const;
		
		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================         CONTROLLING          =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/**
		 * Update the quota of the cpu domain
		 * @params: 
		 *   - nbMicros: the number of microseconds of cpu usage allowed for the cpu domain during one period
		 *   - period: the number of period in one second
		 */
		void setQuota (unsigned long nbMicros, unsigned long period = 10000);

		/**
		 * Remove the quota limitation of the cpu domain
		 * @info: sets quota to -1
		 */
		void unlimit ();

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================             LOG              =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/**
		 * @returns: the log about the cpu controller for the current tick
		 */
		nlohmann::json dumpLogs () const;

	    private :

		/**
		 * Add a value to the end of the history
		 */
		void addToHistory ();

		/**
		 * Compute the slope of the history
		 */
		void computeSlope ();

		/**
		 * @returns: the current consumption of the cgroup
		 */
		unsigned long readConsumption () const;

		/**
		 * Recursively search for the cgroup of the VM
		 */
		std::filesystem::path recursiveSearch (const std::filesystem::path & p, const std::string & vmName);
	    };
	    
	}
	
    }    

}
