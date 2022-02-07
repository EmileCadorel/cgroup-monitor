#!/usr/bin/env python3

from mongo_utils import database
from result_utils import analyser
import time
import argparse

def parseArguments ():
    parser = argparse.ArgumentParser ()
    parser.add_argument ("scenario", help="the scenario file")
    
    return parser.parse_args ()

# Retreive the result of a scenario from database
# And generate the latex file presenting the results
# latex file is printed in stdout
def main (args) : 
    an = analyser.ResultAnalyser (database.DatabaseClient (), args.scenario)
    an.run ()    
    
    
    
if __name__ == "__main__" :
    main (parseArguments ())
