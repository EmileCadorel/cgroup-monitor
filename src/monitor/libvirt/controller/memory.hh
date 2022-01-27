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
	     * The memory controller class is used to get the memory usage of a VM and tune it
	     */
	    class LibvirtMemoryController {

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================           CONTEXT            =========================
		 * ================================================================================
		 * ================================================================================
		 */

		
		/// The VM associated to the controller
		LibvirtVM & _context;

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================            USAGE             =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/// The quantity of memory being used in KB, allocated on the host
		unsigned int _hostUsed;

		/// THe quantity of memory that is swapping in the VM in KB
		unsigned int _swapping;

		/// The quantity of memory being reclamed by the guest in KB (taken in host and swapping)
		unsigned int _guestUsed;
		
		/// The quantity of memory that is allocated to the VM in KB (forced by the controller not the value acquired by monitoring)
		unsigned int _allocated;

		/// The maximum size of the memory that can be allocated to the VM in KB
		unsigned int _max;

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
		
	    public:

		/**
		 * @params: 
		 *    - context: the context of the memory controller
		 *    - maxHistory: the maximum length of the history
		 */
		LibvirtMemoryController (LibvirtVM & context, int maxHistory = 5);

		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================          ACQUIRING           =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/**
		 * Start the memory controller
		 * @info: this function should be called just after the creation of the domain
		 */
		void enable () ;
		
		/**
		 * Update the information of the memory usage
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
		 * @returns: the maximum quantity of memory that can be allocated to the VM in KB
		 */
		unsigned long getMaxMemory () const;

		/**
		 * @returns: the quantity of memory that is used by the VM in KB
		 */
		unsigned long getGuestUsed () const;

		/**
		 * @returns: the quantity of memory allocated by the domain on the host in KB
		 */
		unsigned long getHostUsed () const;
		
		/**
		 * @returns: the quantity of swap in the VM in KB
		 */
		unsigned long getSwapping () const; 

		/**
		 * @returns: the quantity of memory that is allocated to the VM in the host, that is forced by the controller 
		 * @warning: might be a different value than getHostUsed (), but refer to the same thing
		 */
		unsigned long getAllocated () const;
		
		/**
		 * @returns: the quantity of memory used by the VM in relation to the maximum allocation
		 * @warning: without swap
		 */
		float getAbsolutePercentUsed () const;


		/**
		 * @returns: the quantity of memory used by the VM in relation to the current allocation
		 * @warning: without swap
		 */
		float getRelativePercentUsed () const;

		/**
		 * @returns: the slope of the used memory
		 */
		float getSlope () const;
			       		
		/**
		 * ================================================================================
		 * ================================================================================
		 * =========================         CONTROLLING          =========================
		 * ================================================================================
		 * ================================================================================
		 */

		/**
		 * Set the quantity of memory that can be allocated on the host without swapping
		 * @params:
		 *   - max: the quantity of memory to allocate in KB
		 */
		void setAllocatedMemory (unsigned long max);

		/**
		 * Remove the memory limitation of the VM
		 * @info: sets the max to this-> getMaxMemory ()
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
		 * @returns: the log about the memory controller for the current tick
		 */
		nlohmann::json dumpLogs () const;



	    private :

		/**
		 * Add the last used consumption to the history
		 */
		void addToHistory ();

		/**
		 * compute the slope of the history
		 */
		void computeSlope ();
		
	    };

	}

    }
    
}
