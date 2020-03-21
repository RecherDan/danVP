#include "VP.h"
#include "simulator.h"
#include "stats.h"



std::tuple<bool, bool> ValuePrediction::getPrediction(UInt64 seq_no, UInt64 pc, unsigned short piece, UInt64 real_value, UInt64 nextPC, InstructionType instype, String TypeName)
{
	// get predicted value
	UInt64 value = 0;
	UInt64& predict = value;
	bool mispredict = false;
	bool good_prediction = false;
	bool havePrediction = false;
	std::tuple<bool, bool> retfalse{false, false};
	if ( m_vptype == DISABLE) return retfalse;
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "ValuePrediction::getPrediction PC: " << std::hex << pc << " next pc: " << nextPC << " real_value: " << std::hex << real_value << std::dec << " type: " << TypeName  << " type: " << instype << std::endl;
	if ( m_vptype == VP_VTAGE)
		havePrediction = Vtage_getPrediction(seq_no, pc, piece, predict);

	if ( m_vptype == VP_SIMPLE)
		havePrediction = VPSIM_getPrediction(seq_no, pc, piece, predict);

	if ( havePrediction && real_value != predict ) {
		mispredict = true;
	}
	if ( havePrediction && real_value == predict ) {
		good_prediction = true;
	}
	if (  Sim()->getConfig()->getVPdebug() )
		std::cout << "ValuePrediction::getPrediction prediction - haveprediction: " << (havePrediction? "yes":"no") << " predicted_value: " << std::hex << predict << std::dec << " mispredict: " << (mispredict? "yes":"no") << " good_prediction: " << (good_prediction? "yes":"no") << std::endl;
	if ( Sim()->getConfig()->getVPdebug() )
		std::cout << "ValuePrediction::getPrediction prediction sum: IP: " << std::hex << pc << std::dec << " miss: " << mispredict << " good: " << good_prediction << " pred value: " << std::hex << predict << "(" << real_value << ")" <<  std::dec << std::endl;

	//std::cout << "Value prediction is on: PC: " << pc << " next pc: " << nextPC << " type: " << TypeName  << " type: " << instype << " have prediction? " << (havePrediction? "yes":"no") << " predicted value:"  << std::hex << predict << " real_value: " << real_value << " mispredict? "<< (mispredict ? "yes" : "no") << std::dec <<  std::endl;
	InstClass type = aluInstClass;
	//update prediction
	if ( m_vptype == VP_VTAGE) {
		Vtage_speculativeUpdate(seq_no, true, (havePrediction ? (mispredict?0:1):2), pc, nextPC, type, 0,0,0,0,0);
		Vtage_updatePredictor(seq_no, pc, real_value, 100); //TODO: set latency to better number
	}
	if ( m_vptype == VP_SIMPLE)
		VPSIM_updatePredictor(seq_no, pc, real_value, 100); //TODO: set latency to better number

	// TODO: remove false mispredict:
	//if ( random()%10 == 1 && good_prediction ) {
	//	mispredict = true;
	//	good_prediction = false;
	//}
	// no mispediction: mispSim()->getConfig()->getVPdebug()redict= false;
	//std::cout << "mispredict: " << mispredict << " good_prediction: " << good_prediction << std::endl;
	this->VP_access++;
	if (good_prediction ) this->VP_hits++;
	if (mispredict ) { this->VP_miss++; this->VP_miss_penalty+=this->m_mispredict_penalty; std::cout << "increasing penalty: " << this->VP_miss_penalty << " by: " << this->m_mispredict_penalty << std::endl; }
	if ( mispredict || good_prediction ) this->VP_haveprediction++;
	std::tuple<bool, bool> retresult{mispredict, good_prediction};

	return retresult;
}
void ValuePrediction::speculativeUpdate(UInt64 seq_no,    		// dynamic micro-instruction # (starts at 0 and increments indefinitely)
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
							   UInt64 dst)
{

	return;
}

void ValuePrediction::updatePredictor(UInt64 seq_no,		// dynamic micro-instruction #
							 UInt64 actual_addr,	// load or store address (0xdeadbeef if not a load or store instruction)
							 UInt64 actual_value,	// value of destination register (0xdeadbeef if instr. is not eligible for value prediction)
							 UInt64 actual_latency)	// actual execution latency of instruction
{
	return;		
}
bool ValuePrediction::isOn() {
	return this->ison;
}
bool ValuePrediction::invalidateEntry(UInt64 pc) {
	this->VP_Invalidate++;
	if ( m_vptype == VP_SIMPLE)
		return VPSIM_invalidateEntry(pc);
	return false;
}
ValuePrediction::ValuePrediction(Core* core) {
	  VP_hits = 0;
	  VP_miss = 0;
	  VP_miss_penalty = 0;
	  VP_access = 0;
	  VP_haveprediction = 0;
	  VP_Invalidate = 0;
	   registerStatsMetric("VP", core->getId(), "VP_hits", &VP_hits);
	   registerStatsMetric("VP", core->getId(), "VP_miss", &VP_miss);
	   registerStatsMetric("VP", core->getId(), "VP_miss_penalty", &VP_miss_penalty);
	   registerStatsMetric("VP", core->getId(), "VP_access", &VP_access);
	   registerStatsMetric("VP", core->getId(), "VP_haveprediction", &VP_haveprediction);
	   registerStatsMetric("VP", core->getId(), "VP_Invalidate", &VP_Invalidate);
	m_vp_debug = false;
	if (Sim()->getConfig()->getVPdebug() )
		m_vp_debug=true;
	if ( Sim()->getConfig()->getVPtype() == Config::DISABLE ) {
		std::cout << "VALUE Predictior: DISABLED" << std::endl;
		m_vptype = DISABLE;
	}
	if ( Sim()->getConfig()->getVPtype() == Config::VP_SIMPLE ) {
		std::cout << "VALUE Predictior: SIMPLE" << std::endl;
		m_vptype = VP_SIMPLE;
	}
	if ( Sim()->getConfig()->getVPtype() == Config::VP_VTAGE ) {
		std::cout << "VALUE Predictior: VTAGE" << std::endl;
		m_vptype = VP_VTAGE;
	}
    this->m_mispredict_penalty = 100;
    this->m_mispredict_penalty = Sim()->getConfig()->getVPpenalty();
	return;	
	}
ValuePrediction::~ValuePrediction() {
return;	
	}
