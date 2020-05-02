/*
 * vp_simple.cc
 *
 *  Created on: May 3, 2019
 *      Author: dan
 */
#include "vp_simple.h"
#include "simulator.h"
static VPSIM_ENTRY vpSim[VPSIMWAYCOUNT+10][VPSIMNUMOFENTRIES+10];

//#define SETSIZE 10
//#define OFFSET 5
//#define VPSIMNUMOFENTRIES 2^SETSIZE
//#define VPSIMWAYCOUNT 4
//#define MAXCONF 7
//#define VPSIMPLEDEBUG
int vpsim_waycount = 10;
int vpsim_entries;
int vpsim_offset =5;
int vpsim_setsize =10;
int vpmaxconf =7;
int entrysize = 10;

// size is: num_of_bits 19 * num of entries * num of ways
// num of bits log2(vpmaxconf) = 3 + log2(tag) =15 + log2(validsize) = 1 ==> 19
// 8K ==> 19 * 2^8 * 2
// 32K ==> 19 * 2^9 * 4
// UNLIMITED ==> 19 * 2^10 * 8

void VPSIM_setglobals(int a) {
	// 8k option
	if ( a== 0 ) {
		vpsim_waycount = 2;
		vpsim_offset =5;
		vpsim_setsize =8;
		vpmaxconf =7;
		entrysize = 10;

	}

	// 32K option
	if ( a ==1 ) {
		vpsim_waycount = 4;
		vpsim_offset =5;
		vpsim_setsize =9;
		vpmaxconf =7;
		entrysize = 10;
	}

	// unlimited size option
	if ( a==2 ) {
		vpsim_waycount = 8;
		vpsim_offset =5;
		vpsim_setsize =10;
		vpmaxconf =7;
		entrysize = 10;
	}
	vpsim_entries = 2^vpsim_setsize;
}
void
VPSIM_updateLRU(int hitbank, int set) {
	for ( int i=0; i<vpsim_waycount; i++ ) {
		if ( i == hitbank ) continue;
		if ( vpSim[i][set].LRU <= vpSim[hitbank][set].LRU )
			vpSim[i][set].LRU++;
	}
	vpSim[hitbank][set].LRU = 0;
}

uint VPSIM_getset(uint64_t pc) {
	//int set = (pc/(2^vpsim_offset)) % vpsim_setsize;
	uint set = pc% vpsim_setsize;
	return set;
}
bool VPSIM_invalidateEntry(UInt64 pc) {
	int hitbank = -1;
	uint set = VPSIM_getset(pc);
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_invalidateEntry PC: " << std::hex << pc << std::dec << std::endl;

	for( int i=0; i < vpsim_waycount; i++ ) {
		if ( vpSim[i][set].valid && vpSim[i][set].pc == pc ) {
			hitbank=i;
			break;
		}
	}
	if ( hitbank == -1 ) return false;
	vpSim[hitbank][set].valid = false;

	return true;
}
bool
VPSIM_getPrediction (UInt64 seq_no, UInt64 pc, UInt64 piece,
	       UInt64 & predicted_value) {

	int hitbank = -1;
	uint set = VPSIM_getset(pc);
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_getPrediction PC: " << std::hex << pc << std::dec << std::endl;

// find match
	for( int i=0; i < vpsim_waycount; i++ ) {
		if ( vpSim[i][set].valid && vpSim[i][set].pc == pc ) {
			hitbank=i;
			break;
		}
	}
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_getPrediction hitbank: " << hitbank << std::endl;
	if ( hitbank == -1 ) return false;
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_getPrediction hit: pc: " << std::hex << vpSim[hitbank][set].pc << std::dec << " conf: " << vpSim[hitbank][set].conf << " value: " << std::hex << vpSim[hitbank][set].value << std::dec << std::endl;
	if ( vpSim[hitbank][set].conf < vpmaxconf ) return false;
	predicted_value =  vpSim[hitbank][set].value;
	// update LRU
	if ( vpSim[hitbank][set].LRU != 0 ) {
		VPSIM_updateLRU(hitbank, set);
	}
	return true;
}


void VPSIM_updatePredictor (UInt64
		 seq_no,
		 UInt64
		 actual_addr, UInt64 actual_value, UInt64 actual_latency){

	int hitbank = -1;
	int set = VPSIM_getset(actual_addr);
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_updatePredictor pc: " << std::hex << actual_addr << std::dec << " value: " << std::hex << actual_value << std::dec << std::endl;
// find match
	for( int i=0; i < vpsim_waycount; i++ ) {
		if ( vpSim[i][set].pc == actual_addr ) {
			hitbank=i;
			break;
		}
	}
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_updatePredictor hitbank: " << hitbank  << std::endl;
	// we have hit
	if ( hitbank != -1 ) {
		// hit and match
		if ( vpSim[hitbank][set].value == actual_value ) {
			if (  Sim()->getConfig()->getVPdebug() )
				std::cout << "DEBUG: VPSIM_updatePredictor match value conf: " << vpSim[hitbank][set].conf  << std::endl;
			if ( vpSim[hitbank][set].conf != vpmaxconf ) {
				vpSim[hitbank][set].conf++;
			}
		} else {
			if (  Sim()->getConfig()->getVPdebug() )
				std::cout << "DEBUG: VPSIM_updatePredictor wrong value conf: " << vpSim[hitbank][set].value  << std::endl;
			vpSim[hitbank][set].value = actual_value;
			vpSim[hitbank][set].conf = 0;
		}
	}
	if ( hitbank == -1 ) {
		//find victim
		int victimbank = -1;
		int LRU = -1;
		for( int i=0; i < vpsim_waycount; i++ ) {
			if ( vpSim[i][set].valid == false ) {
				victimbank = i;
				break;
			}
			if (  Sim()->getConfig()->getVPdebug() )
				std::cout << "DEBUG: VPSIM_updatePredictor check LRU: set: " << set << " bank: " << i << " LRU NEW: " << vpSim[i][set].LRU << " OLD: " << LRU << std::endl;
			if ( vpSim[i][set].LRU > LRU ) {
				LRU = vpSim[i][set].LRU;
				victimbank = i;
			}
		}
		if (  Sim()->getConfig()->getVPdebug() )
			std::cout << "DEBUG: VPSIM_updatePredictor find victim: " << victimbank  << std::endl;
		if ( victimbank != -1 ) {
			if (  Sim()->getConfig()->getVPdebug() )
				std::cout << "DEBUG: VPSIM_updatePredictor victim replaced, PC: " << std::hex << vpSim[victimbank][set].pc << " victimbank: " << std::dec << victimbank << " set: " <<  set << " conf: " << vpSim[victimbank][set].conf << " valid: " << vpSim[victimbank][set].valid << " value: " << std::hex << vpSim[victimbank][set].value << " to: " << actual_value << std::dec << " conf: " << vpSim[victimbank][set].conf  << std::endl;
			vpSim[victimbank][set].pc = actual_addr;
			vpSim[victimbank][set].value = actual_value;
			vpSim[victimbank][set].conf = 0;
			vpSim[victimbank][set].valid = true;
			VPSIM_updateLRU(victimbank, set);
		}

	}
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "DEBUG: VPSIM_updatePredictor end"  << std::endl;

}
