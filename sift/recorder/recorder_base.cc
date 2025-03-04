#include "recorder_base.h"
#include "globals.h"
#include "threads.h"
#include "sift_assert.h"
#include "recorder_control.h"
#include "syscall_modeling.h"

#include <iostream>

VOID countInsns(THREADID threadid, INT32 count)
{
   thread_data[threadid].icount += count;

   if (!any_thread_in_detail && thread_data[threadid].output)
   {
      thread_data[threadid].icount_reported += count;
      if (thread_data[threadid].icount_reported > KnobFlowControlFF.Value())
      {
         Sift::Mode mode = thread_data[threadid].output->InstructionCount(thread_data[threadid].icount_reported);
         thread_data[threadid].icount_reported = 0;
         setInstrumentationMode(mode);
      }
   }

   if (thread_data[threadid].icount >= fast_forward_target && !in_roi && !KnobUseROI.Value() && !KnobMPIImplicitROI.Value())
   {
      if (KnobVerbose.Value())
         std::cerr << "[SIFT_RECORDER:" << app_id << ":" << thread_data[threadid].thread_num << "] Changing to detailed after " << thread_data[threadid].icount << " instructions" << std::endl;
      if (!thread_data[threadid].output)
         openFile(threadid);
      thread_data[threadid].icount = 0;
      in_roi = true;
      setInstrumentationMode(Sift::ModeDetailed);
   }
}

VOID sendInstruction(THREADID threadid, ADDRINT addr, UINT32 size, UINT32 num_addresses, BOOL is_branch, BOOL taken, BOOL is_predicate, BOOL executing, BOOL isbefore, BOOL ispause, const CONTEXT * ctxt, UINT32 dstreg, uint64_t dstregval/*, bool ISR14WRITE, bool ISR15WRITE, UINT32 R14VAL, UINT32 R15VAL */, ADDRINT bbhead)
{
   // We're still called for instructions in the same basic block as ROI end, ignore these
   if (!thread_data[threadid].output)
   {
      thread_data[threadid].num_dyn_addresses = 0;
      return;
   }

   ++thread_data[threadid].icount;
   ++thread_data[threadid].icount_detailed;


   // Reconstruct basic blocks (we could ask Pin, but do it the same way as TraceThread will do it)
   if (thread_data[threadid].bbv_end || thread_data[threadid].bbv_last != addr)
   {
      // We're the start of a new basic block
      thread_data[threadid].bbv->count(thread_data[threadid].bbv_base, thread_data[threadid].bbv_count);
      thread_data[threadid].bbv_base = addr;
      thread_data[threadid].bbv_count = 0;
   }
   thread_data[threadid].bbv_count++;
   thread_data[threadid].bbv_last = addr + size;
   // Force BBV end on non-taken branches
   thread_data[threadid].bbv_end = is_branch;


   sift_assert(thread_data[threadid].num_dyn_addresses == num_addresses);
   if (isbefore)
   {
      for(uint8_t i = 0; i < num_addresses; ++i)
      {
         // If the instruction hasn't executed yet, access the address to ensure a page fault if the mapping wasn't set up yet
         static char dummy = 0;
         dummy += *(char *)translateAddress(thread_data[threadid].dyn_addresses[i], 0);
      }
   }

#ifdef VPDEBUG
   std::cout << "sendInstruction PC: " << std::hex << addr << " reg: " << std::dec << dstreg << " val: " << std::hex << dstregval << std::endl;
#endif
   //if ( (R14VAL == 0x1212 || R14VAL == 0x1213 )  && ISR14WRITE )
//	   std::cout << "R14 WRITE PC: " << std::hex << addr << " R14: " << R14VAL << " tst(" << std::dec << (int)REG_R14 << "): " << std::hex << dstval[(int)REG_R14] << std::dec << std::endl;
 //  if (  (R15VAL == 0x1212 || R15VAL == 0x1213 ) && ISR15WRITE )
	//   std::cout << "R15 WRITE PC: " << std::hex << addr << " R15: " << R15VAL << " tst(" << std::dec  << (int)REG_R15 << "): " << std::hex << dstval[(int)REG_R15] <<  std::dec << std::endl;
   /*
   std::cout << "RCX: " << (int)REG_RCX << std::endl;
   std::cout << "RBP: " << (int)REG_RBP << std::endl;
   std::cout << "RAX: " << (int)REG_RAX << std::endl;
   std::cout << "RDX: " << (int)REG_RDX << std::endl;
   std::cout << "RDI: " << (int)REG_RDI << std::endl;
   std::cout << "RSI: " << (int)REG_RSI << std::endl;
   std::cout << "RBX: " << (int)REG_RBX << std::endl;
   std::cout << "RSP: " << (int)REG_RSP << std::endl;
   std::cout << "R8: " << (int)REG_R8 << std::endl;
   std::cout << "R8: " << (int)REG_R15 << std::endl;
   */
   /*
   dstval[0] = PIN_GetContextReg(ctxt, REG_RCX); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[1] = PIN_GetContextReg(ctxt, REG_RBP); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[2] = PIN_GetContextReg(ctxt, REG_RAX); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[3] = PIN_GetContextReg(ctxt, REG_RDX); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[4] = PIN_GetContextReg(ctxt, REG_RDI); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[5] = PIN_GetContextReg(ctxt, REG_RSI); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[6] = PIN_GetContextReg(ctxt, REG_RBX); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[7] = PIN_GetContextReg(ctxt, REG_RSP); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[8] = PIN_GetContextReg(ctxt, REG_R8); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[9] = PIN_GetContextReg(ctxt, REG_R9); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[10] = PIN_GetContextReg(ctxt, REG_R10); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[11] = PIN_GetContextReg(ctxt, REG_R11); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[12] = PIN_GetContextReg(ctxt, REG_R12); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[13] = PIN_GetContextReg(ctxt, REG_R13); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[14] = PIN_GetContextReg(ctxt, REG_R14); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   dstval[15] = PIN_GetContextReg(ctxt, REG_R15); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   */
   //std::cout << "PV: " << addr <<  " RAX val: " << std::hex << dstval[(int)REG_RAX] << std::endl;
   //dstval[16] = PIN_GetContextReg(ctxt, REG_ZMM0); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[17] = PIN_GetContextReg(ctxt, REG_ZMM1); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[18] = PIN_GetContextReg(ctxt, REG_ZMM2); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[19] = PIN_GetContextReg(ctxt, REG_ZMM3); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[20] = PIN_GetContextReg(ctxt, REG_ZMM4); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[21] = PIN_GetContextReg(ctxt, REG_ZMM5); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[22] = PIN_GetContextReg(ctxt, REG_ZMM6); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[23] = PIN_GetContextReg(ctxt, REG_ZMM7); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[24] = PIN_GetContextReg(ctxt, REG_ZMM8); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
   //dstval[25] = PIN_GetContextReg(ctxt, REG_ZMM9); // TODO: make array of registers and save all values - then in micro_op get the relevant value.
	  std::cout << "recorder base: TST: pc: " << std::hex << addr << " HEAD: " << thread_data[threadid].bbv_base << std::dec << std::endl;

   thread_data[threadid].output->Instruction(addr, size, num_addresses, thread_data[threadid].dyn_addresses, is_branch, taken, is_predicate, executing, dstregval, 11, thread_data[threadid].bbv_base);
   thread_data[threadid].num_dyn_addresses = 0;

   if (KnobUseResponseFiles.Value() && KnobFlowControl.Value() && (thread_data[threadid].icount > thread_data[threadid].flowcontrol_target || ispause))
   {
      Sift::Mode mode = thread_data[threadid].output->Sync();
      thread_data[threadid].flowcontrol_target = thread_data[threadid].icount + KnobFlowControl.Value();
      setInstrumentationMode(mode);
   }

   if (detailed_target != 0 && thread_data[threadid].icount_detailed >= detailed_target)
   {
      closeFile(threadid);
      PIN_Detach();
      return;
   }

   if (blocksize && thread_data[threadid].icount >= blocksize)
   {
      openFile(threadid);
      thread_data[threadid].icount = 0;
   }
}

VOID cacheOnlyUpdateInsCount(THREADID threadid, UINT32 icount)
{
   thread_data[threadid].icount_cacheonly_pending += icount;
}

VOID cacheOnlyConsumeAddresses(THREADID threadid)
{
   thread_data[threadid].num_dyn_addresses = 0;
}

VOID sendCacheOnly(THREADID threadid, UINT32 icount, UINT32 type, ADDRINT eip, ADDRINT arg)
{
   // We're still called for instructions in the same basic block as ROI end, ignore these
   if (!thread_data[threadid].output)
      return;

   cacheOnlyUpdateInsCount(threadid, icount);

   ADDRINT address;
   switch(Sift::CacheOnlyType(type))
   {
      case Sift::CacheOnlyMemRead:
      case Sift::CacheOnlyMemWrite:
         address = thread_data[threadid].dyn_addresses[arg];
         break;
      default:
         address = arg;
         break;
   }
   thread_data[threadid].output->CacheOnly(thread_data[threadid].icount_cacheonly_pending, Sift::CacheOnlyType(type), eip, address);

   thread_data[threadid].icount_cacheonly += thread_data[threadid].icount_cacheonly_pending;
   thread_data[threadid].icount_reported += thread_data[threadid].icount_cacheonly_pending;
   thread_data[threadid].icount_cacheonly_pending = 0;

   if (thread_data[threadid].icount_reported > KnobFlowControlFF.Value())
   {
      Sift::Mode mode = thread_data[threadid].output->Sync();
      thread_data[threadid].icount_reported = 0;
      setInstrumentationMode(mode);
   }
}

VOID handleMemory(THREADID threadid, ADDRINT address)
{
   // We're still called for instructions in the same basic block as ROI end, ignore these
   if (!thread_data[threadid].output)
      return;

   thread_data[threadid].dyn_addresses[thread_data[threadid].num_dyn_addresses++] = address;
}

UINT32 addMemoryModeling(INS ins)
{
   UINT32 num_addresses = 0;

   if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
   {
      for (unsigned int i = 0; i < INS_MemoryOperandCount(ins); i++)
      {
         INS_InsertCall(ins, IPOINT_BEFORE,
               AFUNPTR(handleMemory),
               IARG_THREAD_ID,
               IARG_MEMORYOP_EA, i,
               IARG_END);
         num_addresses++;
      }
   }
   sift_assert(num_addresses <= Sift::MAX_DYNAMIC_ADDRESSES);

   return num_addresses;
}

VOID insertCall(INS ins, IPOINT ipoint, UINT32 num_addresses, BOOL is_branch, BOOL taken, INS bbhead)
{
	#ifdef VPDEBUG
		if ( INS_RegW(ins, 0) == REG_R14D ) {
			std::cout << std::hex << INS_Address(ins) << ": R14 disasembly " << std::dec << INS_Disassemble(ins) << std::endl;
		}
		if ( INS_RegW(ins, 0) == REG_R15D ) {
			std::cout << std::hex << INS_Address(ins) << ": R15 disasembly " << std::dec <<  INS_Disassemble(ins) << std::endl;
		}
	#endif
   INS_InsertCall(ins, ipoint,
      AFUNPTR(sendInstruction),
      IARG_THREAD_ID,
      IARG_ADDRINT, INS_Address(ins),
      IARG_UINT32, UINT32(INS_Size(ins)),
      IARG_UINT32, num_addresses,
      IARG_BOOL, is_branch,
      IARG_BOOL, taken,
      IARG_BOOL, INS_IsPredicated(ins),
      IARG_EXECUTING,
      IARG_BOOL, ipoint == IPOINT_BEFORE,
      IARG_BOOL, INS_Opcode(ins) == XED_ICLASS_PAUSE,
	  IARG_CONTEXT,
	  IARG_UINT32, INS_RegW(ins, 0),
	  IARG_REG_VALUE, ( (INS_RegW(ins, 0) != REG_INVALID_) && (INS_RegW(ins,0) < REG_XMM0)) ? INS_RegW(ins, 0) : REG_R14D,
	  /*
	  IARG_BOOL, INS_RegW(ins, 0) == REG_R14D,
	  IARG_BOOL, INS_RegW(ins, 0) == REG_R15D,
	  IARG_REG_VALUE, REG_R14D,
	  IARG_REG_VALUE, REG_R15D,*/
	  IARG_ADDRINT, INS_Address(bbhead),
      IARG_END);
}

static VOID traceCallback(TRACE trace, void *v)
{
   // to not add overhead when extrae is linked, we must ignore extrae instr.
   if (extrae_image.linked)
   {
        ADDRINT trace_address = TRACE_Address(trace);
        if (trace_address >= extrae_image.top_addr &&
                trace_address < extrae_image.bottom_addr)
        {
            return;
        }
    }

   BBL bbl_head = TRACE_BblHead(trace);

   for (BBL bbl = bbl_head; BBL_Valid(bbl); bbl = BBL_Next(bbl))
   {
      for(INS ins = BBL_InsHead(bbl); ; ins = INS_Next(ins))
      {
         // Simics-style magic instruction: xchg bx, bx
         if (INS_IsXchg(ins) && INS_OperandReg(ins, 0) == REG_BX && INS_OperandReg(ins, 1) == REG_BX)
         {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)handleMagic, IARG_RETURN_REGS, REG_GAX, IARG_THREAD_ID, IARG_CONTEXT, IARG_REG_VALUE, REG_GAX,
#ifdef TARGET_IA32
                                     IARG_REG_VALUE, REG_GDX,
#else
                                     IARG_REG_VALUE, REG_GBX,
#endif
                                     IARG_REG_VALUE, REG_GCX, IARG_END);
         }

         // Handle emulated syscalls
         if (INS_IsSyscall(ins))
         {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(emulateSyscallFunc), IARG_THREAD_ID, IARG_CONST_CONTEXT, IARG_END);
         }

         if (KnobStopAddress && (INS_Address(ins) == KnobStopAddress))
         {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(endROI), IARG_THREAD_ID, IARG_END);
         }

         if (ins == BBL_InsTail(bbl))
            break;
      }

      if (!any_thread_in_detail)
      {
         BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)countInsns, IARG_THREAD_ID, IARG_UINT32, BBL_NumIns(bbl), IARG_END);
      }
      else if (current_mode == Sift::ModeDetailed)
      {
         for(INS ins = BBL_InsHead(bbl); ; ins = INS_Next(ins))
         {
            // For memory instructions, collect all addresses at IPOINT_BEFORE
            UINT32 num_addresses = addMemoryModeling(ins);

            bool is_branch = INS_IsBranch(ins) && INS_HasFallThrough(ins);

            if (is_branch)
            {
               insertCall(ins, IPOINT_AFTER,        num_addresses, true  /* is_branch */, false /* taken */, BBL_InsHead(bbl));
               insertCall(ins, IPOINT_TAKEN_BRANCH, num_addresses, true  /* is_branch */, true  /* taken */, BBL_InsHead(bbl));
            }
            else
            {
               // Whenever possible, use IPOINT_AFTER as this allows us to process addresses after the application has used them.
               // This ensures that their logical to physical mapping has been set up.
               insertCall(ins, INS_HasFallThrough(ins) ? IPOINT_AFTER : IPOINT_BEFORE, num_addresses, false /* is_branch */, false /* taken */, BBL_InsHead(bbl));
            }

            if (ins == BBL_InsTail(bbl))
               break;
         }
      }
      else if (current_mode == Sift::ModeMemory)
      {
         UINT32 inscount = 0;

         for(INS ins = BBL_InsHead(bbl); ; ins = INS_Next(ins))
         {
            ++inscount;

            // For memory instructions, collect all addresses at IPOINT_BEFORE
            addMemoryModeling(ins);

            if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
            {
               // Prefer IPOINT_AFTER to maximize probability of physical mapping to be available
               IPOINT ipoint = INS_HasFallThrough(ins) ? IPOINT_AFTER : IPOINT_BEFORE;
               for (unsigned int idx = 0; idx < INS_MemoryOperandCount(ins); ++idx)
               {
                  if (INS_MemoryOperandIsRead(ins, idx))
                  {
                     INS_InsertCall(ins, ipoint,
                           AFUNPTR(sendCacheOnly),
                           IARG_THREAD_ID,
                           IARG_UINT32, inscount,
                           IARG_UINT32, UINT32(Sift::CacheOnlyMemRead),
                           IARG_ADDRINT, INS_Address(ins),
                           IARG_UINT32, UINT32(idx),
                           IARG_END);
                     inscount = 0;
                  }
                  if (INS_MemoryOperandIsWritten(ins, idx))
                  {
                     INS_InsertCall(ins, ipoint,
                           AFUNPTR(sendCacheOnly),
                           IARG_THREAD_ID,
                           IARG_UINT32, inscount,
                           IARG_UINT32, UINT32(Sift::CacheOnlyMemWrite),
                           IARG_ADDRINT, INS_Address(ins),
                           IARG_UINT32, UINT32(idx),
                           IARG_END);
                     inscount = 0;
                  }
               }
               INS_InsertCall(ins, ipoint,
                     AFUNPTR(cacheOnlyConsumeAddresses),
                     IARG_THREAD_ID,
                     IARG_END);
            }
            if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
            {
               INS_InsertCall(ins, IPOINT_TAKEN_BRANCH,
                  AFUNPTR(sendCacheOnly),
                  IARG_THREAD_ID,
                  IARG_UINT32, inscount,
                  IARG_UINT32, UINT32(Sift::CacheOnlyBranchTaken),
                  IARG_ADDRINT, INS_Address(ins),
                  IARG_BRANCH_TARGET_ADDR,
                  IARG_END);
               INS_InsertCall(ins, IPOINT_AFTER,
                  AFUNPTR(sendCacheOnly),
                  IARG_THREAD_ID,
                  IARG_UINT32, inscount,
                  IARG_UINT32, Sift::CacheOnlyBranchNotTaken,
                  IARG_ADDRINT, INS_Address(ins),
                  IARG_FALLTHROUGH_ADDR,
                  IARG_END);
               inscount = 0;
            }

            if (ins == BBL_InsTail(bbl))
            {
               if (inscount)
                  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(cacheOnlyUpdateInsCount), IARG_THREAD_ID, IARG_UINT32, inscount, IARG_END);
               break;
            }
         }
      }
   }
}

void extraeImgCallback(IMG img, void * args)
{
    using namespace std;
    string img_name = IMG_Name(img);

    if (!extrae_image.linked)
    {
        extrae_image.linked = img_name.find("libmpitrace") != string::npos;
        if(extrae_image.linked)
        {
            extrae_image.top_addr=IMG_LowAddress(img);
            extrae_image.bottom_addr=IMG_HighAddress(img);

            if (KnobVerbose.Value())
               cerr << "[SIFT_RECORDER:" << app_id << "] Extrae has been detected."
                    << "[0x" << hex << extrae_image.top_addr << ", 0x" << hex << extrae_image.bottom_addr << "]" << endl;
        }
    }
}

void initRecorderBase()
{
   TRACE_AddInstrumentFunction(traceCallback, 0);

   extrae_image.linked = false;

   if (KnobExtraePreLoaded.Value() != 0)
   {
      IMG_AddInstrumentFunction(extraeImgCallback, 0);
   }
}
