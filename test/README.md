# Tests

The test are runnable using the scripts files.

## Launching a scenario

First edit the `scripts/run_scenario.py` file and enter the ip of the machines on which the scenario will be executed (does not work with `localhost`, must be a complete ipv4 ip). And then launch the script using a scenario:

```bash
$ python3 run_scenario.py ../scenario/phoronix/small/one.yml
```

This scripts install the dependencies on the node executing the scenario. 
It stores the results inside the mongo database : `dio-monitor/tests`

## Analyse the results

The script `scripts/analyse_scenario.py` retreive the execution inside the mongo database and generates a report in latex : 

```bash
$ python3 analyse_scenario.py ../scenario/phoronix/small/one.yml > /tmp/out.tex
$ cd /tmp/
$ pdflatex --shell-escape out.tex
```

It is recommanded to use shell-escape, there are many figures, and pdflatex runs out of memory quite fast.

## Scenario description

A scenario is composed of two main parts : 

### vms: 

The list of VM instances to launch, and the benchmark to run inside them
- name: the name of the VM instances 
- vcpus: the number of vCPUs per instance
- memory: the size of the memory in MB
- frequency: the frequency of the vCPUs in MHz
- instances: the number of instances to launch 
- disk : the size of the disk in MB
- memorySLA : useless
- image : the image of the VM (OS)
- test: the test to launch

The possible tests are the following : 
- phoronix: 
  + name: compress-7zip/openssl
  + start: second to wait before launching the test
  + nb-run: number of time the benchmark will be executed
  
- deathstar: 
  + name: hotel
  + start: second to wait before launching the test
  + end: second to wait before killing the test
  + nb-threads: the number of threads used to create the workload
  + nb-connections: the number of simultatneous connection 
  + nb-per-seconds: the number of request per seconds

- stress: 
  + start: second to wait before launching the test
  + end: second to wait before killing the test
  + nb-cpus: the number of threads to stress

### cpu-market: 

The configuration of the controller.

## Reports

The directory `reports` contains reports of some scenarios.
