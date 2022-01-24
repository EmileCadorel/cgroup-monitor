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

		/// The quantity of memory being used in KB (real RAM)
		unsigned int _used;

		/// THe quantity of memory that is swapping in the VM in KB
		unsigned int _swapping;

		/// The quantity of RAM that is unused (from the domain, that is not aware of swapping) in KB
		unsigned int _unused;
		
		/// The quantity of memory that is allocated to the VM in KB
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
		LibvirtMemoryController (LibvirtVM & context);

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
		unsigned long getUsedMemory () const;

		/**
		 * @returns: the quantity of swap in the VM in KB
		 */
		unsigned long getSwapMemory () const; 

		/**
		 * @returns: the quantity of memory that is allocated to the VM in the host (memory that can be used before swapping)
		 */
		unsigned long getAllocatedMemory () const;
		
		/**
		 * @returns: the quantity of memory used by the VM in relation to the maximum allocation (without counting swap)
		 */
		float getAbsolutePercentUsed () const;


		/**
		 * @returns: the quantity of memory used by the VM in relation to the current allocation (without counting swap)
		 */
		float getRelativePercentUsed () const;

		/**
		 * @returns: the slope of the used memory
		 */
		float getSlope () const;

		/**
		 * @returns: the slope of the swapping memory
		 */
		float getSwapSlope () const;
			       		
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
		void setAllocatedMemory (int max);

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
		 * @returns: the slope of the list of points
		 */
		float computeSwapSlope (const std::vector <float> & history);
		
	    };

	}

    }
    
}
