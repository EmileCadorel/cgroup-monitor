#pragma once

#include <vector>
#include <filesystem>
#include <monitor/concurrency/timer.hh>
#include <nlohmann/json.hpp>
#include <monitor/libvirt/controller/cgroup.hh>

namespace monitor {
    
    namespace libvirt {

	class LibvirtVM;

	namespace control {

	    class LibvirtVCPUController {

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================           CONTEXT            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/// The vm associated to the vcpu
		LibvirtVM & _context;


		/// The id of the vcpu
		int _id;

		/// The nominal frequency of the vcpu
		unsigned long _nominalFreq;

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            CGROUP            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/// The cgroup that get, and set limits
		cgroup _cgroup;

		/// The period of the vm cpu domain in microseconds
		unsigned long _period = 10000; // 10ms

		/// The actual quota of the vm cpu domain in microseconds (allowed micro seconds / seconds = period * quota)
		unsigned long _quota = -1;

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

		/// The sum of the frequency during the last micro ticks
		unsigned long _sumFrequency;

		/// The frequency during the last macro tick
		unsigned long _lastFrequency;

		/// The sum of the consumption during the last micro ticks
		unsigned long _sumConsumption;

		/// The number of micro ticks
		unsigned long _nbMicros;

		/// The cpu consumption of the vcpu 
		unsigned long _microConsumption = 0;

		/// The consumption of the last interval
		unsigned long _lastMicroConsumption = 0;

		/// The consumption of the vcpu during the last macro tick
		unsigned long _consumption = 0;		

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            TIMING            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/// The time spend between the last two micro ticks		
		float _microDelta;

		/// The sum of the time spent in the last micro ticks
		float _sumDelta;

		/// The time spent in the last macro tick
		float _delta;

		/// The timer used to compute the time spent between two frames
		concurrency::timer _t;

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            MARKET            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/// The number of cycles allocated to the vcpu
		unsigned long _allocated;		

		/// The number of cycles to buy 
		unsigned long _buying;
		
	    public:

		friend cgroup;
		

		/**
		 * @params: 
		 *    - id: the number of the vcpu (for example 0 of 4)
		 *    - context: the context of the cpu controller
		 *    - maxHistory: the maximum length of the history		  
		 */
		LibvirtVCPUController (int id, LibvirtVM & context, int maxHistory = 5);

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
		 * @params:
		 *    - cpuFrequency: the frequency of the cpus in the last tick
		 */
		void update (const std::vector <unsigned int> & cpuFrequency) ;

		/**
		 * Update the mean informations of the vcpu		 
		 */
		void updateBeforeMarket () ;
		
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
		 * @returns: the average frequency of the vcpu in the last macro tick
		 */
		unsigned long getFrequency () const;

		/**
		 * @returns: the frequency to guarantee for the vcpu
		 */
		unsigned long getNominalFreq () const;

		/**
		 * @returns: the context of the vcpu
		 */
		LibvirtVM & vm ();
		
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
		 * =========================            MARKET            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/**
		 * @returns: the number of cycles allocated to the vcpu
		 */
		unsigned long & allocated ();

		/**
		 * @returns: the number of cycles the vcpu wants to buy
		 */
		unsigned long & buying ();

		
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
				
	    };	    
	    
	}
	
    }
    
}
