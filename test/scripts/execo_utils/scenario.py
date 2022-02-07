import yaml

from execo import *
from execo_g5k import *
import logging
import sys
import os
import time
from os import walk
import os

# ***********************************************
# This class is responsible for executing a scenario and storing the result in the mongodb database
# ***********************************************
class Scenario :

    # ***********************************************
    # @params:
    #    - scenar: the scenario path
    #    - execoClient: the client (already configured) that will start the VMs
    # ***********************************************
    def __init__ (self, scenar, execoClient, mongoClient) :
        self._client = execoClient
        self._mongo = mongoClient
        self._scenar = scenar
        self._vms = {}
        self._toInstall = []
        self._toRun = {}
        self._running = {}
        self._output = {}
        self._vmNames = {}
        self._cpuconfig = {}
        self._memconfig = {}
        
        self._readScenario ()
        
    # ***********************************************
    # Start the VMs involved in the scenario
    # Wait until all the VMs are correctly configured
    # ***********************************************
    def configure (self):
        self._client.uploadMonitorConfiguration (self._cpuconfig, self._memconfig)
        self._client.startMonitor ()
        time.sleep (2)
        
        cmds = {}
        for v in self._toInstall :
            cmds[v[0]["name"]] = self._client.startVM (v[0])

        for v in self._toInstall :
            cmds [v[0]["name"]].wait ()
            vm = self._client.connectVM (v[0])
            self._vms [v[0]["name"]] = (vm, v[1])
            self._vmNames [vm] = v[0]["name"]
            
        self._installTests ()
        self._configureRun ()
        
    # ***********************************************
    # Install the test on the VMs before running them
    # This is a part of the configuration
    # ***********************************************
    def _installTests (self) :
        cmds = []
        part2 = []

        # Install the tests inside the VMs
        for v in self._vms :  
            vm = self._vms[v][0]
            test = self._vms[v][1]            
            if (test["type"] == "phoronix") :
                cmds = cmds + [self._installPhoronix (vm)] # Just install phoronix, second step will install the proper test
                part2 = part2 + [self._vms[v]] # Phoronix needs a second step
            elif (test ["type"] == "custom") :
                cmds = cmds + [self._installCustomTest (vm, test)]
            else :
                logger.error ("Unknown test type : " + str (test["type"]) + " for VM " + str (v))

        self._client.waitAndForceCmds (cmds)

        # Some tests needs a second step to be properly installed (e.g. phoronix)
        cmds = []
        for p in part2 :
            cmds = cmds + [self._installPhoronixTest (p[0], p[1])]
            
        self._client.waitAndForceCmds (cmds)


    # ***********************************************
    # Configure the set that will be used to determine when to start and stop tests
    # ***********************************************
    def _configureRun (self) : 
        for v in self._vms :
            vm = self._vms[v][0]
            test = self._vms[v][1]            
            if (test["start"] in self._toRun) : 
                self._toRun [test["start"]] = self._toRun[test["start"]] + [(test, vm)]
            else :
                self._toRun[test["start"]] = [(test, vm)]

            if ("end" in test) : 
                if (test["end"] in self._toRun) : 
                    self._toRun [test["end"]] = self._toRun[test["end"]] + [(test, vm)]
                else :
                    self._toRun[test["end"]] = [(test, vm)]

        

    # ***********************************************
    # Install phoronix on the VM "vm"
    # @warning: does not wait the end of the install command
    # @params:
    #    - vm: the VM Host
    # @returns: the install command to wait
    # ***********************************************
    def _installPhoronix (self, vm) :
        # Phoronix is installed by a deb file
        self._client.uploadFiles ([vm], ["../utils/phoronix/phoronix.deb"], ".", user = "phil")

        # There is a directory containing config information that must be updated
        self._client.launchAndWaitCmd ([vm], "mkdir -p /home/phil/.phoronix-test-suite", user="phil");

        # put the config file in that directory
        self._client.uploadFiles ([vm], ["../utils/phoronix/user-config.xml"], ".phoronix-test-suite/", user = "phil")

        # Install the dependency of phoronix, and then phoronix, and configure it
        cmd = self._client.launchCmd ([vm], 'sudo apt update ; sudo apt install -y php-cli php-xml unzip ; sudo apt --fix-broken install -y ; sudo dpkg -i phoronix.deb ; echo -e "y\\nn" | phoronix-test-suite', user="phil")
        
        return cmd

    # ***********************************************
    # Install the phoronix test "test" on the VM "vm"
    # @assume: phoronix is installed and configured on the VM
    # @warning: does not wait the end of the installation command
    # @params:
    #    - vm: the VM host
    #    - test: the dictionnary containing the name of the test to execute (e.g. {"name" : "compress-7zip"})
    # @returns: the install command to wait
    # ***********************************************
    def _installPhoronixTest (self, vm, test) :
        # install the test
        return self._client.launchCmd ([vm], "phoronix-test-suite install " + str (test["name"]))        

    # ***********************************************
    # Install a custom test in the VM
    # @warning: does not wait the end of the install command
    # @params:
    #    - vm: the VM Host
    #    - test: the dictionnary containing the name of the custom test to install
    # @returns: the install command to wait
    # ***********************************************
    def _installCustomTest (self, vm, test) :
        files = {}
        rootPath = "../utils/" + str (test["name"]);
        for (d, d2, filenames) in walk (rootPath) : # Create a dictionnary containing the tree definition of the directory containing the test
            p = os.path.relpath (d, rootPath)
            fis = []
            for f in filenames :
                fis = fis + [str (d) + "/" + f]
            files[p] = fis

        rootPath = test["name"]
        for p in files : # Upload all the files of the test
            self._client.launchAndWaitCmd ([vm], "mkdir -p " + rootPath + "/" + p, user = "phil")
            self._client.uploadFiles ([vm], files[p], rootPath + "/" + p, user = "phil")

        # Some test have dependencies, so the install script is there to install them
        return self._client.launchCmd ([vm], rootPath + "/install.sh", user = "phil")    

    # ***********************************************
    # Start the scenario (launch the benchmark at the correct moment)
    # Wait until the scenario is finished
    # Retreive the log files of the monitors
    # ***********************************************
    def run (self): 
        instants = list (self._toRun.keys ())
        sorted (instants)

        time.sleep (30)
        logger.info ("Starting benchmark")
        self._client.resetMonitorCounters ()
        
        for i in range (len (instants)) :
            logger.info ("Running test at instant : " + str (instants[i]))
            self._runTests (instants [i], self._toRun [instants [i]])
            if (i != len (instants) - 1) :
                toSleep = instants [i + 1] - instants [i]
                logger.info ("Sleeping : " + str (toSleep))
                time.sleep (toSleep)

        # Wait the tests that are not killed 
        for t in self._running:
            self._running [t].wait ()
            self._output [self._vmNames[t]] = self._running[t].processes[0].stdout

        time.sleep (30)
        logger.info ("Benchmark finished")        
    
    # ***********************************************
    # Run or end the test in "tests"
    # @info: populate the set "self._running"
    # @params:
    #     - instant: the current instant of the test running
    #     - tests: the tests to run or kill    
    # ***********************************************    
    def _runTests (self, instant, tests) :
        for t in tests :
            if (instant == t[0]["start"]) :
                self._startTest (t[1], t[0])
            else :
                if ("end" in t[0]) : 
                    if (instant == t[0]["end"]) :
                        self._killTest (t[1])

    # ***********************************************
    # Start the test "test" on the vm "vm"
    # @info: populate the set "self._running"
    # @params:
    #    - vm: the vm Host
    #    - test: the test to launch
    # ***********************************************    
    def _startTest (self, vm, test):
        if (test["type"] == "phoronix") :
            nbRun = test["nb-run"]
            self._running [vm] = self._client.launchCmd ([vm], "export FORCE_TIMES_TO_RUN=" + str (nbRun) + ' ; phoronix-test-suite batch-run ' + test["name"], user="phil")
        else :
            rootPath = test["name"]
            self._running[vm] = self._client.launchCmd ([vm], "cd " + rootPath + " ; ./launch.sh " + test["params"], user = "phil")

    # ***********************************************
    # Kill the test that is currently running in the VM "vm"
    # @info: update the set "self._running"
    # @params:
    #    - vm: the vm Host
    # ***********************************************    
    def _killTest (self, vm) :
        if (vm in self._running) :
            logger.info ("Ending the test on " + str (vm))
            self._client.killCmds ([self._running [vm]])
            self._output[self._vmNames[vm]] = self._running[vm].processes[0].stdout
        
    # ***********************************************
    # Store the result of the scenario in the mongodb database
    # ***********************************************
    def store (self):
        logs = self._client.downloadMonitorLogs ()
        self._client.killAllVMs ()
        self._client.killMonitor ()

        resultPage = {}
        resultPage ["monitors"] = self._readMonitorLogs (logs)
        resultPage ["vms"] = self._output
        placement = self._client._vmInfos;
        resultPage ["placement"] = {name : placement[name].address for name in placement}
        
        with open (self._scenar, 'r') as fp :
            content = yaml.load (fp, Loader=yaml.FullLoader)
            s = yaml.dump (content)
            resultPage ["scenario"] = s
            self._mongo.insertResult (s, resultPage)
                    
    # ***********************************************
    # Read the monitor logs
    # @params:
    #    - logs: the list of log files ({node_address : path_to_file})
    # @returns: {node_address : file_content}
    # ***********************************************
    def _readMonitorLogs (self, logs):
        result = {}
        for (i, j) in logs :
            with open (j, "r") as fp : 
                result [i] = fp.read ()
        return result

    # ***********************************************
    # Read the scenario file
    # ***********************************************
    def _readScenario (self) :
        with open (self._scenar, 'r') as fp :
            content = yaml.load (fp, Loader=yaml.FullLoader)
            for vi in content["vms"] :
                for j in range (int (vi["instances"])) :
                    vm = {"name" : vi["name"] + str (j),
                          "image" : vi["image"],
                          "vcpus" : int (vi["vcpus"]),
                          "memory" : int (vi["memory"]),
                          "frequency" : int (vi["frequency"]),
                          "memorySLA" : float (vi["memorySLA"]),
                          "disk" : int (vi["disk"])}
                    bench = vi["test"]
                    self._toInstall = self._toInstall + [(vm, bench)]

            if ("cpu-market" in content):
                self._cpuconfig = content["cpu-market"]
            if ("mem-market" in content): 
                self._memconfig = content["mem-market"]
