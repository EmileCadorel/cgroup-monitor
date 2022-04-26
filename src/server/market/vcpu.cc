#include "vcpu.hh"
#include <sys/sysinfo.h>
#include <algorithm>  
#include <monitor/utils/log.hh>

using namespace monitor::libvirt;
using namespace monitor::libvirt::control;
using namespace monitor::utils;

namespace server {

    namespace market {

	VCPUMarket::VCPUMarket (monitor::libvirt::LibvirtClient & client) :
	    _libvirt (client)
	{}
	
	VCPUMarket::VCPUMarket (monitor::libvirt::LibvirtClient & client, VCPUMarketConfig cfg) :
	    _libvirt (client),
	    _config (cfg)
	{}

	void VCPUMarket::setConfig (VCPUMarketConfig cfg) {
	    this-> _config = cfg;
	}

	void VCPUMarket::reset () {
	    for (auto & vm : this-> _libvirt.getRunningVMs ()) {
		vm-> money () = 0;
	    }
	}

	void VCPUMarket::run () {
	    auto & vms = this-> _libvirt.getRunningVMs ();
	    if (vms.size () == 0) return;
	    
	    long market = ((unsigned long) (get_nprocs () * 1000000));
	    unsigned long nbVcpus = 0;
	    auto buyers = this-> sellBaseCycles (vms, market, nbVcpus);
	    
	    // over allocation, can't do much
	    if (market < 0) return;
	    
	    unsigned long allNeeded = 0;
	    auto fails = this-> buyCycles (buyers, market, allNeeded);
	    
	    if (market > 0) {
		long notSold = market;
		long rest = std::min (allNeeded, (unsigned long) market);
		for (auto & vcpu : fails) { // we split the rest of the market between all the VMs that failed to buy
		    float percent = (float) (vcpu-> buying ()) / (float) allNeeded; // Implication of the VMs in the market 
		    unsigned long add = std::min (vcpu-> buying (), (unsigned long) (percent * rest));
		    vcpu-> allocated () += add;
		    vcpu-> buying () -= add;
		    notSold -= add;	   
		}
		market = notSold;
	    }

	    if (market > 0) {
		unsigned long percent = market / nbVcpus;		
		for (auto & v : vms) {
		    for (auto & vcpu : v-> getVCPUControllers ()) {
			auto old = vcpu.allocated ();
			auto max = std::min ((unsigned long) 1000000, old + percent);
			vcpu.allocated () = max;
		    }
		}
	    }

	    for (auto & v : vms) { // apply the vcpu allocations
	    	v-> applyMarketAllocation (100000);
	    }	    
	}


	std::list <LibvirtVCPUController*> VCPUMarket::buyCycles (std::list <LibvirtVCPUController*> & buyers,
								  long & market,
								  unsigned long & allNeeded) {
	    std::list <LibvirtVCPUController*> fails;
	    while (market > 0 && buyers.size () > 0) {
		for (auto v = buyers.cbegin () ; v != buyers.cend () ; ) { // we cannot use : for (auto & v : buyers), because we need to erase elements in the map
		    auto money = (*v)-> vm ().money ();
		    if ((*v)-> buying () != 0) {
			unsigned long windowSize = std::min (this-> _config.windowSize, money);
			
			/// The vcpu can buy at most, what they can (money, as windowSize), what they need (v-> second), or what is left in the market
			auto bought = std::min (std::min (windowSize, (*v)-> buying ()), (unsigned long) market);
			if (bought != 0) { /// The vcpu bought some cycles
			    (*v)-> allocated () += bought;
			    (*v)-> vm ().money () -= bought;
			    (*v)-> buying () -= bought;
			    market -= bought;
			    v++;
			} else {
			    allNeeded += (*v)-> buying ();
			    fails.push_back (*v);
			    buyers.erase (v++);
			}
		    } else {
			buyers.erase (v++);
		    }
		}
	    }
	    
	    return fails;
	}	    	

	std::list <LibvirtVCPUController*> VCPUMarket::sellBaseCycles (std::vector <LibvirtVM*> & vms, long & market, unsigned long & nbVcpus) {
	    std::list <LibvirtVCPUController*> ret;
	    for (auto & v : vms) {
		for (auto & c : v-> getVCPUControllers ()) {
		    if (this-> sellBaseCycles (c, market)) {
			ret.push_back (&c);
		    }
		    nbVcpus += 1;
		}
	    }

	    return ret;
	}

	bool VCPUMarket::sellBaseCycles (LibvirtVCPUController & v, long & market) {
	    unsigned long usage = v.getConsumption ();
	    unsigned long max = 1000000;
	    unsigned long min = max / 100; 
		
	    unsigned long nominal = ((float) v.getNominalFreq ()) / ((float) this-> _config.cpuFreq) * max;
	    unsigned long capp = v.getAbsoluteCapping ();

	    float perc_usage = v.getRelativePercentConsumption () / 100.0f;
	    double slope = v.getSlope ();	    
	    
	    /**
	     * We have three cases : 
	     *  - 1) The vcpu consumption is stable
	     */
	    if (slope > -0.1f && slope < 0.1f) {
		unsigned long increase = std::min (max, (unsigned long) (usage + max * 0.01));
		unsigned long current = std::max (min, std::min (nominal, increase));
		v.allocated () = current;
		market -= current;
		
		if (increase > nominal) {
		    v.buying () = std::min (max - nominal, increase - nominal);
		    return true;
		} else {
		    v.vm ().money () += nominal - increase;
		    return false;
		}		    
	    }

	    /**
	     *   - 2) The vcpu usage is lower than the decrease trigger
	     */	    
	    else if (perc_usage < this-> _config.triggerDecrement) {
		unsigned long decrease = std::max (min, std::max (usage, (unsigned long) (capp * (1.0 - this-> _config.decreasingSpeed))));
		unsigned long current = std::min (nominal, decrease);
		v.allocated () = current;
		market -= current;

		if (decrease > nominal) {
		    v.buying () = std::min (max - nominal, decrease - nominal);
		    return true;
		} else {
		    v.vm ().money () += nominal - decrease;
		    return false;
		}
	    }

	    /**
	     *   - 3) The vcpu usage is higher than the increase trigger
	     */
	    else if (perc_usage > this-> _config.triggerIncrement) {
		unsigned long increase = capp * (1.0 + this-> _config.increasingSpeed);
		unsigned long current = std::max (min, std::min (nominal, increase));
		v.allocated () = current;
		market -= current;

		if (increase > nominal) {
		    v.buying () = std::min (max - nominal, increase - nominal);
		    return true;
		} else {
		    v.vm ().money () += nominal - increase;
		    return false;
		}		    
	    }

	    /**
	     *   - 4) The VM usage is between the two triggers, or the slope is really flat
	     */
	    else {
		unsigned long current = std::max (min, std::min (nominal, capp));
		v.allocated () = current;
		market -= current;
		
		if (capp > nominal) {
		    v.buying () = std::min (max - nominal, capp - nominal);
		    return true;
		} else {
		    v.vm ().money () += nominal - capp;
		    return false;
		}
	    }

	}

    }
}
