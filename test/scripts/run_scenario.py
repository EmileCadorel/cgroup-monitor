#!/usr/bin/env python3

from mongo_utils import database
from execo_utils import launcher
from execo_utils import scenario
from execo import *
from execo_g5k import *
import logging
import yaml
import time
import argparse

def parseArguments ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ("scenario", help="the scenario file")
    
    return parser.parse_args ()

# Run a scenario  and store the result in the database
def main (args) : 
    client = launcher.ExecoClient.fromIps (ips = ["192.168.167.62"])
    client.configureNodes (withInstall = False, withDownload = False)

    scenar = scenario.Scenario (args.scenario, client, database.DatabaseClient ())
    scenar.configure ()
    scenar.run ()
    scenar.store ()
    
    client.joinMonitor ()
    

if __name__ == "__main__" :
    main (parseArguments ())
