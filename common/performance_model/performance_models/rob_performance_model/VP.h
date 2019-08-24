#ifndef __VALUEPREDICTION_H
#define __VALUEPREDICTION_H
#include "dynamic_instruction.h"
#include "instruction.h"
#include "allocator.h"
#include "core.h"
#include "fixed_types.h"
#include "vp_simple.h"
#include "vp_vtage.h"
#include <vector>
#include <deque>
//#define VPDEBUG


enum VPTYPE
{
	   DISABLE,
	   VP_SIMPLE,
	   VP_VTAGE
};







class ValuePrediction {
	private:
		bool ison = true;
		VPTYPE m_vptype;
		UInt64 m_mispredict_penalty;
		bool m_vp_debug = false;

		  //stats:
		  UInt64 VP_hits;
		  UInt64 VP_miss;
		  UInt64 VP_access;
		  UInt64 VP_haveprediction;
		  UInt64 VP_Invalidate;

	public:
		ValuePrediction(Core* core);
	    ~ValuePrediction();
	    std::tuple<bool, bool> getPrediction(UInt64 seq_no, UInt64 pc, unsigned short piece, UInt64 real_value, UInt64 nextPC, InstructionType type, String TypeName);
		void speculativeUpdate(UInt64 seq_no,    		// dynamic micro-instruction # (starts at 0 and increments indefinitely)
							   bool eligible,			// true: instruction is eligible for value prediction. false: not eligible.
							   unsigned short prediction_result,	// 0: incorrect, 1: correct, 2: unknown (not revealed)
								// Note: can assemble local and global branch history using pc, next_pc, and insn.
							   UInt64 pc,
							   UInt64 next_pc,
							   InstClass insn,
							   unsigned short piece,
							   // Note: up to 3 logical source register specifiers, up to 1 logical destination register specifier.
							   // 0xdeadbeef means that logical register does not exist.
							   // May use this information to reconstruct architectural register file state (using log. reg. and value at updatePredictor()).
							   UInt64 src1,
							   UInt64 src2,
							   UInt64 src3,
							   UInt64 dst);

		void updatePredictor(UInt64 seq_no,		// dynamic micro-instruction #
							 UInt64 actual_addr,	// load or store address (0xdeadbeef if not a load or store instruction)
							 UInt64 actual_value,	// value of destination register (0xdeadbeef if instr. is not eligible for value prediction)
							 UInt64 actual_latency);	// actual execution latency of instruction

		bool isOn();
		bool isDebug() { return this->m_vp_debug; };
		bool invalidateEntry(UInt64 pc);
		UInt64 getMispredictPenalty() { return m_mispredict_penalty; };

};

#endif // __VALUEPREDICTION_H
