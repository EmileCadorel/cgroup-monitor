#include <monitor/market.hh>
#include <sys/sysinfo.h>
#include <algorithm>  
#include <monitor/utils/log.hh>


using namespace monitor::cgroup;
using namespace monitor::utils;

namespace monitor {


    Market::Market () {
	this-> _config = MarketConfig {
	    1.0,
	    1.0,
	    0.1,
	    0.01,
	    0.1,
	    0.3,
	    85.0,
	    25.0, 
	    10000,
	    5,
	    10
	};
    }

    Market::Market (MarketConfig conf) {
	this-> _config = conf;
    }
    
    std::map <std::string, unsigned long> Market::update (const std::map <std::string, VMInfo> & vms) {
	if (vms.size () == 0) return {};

	unsigned long sold, allNeeded = 0;
	this-> incrementAccounts (vms);
	std::map <std::string, unsigned long> need;
	auto base = this-> sellBaseCycles (vms, sold, need);

	
	unsigned long market = ((unsigned long) (get_nprocs () * 1000000)) - sold;
	auto result = this-> buyCycles (vms, base, need, market, this-> _config.cyclePrice, this-> _firstIteration, allNeeded);	
	this-> _firstMarket = (get_nprocs () * 1000000) - market;

	// Rest some cycle that have not been sold, so there is some room for optimization
        // Maybe they are not bought because nobody has sufficient money to buy them, even if they are needed, so we run a second time the market but this time everything is free
        // This does not impact the performance of the VM with money but that are not buying, they have enough resources
	if (market > 0) {
	    long allSold = market;
	    for (auto & v : vms) {
		if (need [v.first] != 0) {
		    float percent = (float) (need [v.first]) / (float) allNeeded;
		    unsigned long add = (unsigned long) (percent * market);
		    result [v.first] += add;
		    allSold -= add;
		}
	    }
	    
	    this-> _lost = allSold;
	} else this-> _lost = 0;

	return result;
    }

    const std::map <std::string, unsigned long> & Market::getAccounts () const {
	return this-> _accounts;
    }
    
    unsigned long Market::getFirstNbIterations () const {
	return this-> _firstIteration;
    }

    unsigned long Market::getSecondNbIterations () const {
	return this-> _secondIteration;
    }

    unsigned long Market::getFirstMarketSold () const {
	return this-> _firstMarket;
    }

    unsigned long Market::getLost () const {
	return this-> _lost;
    }
    
    std::map <std::string, unsigned long> Market::buyCycles (const std::map <std::string, VMInfo> & vms, const std::map <std::string, unsigned long> & current, std::map <std::string, unsigned long> & needs, unsigned long & market, float cyclePrice, unsigned long & i, unsigned long & allNeed) {
	auto res = current;
	auto buyers = vms;
	i = 0;
	while (market > 0 && buyers.size () > 0) {
	    i += 1; 
	    for (auto v = buyers.cbegin () ; v != buyers.cend () ; ) {
		auto reserved = res [v-> first];
		auto need = needs [v-> first];
		auto money = this-> _accounts [v-> first];
		if (reserved < need) {
		    unsigned long window = this-> _config.windowSize; //this-> computeWindowSize (v.first, v.second);
		    unsigned long windowSize = 0;
		    if (cyclePrice != 0.0f) {
			windowSize = std::min (window, (unsigned long) (money / cyclePrice));
		    } else windowSize = window;

		    auto bought = std::min (std::min (windowSize, need - reserved), market);
		    if (bought != 0) {
			this-> _accounts [v-> first] = money - (bought * cyclePrice);
			res [v-> first] = res [v-> first] + bought;
			market = market - bought;
			v ++;
		    } else {
			needs [v-> first] = need - reserved;
			allNeed += need - reserved;
			buyers.erase (v ++);
		    }
		} else {
		    needs [v-> first] = 0;
		    buyers.erase (v ++);
		}
	    }
	}
	
	return res;
    }

    unsigned long Market::computeWindowSize (const std::string & name, const VMInfo & vm) const {
	auto money = this-> _accounts.find (name)-> second;
	if (money == 0) money = 1; // Log (0) is Nan
	
	auto maxCycle = vm.getMaximumConso ();
	auto perTurn = maxCycle * this-> _config.moneyIncrement;
	auto n = perTurn * this-> _config.windowMaximumTurn;
	
	auto w = std::min ((unsigned long) (this-> _config.windowSize * this-> _config.windowMultiplier),
			   (unsigned long) (1.0f / std::max ((n - money), 1.0f) * perTurn * (this-> _config.windowSize * this-> _config.windowMultiplier))) + this-> _config.windowSize;

	return w;
    }

    std::map <std::string, unsigned long> Market::sellBaseCycles (const std::map <std::string, VMInfo> & vms, unsigned long & all, std::map <std::string, unsigned long> & needs) const {
	std::map <std::string, unsigned long> res;
	all = 0;
	for (auto & v : vms) {
	    auto nb = (unsigned long) (v.second.getMaximumConso () * this-> _config.baseCycle);
	    auto need = v.second.getAbsoluteCapping ();
	    if (v.second.getRelativePercentConso () > this-> _config.triggerIncrement) {
		need = std::min (v.second.getMaximumConso (), (unsigned long) (v.second.getAbsoluteCapping () + (this-> _config.cycleIncrement * v.second.getMaximumConso ())));
	    } else if (v.second.getRelativePercentConso () > this-> _config.triggerDecrement) {
		need = std::min (nb, (unsigned long) (v.second.getAbsoluteCapping () - (this-> _config.cycleIncrement * v.second.getMaximumConso ())));
	    }
	    
	    
	    res.emplace (v.first, nb);
	    needs.emplace (v.first, need);	    
	    all += nb;
	}

	return res;
    }   

    void Market::incrementAccounts (const std::map <std::string, VMInfo> & vms) {
	std::map <std::string, unsigned long> res;
	for (auto & v : vms) {
	    auto nb = (long) (v.second.getMaximumConso () * this-> _config.moneyIncrement);
	    auto it = this-> _accounts.find (v.first);
	    if (it != this-> _accounts.end ()) {
		res.emplace (v.first, it-> second + nb);
	    } else res.emplace (v.first, nb);
	}

	this-> _accounts = res;
    }
    

}
