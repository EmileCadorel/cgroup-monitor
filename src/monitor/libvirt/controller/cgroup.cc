#include <monitor/libvirt/controller/vcpu.hh>
#include <monitor/libvirt/vm.hh>
#include <libvirt/libvirt.h>
#include <monitor/utils/log.hh>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
using namespace monitor::utils;

namespace monitor {

    namespace libvirt {

		namespace control {

			cgroup::cgroup (const std::string & vmName) :
				_vmName (vmName)
			{
				std::ifstream f ("/sys/fs/cgroup/cgroup.controllers");
				this-> _v2 = f.good ();
				f.close ();
			}

			void cgroup::enable (unsigned int vcpuId) {
				concurrency::timer t;
				std::stringstream ss; ss << "libvirt/vcpu" << vcpuId;
				std::stringstream vname; vname << this-> _vmName;
				std::filesystem::path cgroupPath;
				if (this-> _v2) {
					auto path = fs::path ("/sys/fs/cgroup/machine.slice");
					for (;;) {
						cgroupPath = this-> recursiveSearch (path, vname.str ());
						if (cgroupPath.u8string ().length () != 0) {
							cgroupPath = cgroupPath  / ss.str ();
							break;
						}
						t.sleep (0.1);
					}
				} else {
					auto path = fs::path ("/sys/fs/cgroup/cpu/machine.slice");
					for (;;) {
						cgroupPath = this-> recursiveSearch (path, vname.str ());
						if (cgroupPath.u8string ().length () != 0) {
							cgroupPath = cgroupPath  / ss.str ();
							break;
						}
						t.sleep (0.1);
					}
		
				}

				this-> _procId = this-> readProcId (cgroupPath);
		
				std::stringstream ss2;
				ss2 << "/proc/" << this-> _procId << "/stat";
				auto procPath = std::filesystem::path (ss2.str ());

				if (this-> _v2) {
					this-> _usage = std::ifstream (cgroupPath / "cpu.stat");
					this-> _limit = cgroupPath / "cpu.max";
					this-> _cpuPins = cgroupPath / "cpuset.cpus";
				} else {
					this-> _usage = std::ifstream (cgroupPath / "cpuacct.usage");
					this-> _limit =  cgroupPath / "cpu.cfs_quota_us";
					this-> _cpuPins = cgroupPath / "cpuset.cpus";
				}
		
				this-> _procPath = procPath;
			}

			unsigned int cgroup::readUsage () {
				this-> _usage.sync ();
				this-> _usage.seekg (0, this-> _usage.beg);
				unsigned long res = 0;
				if (this-> _v2) {
					std::string ignore;
					this-> _usage >> ignore;
					this-> _usage >> res;

					return res;
				} else {
					this-> _usage >> res;
					return res / 1000;
				}
			}


			void cgroup::setCorePins (const std::string & cores) {
				std::ofstream file (this-> _cpuPins);
				file << cores;
				file.close ();
			}

			void cgroup::setLimit (long nbMicros, unsigned long period) {
				std::ofstream limit (this-> _limit);
				if (this-> _v2) {
					if (nbMicros == -1) {
						limit << "max " << period;
					} else {
						limit << nbMicros << " " << period;
					}
				} else {
					limit << nbMicros;
				}
		
				limit.close ();
			}

			unsigned int cgroup::readCpu () {
				FILE *fp = fopen(this-> _procPath.c_str (), "r");
				if(fp != NULL)
				{
					char content[1024];
					size_t len = fread (content, sizeof (char), 1024, fp);
					content[len] = '\0';
		    
					int i = 0, j = 0;
					for (;;)
					{
						if (content[j] == ' ') { i += 1; if (i == 39) break;}
						j += 1;
					}

					char * endPtr;
					int p = strtol (content + j, &endPtr, 10);
		    
					fclose(fp);
					return p;
				}
		
				return 0;
			}


			unsigned int cgroup::readProcId (const std::filesystem::path & cgroupPath) const {
				auto path = cgroupPath / "cgroup.threads";
				std::ifstream t (path);
				std::stringstream buffer;
				buffer << t.rdbuf();
				t.close ();
		
				int j = 0;
				buffer >> j;

				return j;
			}
	    
			std::filesystem::path cgroup::recursiveSearch (const fs::path & path, const std::string & name) {
				if (fs::is_directory (path)) {
					std::cout << path << " " << name << " " << (path.u8string ().find(name) != std::string::npos) << std::endl;
					if (path.u8string ().find(name) != std::string::npos) {
						return path;
					} else {
						for (const auto & entry : fs::directory_iterator(path)) {
							if (fs::is_directory (entry.path ())) {
								auto ret = this-> recursiveSearch (fs::path (entry.path ()), name);
								if (ret != "") return ret;
							}
						}
					}
				}

				return "";
			}

		}

    }

}
