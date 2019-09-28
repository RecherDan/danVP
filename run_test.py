#!/usr/bin/python
import os
import subprocess
import re
import time

class datastats:
  def __init__(self,keyword,location):
	self.value = 0
	self.keyword = keyword
	self.location = location
  def setIf(self,value):
	if value != -1:
	  self.value = value
	  #print("set value of: " + self.keyword + " value: " + str(self.value))

class Data:
  def __init__(self, csv):
    global test_dir
    global keys
    self.csv = test_dir + "/" + csv;
    self.VPstats = {}
    for stat in keys:
	    self.VPstats[stat] = 0;
    self.VP_access = 0;
    self.VP_haveprediction = 0;
    self.VP_hits = 0;
    self.cycles = 0;
    self.IPC = 0;
    os.system('rm -f ' + self.csv);
    line = ""
    for a in keys:
	    line = line + ", " + str(a)
    os.system('echo name, cycles, IPC ' + line + ' > ' + self.csv);
  def addline(self, name, cycles, IPC, VPStats):
    global keys
    line = ""
    for a in keys:
	    line = line + ", " + str(VPStats[a].value)
    os.system('echo ' + name + ', ' + str(cycles) + ', ' + str(IPC) +  line + ' >> ' + self.csv);    
class Test:
  def __init__(self, name,exe, test, input ,configparams):
    global test_dir
    self.name = name
    self.exe = exe
    self.test = test
    self.output = test_dir + "/" + name + ".out"
    self.output = "/dev/null"
    self.input = input
    if input != "":
	    input = " < " + input
    self.stats = test_dir + "/" + name + ".stats"
    self.errors = test_dir + "/" + name + ".errors"
    self.simout = test_dir + "/" + name + ".sim.out"
    self.cycles = 0
    self.IPC = 0
    self.elapsedTime = 0
    self.VPstats = {}
    for stat in keys:
	    self.VPstats[stat] = datastats(stat, 2)
    if configparams:
    	self.configparams = "-c " + configparams
    else:
	self.configparams = ""

  def extractStats(self, line, string, location):
	ret = -1
	if string in line:
           ret = int(line.split('\n')[0].split(' ')[location])
	return ret

  def runtest(self):
    print("running " + self.exe)
    os.system('./run-sniper -c vp ' + self.configparams + ' -- ' + self.exe + ' ' + self.test + ' ' + self.input + ' > ' + self.output + ' 2>' + self.errors)
    os.system('./tools/dumpstats.py > ' + self.stats )
    os.system('cp -fr ./sim.out ' + self.simout ) 
    with open(self.simout) as origin_file:
      for line in origin_file:
	 if 'Cycles' in line:
            self.cycles = float(line.split('\n')[0].split('|')[1].split(' ')[-1])
         if 'IPC' in line:
	    self.IPC = float(line.split('\n')[0].split('|')[1].split(' ')[-1])
    with open(self.stats) as origin_file:
      for line in origin_file:
	 for stat in self.VPstats:
		self.VPstats[stat].setIf(self.extractStats(line, self.VPstats[stat].keyword,  self.VPstats[stat].location))


def RunTests(testlist):
	for test in testlist:
		print("exe test: " + test.name + " from: " + test.exe + " outputs: " + test.output)
		start_time = time.time()
		test.runtest()
		elapsed_time = time.time() - start_time
		test.elapsedTime = elapsed_time


def CollectData(testlist):
	global total
	for test in testlist:
		print("Test: %s elapsed Time: %d seconds " % (test.name, test.elapsedTime))
		total.addline(test.name, test.cycles, test.IPC, test.VPstats)
		print("		%-40s: %10d" % ("cycles" , test.cycles))
		print("		%-40s: %10.2f" % ("IPC", test.IPC))
 		for stat, v in sorted(test.VPstats.items()):
			total.VPstats[stat] += test.VPstats[stat].value
			print("		%-40s: %10d" % (stat, test.VPstats[stat].value));

def PrintResult(testlist):
	global total
	print("Total Results")
	for stat, v in sorted(total.VPstats.items()):
		print("		%-40s: %10d" % (stat, total.VPstats[stat]));

keys = [ 'VP.VP_miss', 'VP.VP_access', 'VP.VP_haveprediction', 'VP.VP_hits', 'VP.VP_Invalidate', 'uopcache.uopcache_access', \
    'uopcache.uopcache_evictions', 'uopcache.uopcache_hits', 'uopcache.uopcache_miss', 'uopcache.uopcache_stores', \
    'uopcache.uopcache_VP_access', 'uopcache.uopcache_VP_haveprediction', 'uopcache.uopcache_VP_hits', 'uopcache.uopcache_VP_miss', \
    'uopcache.uopcache_VP_stores', 'uopcache.uopcache_VP_stores_fails', 'uopcache.uopcache_VP_stores_success' ]
test_dir="tests_res"
os.system('mkdir -p ' + test_dir)
total= Data("res.csv")

testlist = []
testlist.append(Test("clean", "~/tests/tst2", "","", "VP/type=DISABLE -c uopcache/status=DISABLE"))
testlist.append(Test("VP_SIMPLE-uopcache", "~/tests/tst2", "", "", "VP/type=VP_SIMPLE -c uopcache/status=DISABLE"))
testlist.append(Test("VP_SIMPLE+uopcache", "~/tests/tst2", "", "", "VP/type=VP_SIMPLE"))
testlist.append(Test("VP_VTAGE-uopcache", "~/tests/tst2", "", "", "VP/type=VP_VTAGE -c uopcache/status=DISABLE"))
testlist.append(Test("VP_VTAGE+uopcache", "~/tests/tst2", "", "", "VP/type=VP_VTAGE"))
## SPEC 2017 tests:
testlist.append(Test("perlbench_r_base", "~/SPEC2017/benchspec/CPU/500.perlbench_r/exe/perlbench_r_base.mytest-m64", "~/SPEC2017/benchspec/CPU/500.perlbench_r/data/test/input/makerand.pl","", ""))

testlist.append(Test("bwaves_r_base", "~/SPEC2017/benchspec/CPU/503.bwaves_r/exe/bwaves_r_base.mytest-m64", "~/SPEC2017/benchspec/CPU/503.bwaves_r/data/test/input/control", "~/SPEC2017/benchspec/CPU/503.bwaves_r/data/test/input/bwaves_1.in", ""))
#testlist.append(Test("perlbench_r_base", "~/SPEC2017/benchspec/CPU/500.perlbench_r/exe/perlbench_r_base.mytest-m64", "~/SPEC2017/benchspec/CPU/500.perlbench_r/data/test/input/test.pl","", ""))


RunTests(testlist)
CollectData(testlist)
PrintResult(testlist)


