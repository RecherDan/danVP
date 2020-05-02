/*
 * vp_simple.h
 *
 *  Created on: May 3, 2019
 *      Author: dan
 */

#ifndef COMMON_PERFORMANCE_MODEL_PERFORMANCE_MODELS_ROB_PERFORMANCE_MODEL_VP_SIMPLE_H_
#define COMMON_PERFORMANCE_MODEL_PERFORMANCE_MODELS_ROB_PERFORMANCE_MODEL_VP_SIMPLE_H_
#include <iostream>
#include <inttypes.h>
#include "fixed_types.h"
#define SETSIZE 10
#define OFFSET 5
#define VPSIMNUMOFENTRIES 2^SETSIZE
#define VPSIMWAYCOUNT 4
#define MAXCONF 7
#define VPSIMPLEDEBUG



// structure
struct VPSIM_ENTRY {
	bool valid;
	UInt64 pc;
	int conf;
	UInt64 value;
	int LRU;
};

void
VPSIM_updateLRU(int hitbank, int set);
// prediction
bool
VPSIM_getPrediction (UInt64 seq_no, UInt64 pc, UInt64 piece,
	       UInt64 & predicted_value);
//update
void
VPSIM_updatePredictor (UInt64
		 seq_no,
		 UInt64
		 actual_addr, UInt64 actual_value, UInt64 actual_latency);
bool VPSIM_invalidateEntry(UInt64 actual_addr);
void VPSIM_setglobals(int a);


#endif /* COMMON_PERFORMANCE_MODEL_PERFORMANCE_MODELS_ROB_PERFORMANCE_MODEL_VP_SIMPLE_H_ */
