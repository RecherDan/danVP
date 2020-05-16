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

class TestExec:
  def __init__(self, name, dir, cmd):
    global test_dir
    global defaultinstructionscount
    self.name = name;
    self.dir = dir;
    self.cmd = dir + cmd;
    self.trace ="";
    self.output = "/dev/null";
    self.stats = test_dir + "/" + name + ".stats";
    self.errors = test_dir + "/" + name + ".errors";
    self.simout = test_dir + "/" + name + ".sim.out";		
    self.instructionscount = defaultinstructionscount;
  def record_trace(self):
    global SPECDIR
    curdir=os.path.dirname(os.path.realpath(__file__));
    cmd=". " + SPECDIR + "shrc";
    print(cmd);
    os.system(cmd);
    print("Recording trace of " + self.name);
    os.chdir(self.dir);
    cmd= curdir + '/record-trace -o ' + self.name + ' -d ' + self.instructionscount + ' -- ' + self.cmd + ' > ' + self.output + ' 2> ' + curdir + '/' + self.errors;
    print("Record: " + cmd);
    os.system(cmd);
    self.trace = self.dir + '/' + self.name;
    os.chdir(curdir);

class Data:
  def __init__(self, csv):
    global test_dir
    global keys
    global defaultparams
    global defaultinstructionscount
    self.csv = test_dir + "/" + csv;
    self.VPstats = {}
    for stat in keys:
	    self.VPstats[stat] = datastats("",2);
    self.VP_access = 0;
    self.VP_haveprediction = 0;
    self.VP_hits = 0;
    self.cycles = 0;
    self.IPC = 0;
    self.instructions = 0;
    self.testscount =0;
    os.system('rm -f ' + self.csv);
    line = ""
    for a in keys:
	    line = line + ", " + str(a)
    os.system('echo name, SPEC, TEST, penalty, cycles, IPC, instructions ' + line + ' > ' + self.csv);
  def addline(self, name, specname, testname, penalty, cycles, IPC, instructions, VPStats):
    global keys
    line = ""
    for a in keys:
	    line = line + ", " + str(VPStats[a].value)
    os.system('echo ' + name + ', ' + str(penalty) + ', ' + str(cycles) + ', ' + str(round(IPC,2)) + ', ' + str(instructions) +  line + ' >> ' + self.csv);    
    #os.system('printf "%s, %d, %.2f, %d %s" ' + name + ' ' + cycles + ' ' + IPC + ' ' + instructions + ' ' + line + ' >> ' + self.csv);
class Test:
  def __init__(self, specname, testname, name, config, trace, penalty, test, input ,configparams):
    global test_dir
    global defaultinstructionscount
    self.specname = specname
    self.testname = testname;
    self.name = name
    self.trace = trace.trace;
    self.test = test
    self.config = config
    self.penalty = penalty
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
    self.instructions = 0
    self.elapsedTime = 0
    self.VPstats = {}
    self.instructionscount = defaultinstructionscount
    for stat in keys:
	    self.VPstats[stat] = datastats(stat, 2)
    if configparams:
	    self.configparams = "-c " + configparams
    else:
	self.configparams = defaultparams
	self.configparams = "-c VP/misspenalty=" + str(self.penalty) + " " + self.configparams 

  def extractStats(self, line, string, location):
	ret = -1
	check = line.split('\n')[0].split(' ')[0]
	stat = int(float(line.split('\n')[0].split(' ')[location]))
	if string == check:
           #print ("comparing %s with %s: %s , stat: %d" % (check, string, check == string, stat))
           ret = stat
	return ret
  
  def recordTrace(self):
    global SPECDIR
    cmd=". " + SPECDIR + "shrc";
    print(cmd);
    os.system(cmd);
    print("Recording trace of " + self.name)
    cmd= './record-trace -o ' + self.name + ' -d ' + self.instructionscount + ' -- ' + self.exe + ' ' + self.test + ' ' + self.input + ' > ' + self.output + ' 2> ' + self.errors;
    print("Record: " + cmd);
    os.system(cmd);

  def updateConfig(self, config):
    self.configparams = config;

  def runtest(self):
    print("running " + self.trace)
    #os.system('./run-sniper -c vp ' + self.configparams + ' -- ' + self.exe + ' ' + self.test + ' ' + self.input + ' > ' + self.output + ' 2>' + self.errors)
    cmd='./run-sniper -c ' + self.config + ' '  + self.configparams + ' --traces=' + self.trace + '  > ' + self.output + ' 2>' + self.errors;
    print("Exexute: " + cmd);
    os.system(cmd);
    os.system('./tools/dumpstats.py > ' + self.stats )
    os.system('cp -fr ./sim.out ' + self.simout ) 
    with open(self.simout) as origin_file:
      for line in origin_file:
	 if 'Cycles' in line:
            self.cycles = float(line.split('\n')[0].split('|')[1].split(' ')[-1])
         if 'IPC' in line:
	    self.IPC = float(line.split('\n')[0].split('|')[1].split(' ')[-1])
         if 'Instructions' in line:
	    self.instructions = float(line.split('\n')[0].split('|')[1].split(' ')[-1])
    with open(self.stats) as origin_file:
      for line in origin_file:
	 for stat in self.VPstats:
		self.VPstats[stat].setIf(self.extractStats(line, self.VPstats[stat].keyword,  self.VPstats[stat].location))
		#print("DEBUG: " + line + " keyword " + self.VPstats[stat].keyword);

def NewConfig(name, config):
	global defaultparams
	global configname
	print("new config name: " + name);
	print("updating config: " + config);
	configname = name
	defaultparams = config
	
def RunTests(testlist):
	global SPECDIR
	print(". " + SPECDIR + "shrc")
	os.system(". " + SPECDIR + "shrc")
	for test in testlist:
		print("exe test: " + test.name + " from: " + test.trace + " outputs: " + test.output)
		start_time = time.time()
		#test.recordTrace()
		test.runtest()
		elapsed_time = time.time() - start_time
		test.elapsedTime = elapsed_time
	CollectData(testlist)
	PrintResult()


def CollectData(testlist):
	global total
	for test in testlist:
		print("Test: %s elapsed Time: %d seconds " % (test.name, test.elapsedTime))
		total.addline(test.name, test.specname, test.testname, test.penalty, test.cycles, test.IPC, test.instructions , test.VPstats)
		print("		%-40s: %10d" % ("instrctions" , test.instructions))
		print("		%-40s: %10d" % ("cycles" , test.cycles))
		print("		%-40s: %10.2f" % ("IPC", test.IPC))
		total.cycles += test.cycles * test.instructions;
		total.instructions += test.instructions;
		total.testscount += 1;
		total.IPC += test.IPC * test.instructions;
 		for stat, v in sorted(test.VPstats.items()):
			total.VPstats[stat].setIf(total.VPstats[stat].value  + (test.VPstats[stat].value * test.instructions));
			print("		%-40s: %10d" % (stat, test.VPstats[stat].value));

def PrintResult():
	global total
	global configname
	global defaultparams
	for stat, v in sorted(total.VPstats.items()):
		total.VPstats[stat].setIf(total.VPstats[stat].value/total.instructions);
	total.addline("Total " + configname, "", "Total", "Total", total.cycles/total.instructions, total.IPC/total.instructions, total.instructions/total.testscount, total.VPstats)
	print("Total Results " + configname)
	print("		%-40s: %10d" % ("instrctions" , total.instructions/total.testscount))
	print("		%-40s: %10d" % ("cycles" , total.cycles/total.instructions))
	print("		%-40s: %10.2f" % ("IPC", total.IPC/total.instructions))
	for stat, v in sorted(total.VPstats.items()):
		print("		%-40s: %10d" % (stat, total.VPstats[stat].value/total.instructions));
		total.VPstats[stat].setIf(0);
	total.cycles = 0
	total.IPC = 0 
	total.testscount = 0
	total.instructions = 0

def GenNewTest(testTraces, name, penalty, config):
	global total
	global testlist
	global configname
	global defaultparams
	testlist = []
	NewConfig(name, config)
	for trace in testTraces:
		testlist.append(Test(trace.name,name,trace.name + "_" +  name,"vp_" + name, trace, penalty, "", "", ""))
	RunTests(testlist)

SPECDIR = "/home/danr/SPEC-CPU2017v1.0.1/"
defaultparams=""
configname=""
defaultinstructionscount="100000000"
defaultinstructionscount="10000000"

#keys = [ 'VP.VP_miss', 'VP.VP_access', 'VP.VP_haveprediction', 'VP.VP_hits', 'VP.VP_Invalidate', 'uopcache.uopcache_access', \
 #   'uopcache.uopcache_evictions', 'uopcache.uopcache_hits', 'uopcache.uopcache_miss', 'uopcache.uopcache_stores', \
  #  'uopcache.uopcache_VP_access', 'uopcache.uopcache_VP_haveprediction', 'uopcache.uopcache_VP_hits', 'uopcache.uopcache_VP_miss', \
   # 'uopcache.uopcache_VP_stores', 'uopcache.uopcache_VP_stores_fails', 'uopcache.uopcache_VP_stores_success', 'dtlb.access', 'dtlb.miss', 'itlb.access', 'itlb.miss', 'stlb.access', 'stlb.miss',  ]

keys = [ 'VP.real_vp_miss', 'VP.VP_miss', 'VP.VP_miss_penalty', 'VP.VP_access', 'VP.VP_haveprediction', 'VP.VP_hits', 'VP.VP_Invalidate',  \
		'uopcache.uopcache_VP_access', 'uopcache.uopcache_VP_haveprediction', 'uopcache.uopcache_VP_hits', 'uopcache.uopcache_VP_miss', 'uopcache.uopcache_VP_stores', 'uopcache.uopcache_VP_stores_fails', \
    'uopcache.uopcache_VP_stores_success', 'uopcache.uopcache_access', 'uopcache.uopcache_evictions', 'uopcache.uopcache_hits', 'uopcache.uopcache_miss', 'uopcache.uopcache_stores', \
    'dtlb.access', 'dtlb.miss', 'itlb.access', 'itlb.miss', 'stlb.access', 'stlb.miss',  ]

test_dir="tests_res"
os.system('mkdir -p ' + test_dir)
total= Data("res.csv")
tests = {}

tests['perlbench']="/home/danr/SPEC_RUNS/600.perlbench_s/perlbench_r -I/home/danr/SPEC_RUNS/600.perlbench_s/lib checkspam.pl 2500 5 25 11 150 1 1 1 1"
tests['gcc']="/home/danr/SPEC_RUNS/602.gcc_s/sgcc /home/danr/SPEC_RUNS/602.gcc_s/gcc-pp.c -O5 -fipa-pta -o /home/danr/SPEC_RUNS/602.gcc_s/gcc-pp.opts-O5_-fipa-pta.s"
tests['mcf']="/home/danr/SPEC_RUNS/605.mcf_s/mcf_s /home/danr/SPEC_RUNS/605.mcf_s/inp.in"
tests['omnetpp']="/home/danr/SPEC_RUNS/620.omnetpp_s/omnetpp_s -c General -r 0"
tests['xalancbmk']="/home/danr/SPEC_RUNS/623.xalancbmk_s/xalancbmk_s -v /home/danr/SPEC_RUNS/623.xalancbmk_s/t5.xml /home/danr/SPEC_RUNS/623.xalancbmk_s/xalanc.xsl"
tests['x264']="/home/danr/SPEC_RUNS/625.x264_s/x264_s --pass 1 --stats /home/danr/SPEC_RUNS/625.x264_s/x264_stats.log --bitrate 1000 --frames 1000 -o /home/danr/SPEC_RUNS/625.x264_s/BuckBunny_New.264 /home/danr/SPEC_RUNS/625.x264_s/BuckBunny.yuv 1280x720"
tests['deepsjeng']="/home/danr/SPEC_RUNS/631.deepsjeng_s/deepsjeng_s /home/danr/SPEC_RUNS/631.deepsjeng_s/ref.txt"
tests['leela']="/home/danr/SPEC_RUNSQRJX9DK/641.leela_s/leela_s /home/danr/SPEC_RUNS/641.leela_s/ref.sgf"
tests['exchange2']="/home/danr/SPEC_RUNS/648.exchange2_s/exchange2_s 6"
tests['xz']="/home/danr/SPEC_RUNS/657.xz_s/xz_s /home/danr/SPEC_RUNS/657.xz_s/cpu2006docs.tar.xz 6643 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1036078272 1111795472 4"

testTraces= []
testTraces.append(TestExec('perlbench',"/home/danr/SPEC_RUNS/600.perlbench_s/","perlbench_s -I/home/danr/SPEC_RUNS/600.perlbench_s/lib checkspam.pl 2500 5 25 11 150 1 1 1 1"));
testTraces.append(TestExec('gcc',"/home/danr/SPEC_RUNS/602.gcc_s/", "sgcc /home/danr/SPEC_RUNS/602.gcc_s/gcc-pp.c -O5 -fipa-pta -o /home/danr/SPEC_RUNS/602.gcc_s/gcc-pp.opts-O5_-fipa-pta.s"));
testTraces.append(TestExec('mcf',"/home/danr/SPEC_RUNS/605.mcf_s/", "mcf_s /home/danr/SPEC_RUNS/605.mcf_s/inp.in"));
testTraces.append(TestExec('omnetpp',"/home/danr/SPEC_RUNS/620.omnetpp_s/", "omnetpp_s -c General -r 0"));
testTraces.append(TestExec('xalancbmk',"/home/danr/SPEC_RUNS/623.xalancbmk_s/", "xalancbmk_s -v /home/danr/SPEC_RUNS/623.xalancbmk_s/t5.xml /home/danr/SPEC_RUNS/623.xalancbmk_s/xalanc.xsl"));
testTraces.append(TestExec('x264',"/home/danr/SPEC_RUNS/625.x264_s/", "x264_s --pass 1 --stats /home/danr/SPEC_RUNS/625.x264_s/x264_stats.log --bitrate 1000 --frames 1000 -o /home/danr/SPEC_RUNS/625.x264_s/BuckBunny_New.264 /home/danr/SPEC_RUNS/625.x264_s/BuckBunny.yuv 1280x720"));
testTraces.append(TestExec('deepsjeng',"/home/danr/SPEC_RUNS/631.deepsjeng_s/", "deepsjeng_s /home/danr/SPEC_RUNS/631.deepsjeng_s/ref.txt"));
testTraces.append(TestExec('leela',"/home/danr/SPEC_RUNS/641.leela_s/", "leela_s /home/danr/SPEC_RUNS/641.leela_s/ref.sgf"));
testTraces.append(TestExec('exchange2',"/home/danr/SPEC_RUNS/648.exchange2_s/", "exchange2_s 6"));
testTraces.append(TestExec('xz',"/home/danr/SPEC_RUNS/657.xz_s/", "xz_s /home/danr/SPEC_RUNS/657.xz_s/cpu2006docs.tar.xz 6643 055ce243071129412e9dd0b3b69a21654033a9b723d874b2015c774fac1553d9713be561ca86f74e4f16f22e664fc17a79f30caa5ad2c04fbc447549c2810fae 1036078272 1111795472 4"));
for trace in testTraces:
  trace.record_trace();

#testTraces.append(TestExec('exchange2', '/home/danr/SPEC_RUNS/648.exchange2_s', '/home/danr/SPEC_RUNS/648.exchange2_s/exchange2_s 6'));
##

#.record_trace();
####

#tests['perlbench']="runcpu --config=dan 600.perlbench_s"
#tests['gcc']="runcpu --config=dan 602.gcc_s"
#tests['mcf']="runcpu --config=dan 605.mcf_s"
#tests['omnetpp_s']="runcpu --config=dan 620.omnetpp_s"
#tests['xalancbmk_s']="runcpu --config=dan 623.xalancbmk_s"
#tests['x264_s']="runcpu --config=dan 625.x264_s"
#tests['deepsjeng_s']="runcpu --config=dan 631.deepsjeng_s"
#tests['leela_s']= "runcpu --config=dan 641.leela_s"
#tests['exchange2_s']= "runcpu --config=dan 648.exchange2_s"
#tests['xz_s']="runcpu --config=dan 657.xz_s"

testlist = []

#for i in range(0,100):
#	if ( (i % 10) == 0 ):
#		GenNewTest(testTraces,"clean", i, "-c VP/type=DISABLE -c uopcache/status=DISABLE")
#		GenNewTest(testTraces,"VP_SIMPLE", i, "-c VP/type=VP_SIMPLE -c uopcache/status=DISABLE")
#		GenNewTest(testTraces,"VP_VTAGE", i, "-c VP/type=VP_VTAGE -c uopcache/status=DISABLE")
#		GenNewTest(testTraces, "VP_SIMPLE_UOPCACHE", i, "-c VP/type=VP_SIMPLE -c uopcache/status=ENABLE")
#		GenNewTest(testTraces, "VP_VTAGE_UOPCACHE", i, "-c VP/type=VP_VTAGE -c uopcache/status=ENABLE")

## branch penalty senstivity test 29.4.2020
#for i in range(0,1000):
#	if ( (((i % 5) == 0 ) and i < 121 ) or (((i % 100) == 0 ) and i > 101 and false ) ):
#		GenNewTest(testTraces,"clean", i, "")
#		GenNewTest(testTraces,"VP_SIMPLE", i, "")
#		GenNewTest(testTraces,"VP_VTAGE", i, "")
#		GenNewTest(testTraces, "VP_SIMPLE_UOPCACHE", i, "")
#		GenNewTest(testTraces, "VP_VTAGE_UOPCACHE", i, "")
 #       
##############################################


GenNewTest(testTraces,"clean", 10, "")
GenNewTest(testTraces,"VP_VTAGE_8k", 10, "")
GenNewTest(testTraces,"VP_VTAGE_32k", 10, "")
GenNewTest(testTraces,"VP_VTAGE_unlimited", 10, "")
GenNewTest(testTraces,"VP_SIMPLE_8k", 10, "")
GenNewTest(testTraces,"VP_SIMPLE_32k", 10, "")
GenNewTest(testTraces,"VP_SIMPLE_unlimited", 10, "")