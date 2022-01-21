#pragma once

#include <vector>

namespace monitor {

    namespace libvirt {

	class LibvirtVM;
	
	namespace control {

	    /**
	     * The cpu controller class is used to get the cpu time of a VM, and tune it
	     */
	    class LibvirtCpuController {

		/// The period of the vm cpu domain in microseconds
		int _period = 10000; // 10ms

		/// The actual quota of the vm cpu domain in microseconds (allowed micro seconds / seconds = period * quota)
		int _quota = -1;

		/// The cpu consumption of the cpu 
		unsigned long _consumption = 0;

		/** 
		 * Cache data to compute the cpu consumption inside a tick, and slope of the cpu domain
		 * the data stored in that array are absolute
		 * Meaning that the consumption between two tick i and j is history[j] - history[i];
		 */
		std::vector <unsigned long> _history;

		/// The maximum length of the history 
		int _maxHistory;

		/// The slope of the history
		double _slope = 0;
		
		/// The vm associated to the controller
		LibvirtVM & _context;


	    public:

		/**
		 * @params: 
		 *    - context: the context of the cpu controller
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
		 * Update the information of the cpu
		 * @info: this function should be called periodically (without any change in pace)
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
		void setQuota (int nbMicros, int period = 10000);

		/**
		 * Remove the quota limitation of the cpu domain
		 * @info: sets quota to -1
		 */
		void unlimit ();


	    private :

		/**
		 * Add a value to the end of the history
		 */
		void addToHistory (unsigned long val);

		/**
		 * Compute the slope of the history
		 */
		void computeSlope ();
	    };
	    
	}
	
    }    

}
