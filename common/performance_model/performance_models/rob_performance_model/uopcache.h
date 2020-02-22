#ifndef __UOPCACHE_H
#define __UOPCACHE_H
#define UOPDEBUG
#include <deque>
#include "fixed_types.h"


#define UOPCACHELINES 64000
#define UOPWAYS 8
#define UOPCONFLIMIT 0
#define MAXVPINFO 10

struct vpinfo
{
	bool valid;
	bool validpredict;
	unsigned long  pc;
	unsigned long value;
	int blacklist;
};

struct uopline
{
  bool valid;
  bool loaded;
  int LRU;
  bool ready;
  unsigned long  pc;
  unsigned long  seq_no;
  unsigned long  PredictedValue;
  unsigned long usedcount;
  short blackconf;
  short conf;
  unsigned long  gpath;
  vpinfo VPinfo[MAXVPINFO];
};

class UopCache {
   private:
	  int priod = 0;
	  int numways = 8;
	  int priodLimit = 12;
	  long hashnum = UOPCACHELINES;
	  bool uopcacheIsOn = true;
	  bool RandomPredict();
	  bool periodicPredict();
	  uopline uopCache[UOPWAYS+10][UOPCACHELINES+10];
	  bool uopenabled;
	  bool VPremovevpentry;

	  //stats:
	  UInt64 uopcache_hits;
	  UInt64 uopcache_miss;
	  UInt64 uopcache_access;
	  UInt64 uopcache_stores;
	  UInt64 uopcache_evictions;
	  UInt64 uopcache_VP_access;
	  UInt64 uopcache_VP_stores;
	  UInt64 uopcache_VP_hits;
	  UInt64 uopcache_VP_miss;
	  UInt64 uopcache_VP_haveprediction;
	  UInt64 uopcache_VP_stores_success;
	  UInt64 uopcache_VP_stores_fails;

   public:
	  UopCache();
	  bool isUopCacheValid();
      bool existInUopCache(unsigned long pc);
	  bool PredictUop(unsigned long pc,unsigned long BBhead);
	  bool storeUopCache(unsigned long pc);
	  bool checkVPinfo(unsigned long pc, unsigned long bbhead, unsigned long *value);
	  std::tuple<bool, bool> getVPprediction(unsigned long pc, unsigned long bbhead, unsigned long value);
	  bool AddVPinfo(unsigned long pc, unsigned long bbhead, unsigned long value);
	  unsigned long getWay(unsigned long pc);
	  unsigned long getSet(unsigned long pc);
	  void updateLRU(unsigned long set, int victimbank );
	  bool getVPremovevpentry() { return VPremovevpentry; }
};
#endif // __UOPCACHE_H
