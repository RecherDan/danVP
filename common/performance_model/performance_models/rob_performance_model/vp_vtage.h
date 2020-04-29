#ifndef __VPVTAGE_H
#define __VPVTAGE_H
#include "dynamic_instruction.h"
#include "instruction.h"
#include "allocator.h"
#include "core.h"
#include "fixed_types.h"
#include "vp_simple.h"
#include <vector>
#include <deque>
#include <inttypes.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

enum InstClass : unsigned short;





bool
Vtage_getPrediction (UInt64 seq_no, UInt64 pc, UInt64 piece,
	       UInt64 & predicted_value);

void
Vtage_speculativeUpdate (UInt64 seq_no,	// dynamic micro-instruction # (starts at 0 and increments indefinitely)
		   bool eligible,	// true: instruction is eligible for value prediction. false: not eligible.
		   uint8_t prediction_result,	// 0: incorrect, 1: correct, 2: unknown (not revealed)
		   // Note: can assemble local and global branch history using pc, next_pc, and insn.
		   UInt64
		   pc, UInt64 next_pc, InstClass insn, uint8_t piece,
		   // Note: up to 3 logical source register specifiers, up to 1 logical destination register specifier.
		   // 0xdeadbeef means that logical register does not exist.
		   // May use this information to reconstruct architectural register file state (using log. reg. and value at updatePredictor()).
		   UInt64 src1, UInt64 src2, UInt64 src3, UInt64 dst);

void
Vtage_updatePredictor (UInt64
		 seq_no,
		 UInt64
		 actual_addr, UInt64 actual_value, UInt64 actual_latency);
void setglobals(int a);
#endif // __VALUEPREDICTION_H
