#!/usr/bin/env python3

from mongo_utils import database
from execo_utils import launcher
from execo_utils import scenario
from execo import *
from execo_g5k import *
import logging
import yaml
import time

def main () : 
    client = launcher.ExecoClient.fromIps (ips = ["192.168.167.62"])
    client.configureNodes (withInstall = False, withDownload = False)
    client.startMonitor ()

    scenar = scenario.Scenario ("./scenar1.yml", client, database.DatabaseClient ())
    scenar.configure ()
    scenar.run ()
    scenar.store ()
    
    client.joinMonitor ()
    

if __name__ == "__main__" :
    main ()
