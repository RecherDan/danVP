#include "uopcache.h"
#include "core.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include "simulator.h"
#include "tools.h"
#include "stats.h"

UopCache::UopCache() {
	  /* initialize random seed: */
  	srand (time(NULL));
  	this->uopenabled = true;
  	this->VPremovevpentry = false;
  	if ( Sim()->getConfig()->getUOPstatus() == Config::UOP_DISABLE ) {
  		this->uopenabled = false;
  	}
  	if ( Sim()->getConfig()->getUOPdebug())
  		std::cout << "DEBUG: UopCache::UopCache() STATUS: " << this->uopenabled << " " << (!this->uopenabled ? "DISABLE" : "ENABLE")  << std::endl;
  	if ( Sim()->getConfig()->getVPremovevpentry())
  		this->VPremovevpentry = true;

  	uopcache_hits = 0;
  	uopcache_miss = 0;
  	uopcache_access = 0;
	uopcache_VP_access = 0;
	uopcache_VP_hits = 0;
	uopcache_VP_miss = 0;
	uopcache_VP_stores = 0;
	uopcache_VP_haveprediction = 0;
	uopcache_stores = 0;
	uopcache_evictions = 0;
	uopcache_VP_stores_success = 0;
	uopcache_VP_stores_fails =0;
    registerStatsMetric("uopcache", 0 , "uopcache_hits", &uopcache_hits);
    registerStatsMetric("uopcache", 0 , "uopcache_miss", &uopcache_miss);
    registerStatsMetric("uopcache", 0 ,"uopcache_access", &uopcache_access);
    registerStatsMetric("uopcache", 0 ,"uopcache_stores", &uopcache_stores);
    registerStatsMetric("uopcache", 0 ,"uopcache_evictions", &uopcache_evictions);
    registerStatsMetric("uopcache", 0 , "uopcache_VP_access", &uopcache_VP_access);
    registerStatsMetric("uopcache", 0 , "uopcache_VP_hits", &uopcache_VP_hits);
    registerStatsMetric("uopcache", 0 ,"uopcache_VP_miss", &uopcache_VP_miss);
    registerStatsMetric("uopcache", 0 ,"uopcache_VP_stores", &uopcache_VP_stores);
    registerStatsMetric("uopcache", 0 ,"uopcache_VP_stores_success", &uopcache_VP_stores_success);
    registerStatsMetric("uopcache", 0 ,"uopcache_VP_stores_fails", &uopcache_VP_stores_fails);
    registerStatsMetric("uopcache", 0 ,"uopcache_VP_haveprediction", &uopcache_VP_haveprediction);
}
bool UopCache::isUopCacheValid() {
	if ( !this->uopenabled ) return false;
	return this->uopcacheIsOn;
}
bool UopCache::periodicPredict() {
	if ( !this->uopenabled ) return false;
	this->priod++;
	if ( this->priod >= this->priodLimit ) {
		this->priod = 0;
		return true;
	}
	return false;
}
bool UopCache::PredictUop(unsigned long pc, unsigned long BBhead) {
	if ( !this->uopenabled ) return false;
	bool predict = false;
	if ( pc != BBhead ) {
		return false;
	}
	unsigned long set = UopCache::getSet(BBhead);
	int hitbank = UopCache::getWay(BBhead);
	int conf = this->uopCache[hitbank][set].conf;
	if ( Sim()->getConfig()->getUOPdebug()) {
		std::cout << "DEBUG: UopCache::PredictUop get prediction of PC: " << std::hex << pc << " BBhead: " << BBhead << std::dec << " set: " << set << " way: " << hitbank  << std::endl;
	}
	if ( this->uopCache[hitbank][set].valid && this->uopCache[hitbank][set].pc == BBhead && ((conf >= UOPCONFLIMIT -1) || (UOPCONFLIMIT == 0) )) {
		if ( pc == BBhead )
			this->uopCache[hitbank][set].ready = false;
		if ( !this->uopCache[hitbank][set].ready )
			predict = true;
	}
	if ( pc == BBhead ) {
		UopCache::storeUopCache(BBhead);
	}

	//return this->periodicPredict();
	//return RandomPredict();
	if ( Sim()->getConfig()->getUOPdebug())
		std::cout << "DEBUG: UopCache::PredictUop PC: " << std::hex << pc << " BBhead: " << BBhead << std::dec << " conf: " << conf << " prediction: " << (predict ? "yes" : "no")  << std::endl;
	this->uopcache_access++;
	if ( predict ) uopcache_hits++;
	else uopcache_miss++;
	return predict;
}
bool UopCache::existInUopCache(unsigned long pc) {
	if ( !this->uopenabled ) return false;
	if ( Sim()->getConfig()->getUOPdebug())
					std::cout << "DEBUG: UopCache::existInUopCache PC: " << std::hex << pc << std::dec << "UOP IS: "  << this->uopenabled << " " << (!this->uopenabled?  "DISABLE" : "ENABLE")  << std::endl;

	bool exist = false;
	unsigned long set = UopCache::getSet(pc);
	int hitbank = UopCache::getWay(pc);
if ( this->uopCache[hitbank][set].valid && this->uopCache[hitbank][set].pc == pc ) {
	exist = true;
}

return exist;
	//return true;
	//return (rand() %5 == 0 ) ? true : false;	
}
bool UopCache::RandomPredict() {
	//return true;
	//return true;
	return (rand() %5 == 0 ) ? true : false;	
}
bool UopCache::AddVPinfo(unsigned long pc, unsigned long bbhead, unsigned long value) {
	if ( !this->uopenabled ) return false;

	unsigned long set = UopCache::getSet(bbhead);
	int hitbank = UopCache::getWay(bbhead);
	if ( !this->uopCache[hitbank][set].valid ) return false;
	this->uopcache_VP_stores++;
	for ( int i = 0 ; i < MAXVPINFO ; i++) {
		vpinfo curVPinfo = this->uopCache[hitbank][set].VPinfo[i];
		if ( !curVPinfo.valid || curVPinfo.pc == pc ) {
			curVPinfo.valid = true;
			curVPinfo.validpredict = true;
			curVPinfo.pc = pc;
			curVPinfo.value = value;
			this->uopCache[hitbank][set].VPinfo[i] = curVPinfo;
			this->uopcache_VP_stores_success++;
			return true;
		}
	}
	this->uopcache_VP_stores_fails++;
	return false;
}
std::tuple<bool, bool> UopCache::getVPprediction(unsigned long pc, unsigned long bbhead, unsigned long value) {
	std::tuple<bool, bool> fret{false, false};

	if ( !this->uopenabled ) return fret;
	this->uopcache_VP_access++;
	bool goodPrediction = false;
	bool badPrediction = false;
	unsigned long set = UopCache::getSet(bbhead);
	int hitbank = UopCache::getWay(bbhead);
	if ( !this->uopCache[hitbank][set].valid ) return fret;
	int vpInd = -1;
	for ( int i = 0 ; i < MAXVPINFO ; i++ ) {
		vpinfo curVPinfo = this->uopCache[hitbank][set].VPinfo[i];
		if ( curVPinfo.valid && curVPinfo.pc == pc ) {
			vpInd = i;
		}
	}
	if ( vpInd == -1 ) return fret;
	if ( this->uopCache[hitbank][set].VPinfo[vpInd].validpredict ) this->uopcache_VP_haveprediction++;
	if ( value ==this->uopCache[hitbank][set].VPinfo[vpInd].value && this->uopCache[hitbank][set].VPinfo[vpInd].validpredict )
		goodPrediction = true;
	else if ( this->uopCache[hitbank][set].VPinfo[vpInd].validpredict )
		badPrediction = true;

	if ( goodPrediction ) this->uopcache_VP_hits++;
	if ( badPrediction ) this->uopcache_VP_miss++;
	//if ( goodPrediction || badPrediction ) this->uopcache_VP_haveprediction++;
	// emit VP on miss:
	if ( badPrediction ) {
		this->uopCache[hitbank][set].VPinfo[vpInd].blacklist++;
		this->uopCache[hitbank][set].VPinfo[vpInd].validpredict = false;
		if ( this->uopCache[hitbank][set].VPinfo[vpInd].blacklist > 7 ) this->uopCache[hitbank][set].VPinfo[vpInd].valid = false;
	}
	std::tuple<bool, bool> tupleret{goodPrediction, badPrediction};

	return tupleret;
}
bool UopCache::checkVPinfo(unsigned long pc, unsigned long bbhead, unsigned long *value) {
	if ( !this->uopenabled ) return false;

	unsigned long set = UopCache::getSet(bbhead);
	int hitbank = UopCache::getWay(bbhead);
	if ( !this->uopCache[hitbank][set].valid ) return false;
	for ( int i = 0 ; i < MAXVPINFO ; i++ ) {
		vpinfo curVPinfo = this->uopCache[hitbank][set].VPinfo[i];
		if ( curVPinfo.valid && curVPinfo.pc == pc ) {
			*value = curVPinfo.value;
			return true;
		}
	}
	return false;
}
bool UopCache::storeUopCache(unsigned long pc) {
	if ( !this->uopenabled ) return false;

	bool overwritten = false;
	unsigned long set = UopCache::getSet(pc);
	int hitbank = UopCache::getWay(pc);
	if ( Sim()->getConfig()->getUOPdebug())
		std::cout << "DEBUG: UopCache::storeUopCache store prediction of PC: " << std::hex << pc << std::dec << "set: " << set << " way: " << hitbank  << std::endl;

	if ( this->uopCache[hitbank][set].valid ) {
		if ( this->uopCache[hitbank][set].pc != pc ) {
			overwritten = true;
			this->uopCache[hitbank][set].conf = 0;
			// clean vp info
			for ( int i=0 ; i < MAXVPINFO ; i++ ) {
				this->uopCache[hitbank][set].VPinfo[i].valid = false;
			}
		}
		else if ( (this->uopCache[hitbank][set].conf < (UOPCONFLIMIT -1)) || (UOPCONFLIMIT == 0) ) {
			this->uopCache[hitbank][set].conf = this->uopCache[hitbank][set].conf + 1;
			if (this->uopCache[hitbank][set].conf == (UOPCONFLIMIT -1))
				this->uopCache[hitbank][set].ready = true;
		}
	}
	else {
		this->uopcache_stores++;
	}
	this->uopCache[hitbank][set].valid = true;
	this->uopCache[hitbank][set].pc = pc;
	UopCache::updateLRU(set, hitbank);
	if ( overwritten ) this->uopcache_evictions++;
	return overwritten;
}
unsigned long UopCache::getSet(unsigned long pc) {
	return pc%this->hashnum;
}
unsigned long UopCache::getWay(unsigned long pc) {
	unsigned long set = UopCache::getSet(pc);
	int victimbank = -1;
	int emptyway = -1;
	//check if pc already exist
	for ( int i = 0 ; i < UOPWAYS ; i++) {
		if ( ! uopCache[i][set].valid && emptyway == -1 ) emptyway = i;
		if ( uopCache[i][set].pc == pc ) {
			victimbank = i;
		}
	}

	// if pc is not exist then evict
	if ( victimbank == -1 ) {
		if ( emptyway != -1 ) {
			victimbank = emptyway;
			std::cout << "DEBUG: UopCache::getWay found empty way set: " << set << " bank: " << victimbank  << std::endl;

		}
		else {
			int LRU = -1;
			for( int i=0; i < UOPWAYS; i++ ) {
				if ( Sim()->getConfig()->getUOPdebug())
					std::cout << "DEBUG: UopCache::getWay check LRU: set: " << set << " bank: " << i << " LRU NEW: " << uopCache[i][set].LRU << " OLD: " << LRU << std::endl;

				if ( uopCache[i][set].LRU > LRU ) {
					LRU = uopCache[i][set].LRU;
					victimbank = i;
				}
			}
		}

	}
	return victimbank;
}
void UopCache::updateLRU(unsigned long set, int victimbank ) {
	for ( int i=0; i<UOPWAYS; i++ ) {
		if ( i == victimbank ) continue;
		if ( uopCache[i][set].LRU <= uopCache[victimbank][set].LRU )
			uopCache[i][set].LRU++;
	}
	uopCache[victimbank][set].LRU = 0;
}
