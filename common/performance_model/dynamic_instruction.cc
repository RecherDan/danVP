#include "dynamic_instruction.h"
#include "instruction.h"
#include "allocator.h"
#include "core.h"
#include "branch_predictor.h"
#include "performance_model.h"

Allocator* DynamicInstruction::createAllocator()
{
   return new TypedAllocator<DynamicInstruction, 1024>();
}

DynamicInstruction::~DynamicInstruction()
{
   if (instruction->isPseudo())
      delete instruction;
}

SubsecondTime DynamicInstruction::getCost(Core *core)
{
   if (isBranch())
      return getBranchCost(core);
   else
      return instruction->getCost(core);
}
SubsecondTime DynamicInstruction::getUopCache(Core *core, bool *is_uopispredicted)
{
	//PerformanceModel *perf = core->getPerformanceModel();
	UopCache *uopcache = core->getUopCache();
	bool uopPrediction = false;
	if ( uopcache->isUopCacheValid() )
		uopPrediction = uopcache->PredictUop(eip, instruction->getbbhead());
	const ComponentPeriod *period = core->getDvfsDomain();
	*is_uopispredicted = uopPrediction;
	return static_cast<SubsecondTime>(*period);
}
SubsecondTime DynamicInstruction::getVPCost(Core *core, bool *p_is_mispredict, bool *is_GoodPredicted)
{
   //PerformanceModel *perf = core->getPerformanceModel();
   ValuePrediction *vp = core->getValuePrediction();
   UopCache *uopcache = core->getUopCache();
   //unsigned long long value = vpinfo.value;
   //unsigned long long& predicted_value = value;
   const ComponentPeriod *period = core->getDvfsDomain();
   //std::cout << " test " << std::endl;
   if ( instruction->getbbhead() < 10 ) {
	   std::cout << "DynamicInstruction::getVPCost_BBhead BBhead: " << std::dec << instruction->getbbhead() << std::endl;
	   return static_cast<SubsecondTime>(*period) * 1;
   }
   std::cout << "DynamicInstruction::getVPCost PC: " << std::hex << eip << " BBhead: " << instruction->getbbhead() << std::dec << " reg: " << vpinfo.regname << ": " << std::hex << vpinfo.value << std::endl;
   bool is_mispredict = false;
   bool good_prediction = false;
   std::tuple<bool, bool> prediction_results;
   if (vp->isOn() && vpinfo.isEligible) {
	   std::tie(is_mispredict,good_prediction) = ( vp->getPrediction(0,eip,0,vpinfo.value,vpinfo.NextPC, vpinfo.type, vpinfo.TypeName) );
	   //is_mispredict = std::get<0>(prediction_results);
   	   //good_prediction = std::get<0>(prediction_results);
   }
   //bool is_mispredict = core->accessBranchPredictor(eip, branch_info.taken, branch_info.target);
   UInt64 cost = is_mispredict ? vp->getMispredictPenalty() : 1;
   if ( is_mispredict ) {
		 std::cout << "VP penalty: " << std::dec << vp->getMispredictPenalty() << std::endl;
   }
   bool uopVPhavePrediction = false;
   bool uop_is_mispredict = false;
   bool uop_good_prediction = false;
   //if (is_mispredict)
     // *p_is_mispredict = is_mispredict;
   //if (is_GoodPredicted)
	//   *is_GoodPredicted = good_prediction;

   if ( uopcache->isUopCacheValid() ) {
	   std::tie(uop_is_mispredict,uop_good_prediction) = ( uopcache->getVPprediction(eip , instruction->getbbhead(), vpinfo.value) );
	   uopVPhavePrediction = (uop_is_mispredict || uop_good_prediction);
   }

   if ( uopcache->isUopCacheValid() && uopcache->getVPremovevpentry() && good_prediction ) {
		   uopcache->AddVPinfo(eip, instruction->getbbhead(), vpinfo.value);
	   	   vp->invalidateEntry(eip);
   }
   //*is_GoodPredicted = false;
   std::cout << "is_GoodPredicted address: " << std::hex << is_GoodPredicted << " value: " << *is_GoodPredicted << std::endl;
   *is_GoodPredicted = uopVPhavePrediction || good_prediction;
   *p_is_mispredict =  !(uopVPhavePrediction || good_prediction) && (is_mispredict || uop_is_mispredict);
   return static_cast<SubsecondTime>(*period) * cost;
}

SubsecondTime DynamicInstruction::getBranchCost(Core *core, bool *p_is_mispredict)
{
   PerformanceModel *perf = core->getPerformanceModel();
   BranchPredictor *bp = perf->getBranchPredictor();
   const ComponentPeriod *period = core->getDvfsDomain();

   bool is_mispredict = core->accessBranchPredictor(eip, branch_info.taken, branch_info.target);
   UInt64 cost = is_mispredict ? bp->getMispredictPenalty() : 1;

   if (p_is_mispredict)
      *p_is_mispredict = is_mispredict;

   return static_cast<SubsecondTime>(*period) * cost;
}

void DynamicInstruction::accessMemory(Core *core)
{
   for(UInt8 idx = 0; idx < num_memory; ++idx)
   {
      if (memory_info[idx].executed && memory_info[idx].hit_where == HitWhere::UNKNOWN)
      {
         MemoryResult res = core->accessMemory(
            /*instruction.isAtomic()
               ? (info->type == DynamicInstructionInfo::MEMORY_READ ? Core::LOCK : Core::UNLOCK)
               :*/ Core::NONE, // Just as in pin/lite/memory_modeling.cc, make the second part of an atomic update implicit
            memory_info[idx].dir == Operand::READ ? (instruction->isAtomic() ? Core::READ_EX : Core::READ) : Core::WRITE,
            memory_info[idx].addr,
            NULL,
            memory_info[idx].size,
            Core::MEM_MODELED_RETURN,
            instruction->getAddress()
         );
         memory_info[idx].latency = res.latency;
         memory_info[idx].hit_where = res.hit_where;
      }
      else
      {
         memory_info[idx].latency = 1 * core->getDvfsDomain()->getPeriod(); // 1 cycle latency
         memory_info[idx].hit_where = HitWhere::PREDICATE_FALSE;
      }
   }
}
