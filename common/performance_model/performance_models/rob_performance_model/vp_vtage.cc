#include "vp_vtage.h"
#define K8
#ifdef K8
// 8KB
// 4.026 //3.729 Stride only // 3.437 for TAGE  only
// 65378 bits
#define UWIDTH 2
#define LOGLDATA 7
#define LOGBANK 5
#define TAGWIDTH 11
#define NBBANK 47

#define NHIST 7
int HL[NHIST + 1] = { 0, 0, 1, 3, 6, 12, 18, 30 };
int seq_commit;
#define LOGSTR 4
#define NBWAYSTR 3
#define TAGWIDTHSTR 14
#define LOGSTRIDE 20
#endif


// 32KB //
//#define K32
#ifdef K32

// 4.202 //3.729 for stride only  //3.570 for VTAGE only
// 262018 bits
#define UWIDTH 2
#define LOGLDATA 9
#define LOGBANK 7
#define TAGWIDTH 11
#define NBBANK 49


#define NHIST 8
int HL[NHIST + 1] = { 0, 0, 3, 7, 15, 31, 63, 90, 127 };

#define LOGSTR 4
#define NBWAYSTR 3
#define TAGWIDTHSTR 14
#define LOGSTRIDE 20
#endif
//END 32 KB//




//UNLIMITED//
//#define LIMITSTUDY
#ifdef LIMITSTUDY
// 4.408 //3.730 Stride only // 3.732 for TAGE  only
#define UWIDTH 1
#define LOGLDATA 20
#define LOGBANK 20
#define TAGWIDTH 15
#define NBBANK 63



#define NHIST 14
int HL[NHIST + 1] =
  { 0, 0, 1, 3, 7, 15, 31, 47, 63, 95, 127, 191, 255, 383, 511 };
#define LOGSTR 20
#define TAGWIDTHSTR 15
#define LOGSTRIDE 30
#define NBWAYSTR 3

#endif
//END UNLIMITED //



#define WIDTHCONFID 3
#define MAXCONFID ((1<< WIDTHCONFID)-1)
#define WIDTHCONFIDSTR 5
#define MAXCONFIDSTR  ((1<< WIDTHCONFIDSTR)-1)
#define MAXU  ((1<< UWIDTH)-1)

#define BANKDATA (1<<LOGLDATA)
#define MINSTRIDE -(1<<(LOGSTRIDE-1))
#define MAXSTRIDE (-MINSTRIDE-1)
#define BANKSIZE (1<<LOGBANK)
#define PREDSIZE (NBBANK*BANKSIZE)

#define NOTLLCMISS (actual_latency < 150)
#define NOTL2MISS (actual_latency < 60)
#define NOTL1MISS (actual_latency < 12)
#define FASTINST (actual_latency ==1)
#define MFASTINST (actual_latency <3)



// Global path history

static UInt64 gpath[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/* using up to 512 bits of path history was found to result in some performance benefit : essentially in the unlimited case. I did not explore longer histories */

static UInt64 gtargeth = 0;
/* history of the targets : limited to  64 bits*/

// The E-Stride predictor
//entry in the stride predictor
struct strdata
{
  UInt64 LastValue;		//64 bits
  UInt64 Stride;		// LOGSTRIDE bits
  unsigned short conf;			// WIDTHCONFIDSTR bits
  unsigned int tag;			//TAGWIDTHSTR bits
  unsigned int NotFirstOcc;		//1 bits
  int u;			// 2 bits
  //67 + LOGSTRIDE + WIDTHCONFIDSTR + TAGWIDTHSTR bits
};
//static strdata STR[NBWAYSTR * (1 << LOGSTR)];


static int SafeStride = 0;	// 16 bits

/////////////////////////////////// For E-VTAGE
//the data values
struct longdata
{
  UInt64 data;
  unsigned short u;
};
static longdata LDATA[3 * BANKDATA];
//  managed as a a skewed associative array
//each entry is 64-LOGLDATA bits for the data (since the other bits can be deduced from the index) + 2 bits for u

//VTAGE
struct vtentry
{
  UInt64 hashpt;		// hash of the value + number of way in the value array ; LOGLDATA + 2 bits
  unsigned short conf;			//WIDTHCONFID bits
  unsigned int tag;			// TAGWIDTH bits
  unsigned short u;			//2 bits
  //LOGLDATA +4 +WIDTHCONFID +TAGWIDTH bits
};

static vtentry Vtage[PREDSIZE];

#define  MAXTICK 1024
static int TICK;		//10 bits // for managing replacement on the VTAGE entries
static int LastMispVT = 0;	//8 bits //for tracking the last misprediction on VTAGE

unsigned int
gi (int i, UInt64 pc)
{
  int hl = (HL[i] < 64) ? (HL[i] % 64) : 64;
  UInt64 inter = (hl < 64) ? (((1 << hl) - 1) & gpath[0]) : gpath[0];
  UInt64 res = 0;
  inter ^= (pc >> (i)) ^ (pc);

  for (int t = 0; t < 8; t++)
    {
      res ^= inter;
      inter ^= ((inter & 15) << 16);
      inter >>= (LOGBANK - ((NHIST - i + LOGBANK - 1) % (LOGBANK - 1)));
    }
  hl = (hl < (HL[NHIST] + 1) / 2) ? hl : ((HL[NHIST] + 1) / 2);

  inter ^= (hl < 64) ? (((1 << hl) - 1) & gtargeth) : gtargeth;
  for (int t = 0; t <= hl / LOGBANK; t++)
    {
      res ^= inter;
      inter ^= ((inter & 15) << 16);
      inter >>= LOGBANK;
    }

  if (HL[i] >= 64)
    {
      int REMAIN = HL[i] - 64;
      hl = REMAIN;
      int PT = 1;

      while (REMAIN > 0)
	{


	  inter ^= ((hl < 64) ? (((1 << hl) - 1) & gpath[PT]) : gpath[PT]);
	  for (int t = 0; t < 8; t++)
	    {
	      res ^= inter;
	      inter ^= ((inter & 15) << 16);

	      inter >>= (LOGBANK -
			 ((NHIST - i + LOGBANK - 1) % (LOGBANK - 1)));

	    }
	  REMAIN = REMAIN - 64;
	  PT++;
	}
    }
  return ((unsigned int) res & (BANKSIZE - 1));
}



//tags for VTAGE: just another complex hash function "orthogonal" to the index function
unsigned int
gtag (int i, UInt64 pc)
{
  int hl = (HL[i] < 64) ? (HL[i] % 64) : 64;
  UInt64 inter = (hl < 64) ? (((1 << hl) - 1) & gpath[0]) : gpath[0];

  UInt64 res = 0;
  inter ^= ((pc >> (i)) ^ (pc >> (5 + i)) ^ (pc));
  for (int t = 0; t < 8; t++)
    {
      res ^= inter;
      inter ^= ((inter & 31) << 14);
      inter >>= (LOGBANK - ((NHIST - i + LOGBANK - 2) % (LOGBANK - 1)));
    }
  hl = (hl < (HL[NHIST] + 1) / 2) ? hl : ((HL[NHIST] + 1) / 2);
  inter ^= ((hl < 64) ? (((1 << hl) - 1) & gtargeth) : gtargeth);
  for (int t = 0; t <= hl / TAGWIDTH; t++)
    {
      res ^= inter;
      inter ^= ((inter & 15) << 16);
      inter >>= TAGWIDTH;
    }

  if (HL[i] >= 64)
    {
      int REMAIN = HL[i] - 64;
      hl = REMAIN;
      int PT = 1;

      while (REMAIN > 0)
	{


	  inter ^= ((hl < 64) ? (((1 << hl) - 1) & gpath[PT]) : gpath[PT]);
	  for (int t = 0; t < 8; t++)
	    {
	      res ^= inter;
	      inter ^= ((inter & 31) << 14);
	      inter >>= (TAGWIDTH - (NHIST - i - 1));


	    }
	  REMAIN = REMAIN - 64;
	  PT++;
	}
    }

  return ((unsigned int) res & ((1 << TAGWIDTH) - 1));
}




////// for managing speculative state and forwarding information to the back-end
struct ForUpdate
{
  bool predvtage;
  bool predstride;
  bool preduop;
  bool prediction_result;
  unsigned short todo;
  UInt64 pc;
  unsigned int GI[NHIST + 1];
  unsigned int GTAG[NHIST + 1];
  int B[NBWAYSTR];
  int TAGSTR[NBWAYSTR];
  int STHIT;
  int HitBank;
  short INSTTYPE;
  short NbOperand;
  UInt64 PredictedValue;
};

#define MAXINFLIGHT 256
static ForUpdate Update[MAXINFLIGHT];	// there may be 256 instructions inflight


void
getPredVtage (ForUpdate * U, UInt64 & predicted_value)
{
  bool predvtage = false;
  UInt64 pc = U->pc;
  UInt64 PCindex = ((pc) ^ (pc >> 2) ^ (pc >> 5)) % PREDSIZE;
  UInt64 PCbank = (PCindex >> LOGBANK) << LOGBANK;
  int conf = 0;
  for (int i = 1; i <= NHIST; i++)
    {
      U->GI[i] = (gi (i, pc) + (PCbank + (i << LOGBANK))) % PREDSIZE;
      U->GTAG[i] = gtag (i, pc);
    }
  U->GTAG[0] = (pc ^ (pc >> 4) ^ (pc >> TAGWIDTH)) & ((1 << TAGWIDTH) - 1);
  U->GI[0] = PCindex;
  U->HitBank = -1;

  for (int i = NHIST; i >= 0; i--)
    {
      if (Vtage[U->GI[i]].tag == U->GTAG[i])
	{
	  U->HitBank = i;
	  break;
	}
    }

  if (LastMispVT >= 128)
// when a misprediction is encountered on VTAGE, we do not predict with VTAGE for 128 instructions;
// does not bring significant speed-up, but reduces the misprediction number significantly: mispredictions tend to be clustered
    if (U->HitBank >= 0)
      {
	int index = Vtage[U->GI[U->HitBank]].hashpt;
	if (index < 3 * BANKDATA)
	  {
	    // the hash and the data are both present
	    predicted_value = LDATA[index].data;
	    predvtage = ((Vtage[U->GI[U->HitBank]].conf >= MAXCONFID));
	    conf = Vtage[U->GI[U->HitBank]].conf;
	  }
      }
  U->predvtage = predvtage;
  std::cout << "get prediction, pc: " << pc << " pcindex: " << PCindex << " pc bank: " << PCbank << "  prediction: " << (predvtage ? "yes":"no") << " hit bank: " << U->HitBank << " confidence: " << conf << " predicted value: " << predicted_value << std::endl;
}
/////////Update of  VTAGE
// function determining whether to  update or not confidence on a correct prediction

bool
vtageupdateconf (ForUpdate * U, UInt64 actual_value, int actual_latency)
{
	//return true; // TODO: dan test
//	srand (time(NULL));
#define LOWVAL ((abs (2*((int64_t) actual_value)+1)<(1<<16))+ (actual_value==0))
#ifdef K8
#define updateconf ((random () & (((1 << (LOWVAL+NOTLLCMISS+2*FASTINST+NOTL2MISS+NOTL1MISS + ((U->INSTTYPE!=loadInstClass) ||NOTL1MISS)      + (U->HitBank > 1) ))- 1)))==0)
#else
#define updateconf ((random () & (((1 << (LOWVAL+NOTLLCMISS+2*FASTINST+NOTL2MISS+NOTL1MISS + ((U->INSTTYPE!=loadInstClass) ||NOTL1MISS)       ))- 1)))==0)
#endif


#ifdef LIMITSTUDY
#define UPDATECONF2 ((U->HitBank<=1) ? (updateconf || updateconf ) || (updateconf || updateconf ) : (updateconf || updateconf ))
#define UPDATECONF (UPDATECONF2 || UPDATECONF2)
#else
#ifdef K32
#define UPDATECONF (((U->HitBank<=1) ? (updateconf || updateconf) : updateconf))
#else
  // K8
#define UPDATECONF updateconf
#endif
#endif


//#define UPDATECONF ((rand() & (((1 << (LOWVAL+NOTLLCMISS+2*FASTINST+NOTL2MISS+NOTL1MISS + ((U->INSTTYPE!=loadInstClass) ||NOTL1MISS)      + (U->HitBank > 1) ))- 1)))==0)

  switch (U->INSTTYPE)
    {
    case aluInstClass:

    case fpInstClass:
    case slowAluInstClass:
    case undefInstClass:
    case loadInstClass:
    case storeInstClass:
      return (UPDATECONF);
      break;
    case uncondIndirectBranchInstClass:
      return (true);
      break;
    default:
      return (false);
    };
}

// Update of the U counter or not
bool
VtageUpdateU (ForUpdate * U, UInt64 actual_value, int actual_latency)
{
//return true; // TODO: dan test
#define UPDATEU ((!U->prediction_result) && ((random() & ((1<<( LOWVAL + 2*NOTL1MISS  + (U->INSTTYPE!=loadInstClass) + FASTINST + 2*(U->INSTTYPE==aluInstClass)*(U->NbOperand<2)))-1)) == 0))

  switch (U->INSTTYPE)
    {
    case aluInstClass:
    case fpInstClass:
    case slowAluInstClass:
    case undefInstClass:
    case loadInstClass:
    case storeInstClass:
      return (UPDATEU);
      break;
    case uncondIndirectBranchInstClass:
      return (true);
      break;
    default:
      return (false);
    };



}

bool
VtageAllocateOrNot (ForUpdate * U, UInt64 actual_value, int actual_latency,
		    bool MedConf)
{
  //return true; //TODO : fix why is not allocating
  bool X = false;


  switch (U->INSTTYPE)
    {
    case undefInstClass:
    case aluInstClass:
    case storeInstClass:
#ifndef LIMITSTUDY
      if (((U->NbOperand >= 2)
	   & ((random() & 15) == 0))
	  || ((U->NbOperand < 2) & ((random() & 63) == 0)))
#endif
    case fpInstClass:
    case slowAluInstClass:
    case loadInstClass:


#ifndef LIMITSTUDY
	X = (((random() &
#ifdef K8
	       ((4 <<
#else
	       //K32
	       ((2 <<
#endif
		 (
#ifdef K32
		   ((U->INSTTYPE != loadInstClass) || (NOTL1MISS)) *
#endif
		   LOWVAL + NOTLLCMISS + NOTL2MISS +
#ifdef K8
		   2 *
#endif
		   NOTL1MISS + 2 * FASTINST)) - 1)) == 0) ||
#ifdef K8
	     (((random() & 3) == 0) && MedConf));
#else
	     (MedConf));
#endif
#else
	X = true;
#endif

      break;
    case uncondIndirectBranchInstClass:
      X = true;
      break;
    default:
      X = false;

    };
  //X = true; // TODO: DAN TEST
  return (X);
}


void
UpdateVtagePred (ForUpdate * U, UInt64 actual_value, int actual_latency)
{

  bool MedConf = false;
  UInt64 HashData = ((actual_value ^ (actual_value >> 7) ^
			(actual_value >> 13) ^ (actual_value >> 21) ^
			(actual_value >> 29) ^ (actual_value >> 34) ^
			(actual_value >> 43) ^ (actual_value >> 52) ^
			(actual_value >> 57)) & (BANKDATA - 1)) +
    3 * BANKDATA;
  bool ShouldWeAllocate = true;
  std::cout <<" hitbank: " << std::dec <<  U->HitBank << std::endl;
  if (U->HitBank != -1)
    {
      // there was  an  hitting entry in VTAGE
      UInt64 index = U->GI[U->HitBank];
      std::cout << "index: " << index << " Vtage[index].tag : " << Vtage[index].tag << " U->GTAG[U->HitBank] " << U->GTAG[U->HitBank] <<  std::endl;
      // the entry may have dissappeared in the interval between the prediction and  the commit


      if (Vtage[index].tag == U->GTAG[U->HitBank])
	{
	  //  update the prediction
	  UInt64 indindex = Vtage[index].hashpt;
	  ShouldWeAllocate =
	    ((indindex >= 3 * BANKDATA) & (indindex != HashData))
	    || ((indindex < 3 * BANKDATA) &
		(LDATA[indindex].data != actual_value));
	  std::cout << "should we allocate: " << ShouldWeAllocate << std::endl;
	  if (!ShouldWeAllocate)
	    {
	      // the predicted result is satisfactory: either a good hash without data, or a pointer on the correct data
	      ShouldWeAllocate = false;
	      std::cout << "confidence before: " << Vtage[index].conf << std::endl;
	      if (Vtage[index].conf < MAXCONFID)
		if (vtageupdateconf (U, actual_value, actual_latency))
		  Vtage[index].conf++;

	      if (Vtage[index].u < MAXU)
		if ((VtageUpdateU (U, actual_value, actual_latency))
		    || (Vtage[index].conf == MAXCONFID))

		  Vtage[index].u++;
	      if (indindex < 3 * BANKDATA)
		if (LDATA[indindex].u < 3)
		  if (Vtage[index].conf == MAXCONFID)
		    LDATA[indindex].u++;


	      if (indindex >= 3 * BANKDATA)
		{

		  //try to allocate an effective data entry when the confidence has reached a reasonable level
		  if (Vtage[index].conf >= MAXCONFID - 1)
		    {
		      int X[3];
		      for (int i = 0; i < 3; i++)
			X[i] =
			  (((actual_value) ^
			    (actual_value >> (LOGLDATA + (i + 1))) ^
			    (actual_value >> (3 * (LOGLDATA + (i + 1)))) ^
			    (actual_value >> (4 * (LOGLDATA + (i + 1)))) ^
			    (actual_value >> (5 * (LOGLDATA + (i + 1)))) ^
			    (actual_value >> (6 * (LOGLDATA + (i + 1)))) ^
			    (actual_value >> (2 * (LOGLDATA + (i + 1))))) &
			   ((1 << LOGLDATA) - 1)) + i * (1 << LOGLDATA);
		      bool done = false;
		      for (int i = 0; i < 3; i++)
			{
			  if (LDATA[X[i]].data == actual_value)
			    {
			      //the data is already present
			      Vtage[index].hashpt = X[i];
			      done = true;
			      break;
			    }
			}
		      if (!done)
			if ((random() & 3) == 0)
			  {
			    //data absent: let us try try to steal an entry
			    int i = (((UInt64) random()) % 3);
			    bool done = false;
			    for (int j = 0; j < 3; j++)
			      {
				if ((LDATA[X[i]].u == 0))
				  {
				    LDATA[X[i]].data = actual_value;
				    LDATA[X[i]].u = 1;
				    Vtage[index].hashpt = X[i];
				    done = true;
				    break;
				  }
				i++;
				i = i % 3;

			      }
			    if (U->INSTTYPE == loadInstClass)
			      if (!done)
				{
				  if ((LDATA[X[i]].u == 0))
				    {
				      LDATA[X[i]].data = actual_value;
				      LDATA[X[i]].u = 1;
				      Vtage[index].hashpt = X[i];
				    }
				  else
#ifdef K8
				  if ((random() & 31) == 0)
#else
				  if ((random() & 3) == 0)
#endif
				    LDATA[X[i]].u--;
				}
			  }
		    }
		}


	      std::cout << "confidence: " << Vtage[index].conf << std::endl;
	    }

	  else
	    {
// misprediction: reset


	      Vtage[index].hashpt = HashData;
	      if ((Vtage[index].conf > MAXCONFID / 2)
		  || ((Vtage[index].conf == MAXCONFID / 2) &
		      (Vtage[index].u == 3))
		  || ((Vtage[index].conf > 0) &
		      (Vtage[index].conf < MAXCONFID / 2)))
		MedConf = true;

	      if (Vtage[index].conf == MAXCONFID)
		{

		  Vtage[index].u = (Vtage[index].conf == MAXCONFID);
		  Vtage[index].conf -= (MAXCONFID + 1) / 4;
		}
	      else
		{
		  Vtage[index].conf = 0;
		  Vtage[index].u = 0;
		}

	    }
	}
    }

  if (!U->prediction_result)
    //Don't waste your time allocating if it is predicted by the other component
    if (ShouldWeAllocate)
      {

// avoid allocating too often
	if (VtageAllocateOrNot (U, actual_value, actual_latency, MedConf))
	  {
		std::cout << "allocate" << std::endl;
	    int ALL = 0;
	    int NA = 0;
	    int DEP = (U->HitBank + 1) + ((random() & 7) == 0);
	    if (U->HitBank == 0)
	      DEP++;

	    if (U->HitBank == -1)
	      {
		if (random() & 7)
		  DEP = random() & 1;
		else
		  DEP = 2 + ((random() & 7) == 0);

	      }

	    if (DEP > 1)
	      {

		for (int i = DEP; i <= NHIST; i++)
		  {
		    int index = U->GI[i];
	    	 std::cout <<"before updating, u: " << Vtage[index].u << " hashpt: " <<  Vtage[index].hashpt << " tag: " << Vtage[index].tag  << std::endl;
		    if ((Vtage[index].u == 0)
			&& ((Vtage[index].conf == MAXCONFID / 2)
			    || (Vtage[index].conf <=
				(random() & MAXCONFID))))
//slightly favors the entries with real confidence
		      {
			Vtage[index].hashpt = HashData;
			Vtage[index].conf = MAXCONFID / 2;	//set to 3  for faster warming to  high confidence
			Vtage[index].tag = U->GTAG[i];
			ALL++;

			break;

		      }
		    else
		      {
			NA++;
		      }

		  }
	      }
	    else
	      {

		for (int j = 0; j <= 1; j++)
		  {
		    int i = (j + DEP) & 1;

		    int index = U->GI[i];
	    	 std::cout <<"before updating, u: " << Vtage[index].u << " hashpt: " <<  Vtage[index].hashpt << " tag: " << Vtage[index].tag  << std::endl;

		    if ((Vtage[index].u == 0)
			&& ((Vtage[index].conf == MAXCONFID / 2)
			    || (Vtage[index].conf <=
				(random() & MAXCONFID))))
		      {
			Vtage[index].hashpt = HashData;
			Vtage[index].conf = MAXCONFID / 2;
			if (U->NbOperand == 0)
			  if (U->INSTTYPE == aluInstClass)
			    Vtage[index].conf = MAXCONFID;
			Vtage[index].tag = U->GTAG[i];
			  std::cout <<"updating, tag: " << Vtage[index].tag << " hashpt: " <<  Vtage[index].hashpt << " conf: " << Vtage[index].conf << std::endl;
			ALL++;
			break;
		      }
		    else
		      {
			NA++;
		      }
		  }
	      }

#ifdef K8
	    TICK += NA - (3 * ALL);
#else
	    TICK += (NA - (5 * ALL));
#endif
	    if (TICK < 0)
	      TICK = 0;
	    if (TICK >= MAXTICK)
	      {

		for (int i = 0; i < PREDSIZE; i++)
		  if (Vtage[i].u > 0)
		    Vtage[i].u--;
		TICK = 0;
	      }
	  }

      }

  std::cout <<"updating, pc: " << U->pc << " Hitbank: " <<  U->HitBank << std::endl;
}

bool
Vtage_getPrediction (UInt64 seq_no, UInt64 pc, UInt64 piece,
	       UInt64 & predicted_value)
{

  ForUpdate *U;
  U = &Update[seq_no & (MAXINFLIGHT - 1)];
  U->pc = pc + piece;
  U->predvtage = false;
  U->predstride = false;



  getPredVtage (U, predicted_value);

// the two predictions are very rarely both high confidence; when they are pick the VTAGE prediction
  U->PredictedValue = predicted_value;
  return (U->predvtage);
}

void
Vtage_updatePredictor (UInt64
		 seq_no,
		 UInt64
		 actual_addr, UInt64 actual_value, UInt64 actual_latency)
{
  ForUpdate *U;
  U = &Update[seq_no & (MAXINFLIGHT - 1)];


  if (U->todo == 1)
    {
#ifdef LIMITSTUDY
      //just to force allocations and update on both predictors
      U->prediction_result = 0;
#endif

      UpdateVtagePred (U, actual_value, (int) actual_latency);

      U->todo = 0;
    }
  seq_commit = seq_no;

}




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
		   UInt64 src1, UInt64 src2, UInt64 src3, UInt64 dst)
{

  // the framework does not really allow  to filter the predictions, so we predict every instruction
  ForUpdate *U;
  U = &Update[seq_no & (MAXINFLIGHT - 1)];
  //std::cout << "update pc: " << U->pc << std::endl;
  LastMispVT++;
  if ( !eligible || prediction_result==0) {
  }
  if (eligible)
    {
      U->NbOperand =
	(src1 != 0xdeadbeef) + (src2 != 0xdeadbeef) + (src3 != 0xdeadbeef);
      U->todo = 1;
      U->INSTTYPE = insn;
      U->prediction_result = (prediction_result == 1);
      if (SafeStride < (1 << 15) - 1)
	SafeStride++;
      if (prediction_result != 2)
	{
	  if (prediction_result)
	    {
	      if (U->predstride)
		if (SafeStride < (1 << 15) - 1)
		  SafeStride += 4 * (1 + (insn == loadInstClass));
	    }
	  else
	    {
	      if (U->predvtage)
		LastMispVT = 0;
	      if (U->predstride)
		SafeStride -= 1024;
	    }
	}
    }

  bool isCondBr = insn == condBranchInstClass;
  bool isUnCondBr = insn == uncondIndirectBranchInstClass
    || insn == uncondDirectBranchInstClass;
//path history
  // just to have a longer history without (software) management
  if ((isCondBr) || (isUnCondBr))
    if (pc != next_pc - 4)
      {
	for (int i = 7; i > 0; i--)
	  gpath[i] = (gpath[i] << 1) ^ ((gpath[i - 1] >> 63) & 1);
	gpath[0] = (gpath[0] << 1) ^ (pc >> 2);
	gtargeth = (gtargeth << 1) ^ (next_pc >> 2);
      }
}
