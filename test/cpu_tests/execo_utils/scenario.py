import yaml
import logging
import sys
import os
import time

# ***********************************************
# This class is responsible for executing a scenario and storing the result in the mongodb database
# ***********************************************
class Scenario :

    # ***********************************************
    # @params:
    #    - scenar: the scenario path
    #    - execoClient: the client (already configured) that will start the VMs
    # ***********************************************
    def __init__ (self, scenar, execoClient) :
        self._client = execoClient
        self._scenar = scenar
        self._vms = {}
        self._toLaunch = []

        self._readScenario ()
        
    # ***********************************************
    # Start the VMs involved in the scenario
    # Wait until all the VMs are correctly configured
    # ***********************************************
    def configure (self):
        cmds = {}
        for v in self._toLaunch :
            cmds[v[0]["name"]] = self._client.startVM (v[0])

        for v in self._toLaunch :
            cmds [v[0]["name"]].wait ()
            vm = self._client.connectVM (v[0])
            self._vms [v[0]["name"]] = (vm, v[1])

        print (self._vms)
        
    # ***********************************************
    # Start the scenario (launch the benchmark at the correct moment)
    # Wait until the scenario is finished
    # Retreive the log files of the monitors
    # ***********************************************
    def run (self): 
        pass
        
    # ***********************************************
    # Store the result of the scenario in the mongodb database
    # ***********************************************
    def store (self): 
        pass

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
                    bench = vi["phoronix"]
                    self._toLaunch = self._toLaunch + [(vm, bench)]
        
