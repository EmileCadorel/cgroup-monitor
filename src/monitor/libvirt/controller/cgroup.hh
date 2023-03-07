#pragma once

#include <vector>
#include <filesystem>
#include <fstream>
#include <monitor/concurrency/timer.hh>
#include <nlohmann/json.hpp>

namespace monitor {

	namespace libvirt {
	
		namespace control {
	    
			class cgroup {
		
				const std::string _vmName;
		
				/// True if cgroup is v2
				bool _v2;

				/// The id of the proc of the vcpu
				unsigned int _procId;

				/// The file in which usage of the vcpu is written
				std::ifstream _usage;

				/// The file in which the limit of the vcpu is written
				std::filesystem::path _limit;

				/// THe file in which the core pins of the vcpu is written
				std::filesystem::path _cpuPins;

				/// The file in which the stat of the vcpu process is written
				std::filesystem::path _procPath;
		
				public:

					/**
					 * @params:
					 *    - name: the name of the vm
					 */
					cgroup (const std::string & vmName);

					/**
					 * Enable the cgroup
					 */
					void enable (unsigned int vcpuId);
				
					/**
					 * Read the current usage of the cgroup
					 */
					unsigned int readUsage ();

					/**
					 * Set the list of cpu cores that can be used by the cgroup
					 */
					void setCorePins (const std::string & cores);

					/**
					 * Set the limit of the cgroup
					 */
					void setLimit (long nbMicros, unsigned long period);

					/**
					 * Read the id of the cpu that is running the vcpu
					 */
					unsigned int readCpu ();
		
				private:

					/**
					 * @returns: the proc id of the associated vcpu
					 */
					unsigned int readProcId (const std::filesystem::path & cgroupPath) const;
		
					/**
					 * Recursively search for the cgroup of the VM
					 */
					std::filesystem::path recursiveSearch (const std::filesystem::path & p, const std::string & vmName);
		
			};

		};
	
	}

}
