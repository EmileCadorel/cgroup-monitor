
from execo import *
from execo_g5k import *
import yaml
import logging
import sys
import os
import time

images = {
    "ubuntu-20.04" : "http://cloud-images.ubuntu.com/releases/focal/release-20210921/ubuntu-20.04-server-cloudimg-amd64.img"
}

# ***********************************************
# This class is responsible to node connection, and VM creations
# It possesses nodes, and VMs connections
# ***********************************************
class ExecoClient :

    # *****************************
    # @params:
    #    - nodes: the list of host nodes (execo nodes)
    # @info: this constructor should only be called from self.fromG5K, or self.fromIps
    # *****************************
    def __init__ (self, nodes) :
        self._hnodes = nodes
        self._monitorCmd = None    
        self._ports = {}
        self._vmInfos = {}


    # *****************************
    # Create an execo client from a G5K job
    # Automatically find the running job
    # @warning: Waits for a job to be in running state
    # @params:
    #    - sites: the list of grid5000 sites in which to look for a running job
    # *****************************
    @classmethod
    def fromG5K (cls, sites = ["nantes", "lille"]) : 
        jobs = get_current_oar_jobs (["nantes", "lille"])
        if (len (jobs) == 0) :
            sys.exit ("No jobs on sites : ", sites)

        logger.info ("Current job : " + str (jobs))
        while True :
            running_jobs = [ job for job in jobs if get_oar_job_info (*job).get ("state") == "Running" ]
            if (len (running_jobs) != 0) : 
                nodes = sorted ([job_nodes for job in running_jobs for job_nodes in get_oar_job_nodes (*job)], key=lambda x: x.address)
                logger.info ("Will deploy on : " + str (nodes))

                deployed, undeployed = deploy (Deployment (nodes, env_name="ubuntu2004-x64-min"), check_deployed_command=started)
                return cls (nodes)
            else :
                time.sleep (1)

    
    # *****************************
    # Create an execo client from ips 
    # @params:
    #   - ips: the list of node ips
    #   - user: the user of those machines
    # *****************************
    @classmethod
    def fromIps (cls, ips = ["127.0.0.1"], user = "root", keyfile = os.path.expanduser ('~') + "/.ssh/id_rsa") :
        nodes = []
        for ip in ips :
            nodes = nodes + [Host (ip, user=user, keyfile=keyfile)]

        return cls (nodes)


    # *****************************
    # Configure the nodes to be able to run the VMs, and monitor
    # *****************************
    def configureNodes (self, debFile = "./libdio_1.0.deb", withInstall = True, withDownload = True) :
        if (withInstall) : 
            self.launchAndWaitCmd (self._hnodes, "sudo apt-get update")
            self.launchAndWaitCmd (self._hnodes, "sudo apt-get install -y stress cgroup-tools apt-transport-https ca-certificates curl gnupg lsb-release")
            self.launchAndWaitCmd (self._hnodes, "sudo apt-get update")
            self.launchAndWaitCmd (self._hnodes, "sudo apt-get install -y qemu-kvm libvirt-daemon-system libvirt-clients bridge-utils virtinst virt-manager nfs-common libguestfs-tools default-jre ruby dnsmasq-utils php-cli php-xml")
            self.uploadFiles (self._hnodes, ["./libdio.deb"], "./")
            self.launchAndWaitCmd (self._hnodes, "dpkg -i libdio.deb")

        if (withDownload) :
            self.launchAndWaitCmd (self._hnodes, "mkdir -p .qcow2")
            self.downloadImages (self._hnodes, images, ".qcow2")


    # ================================================================================
    # ================================================================================
    # =========================           MONITOR            =========================
    # ================================================================================
    # ================================================================================

        
    # ***************************
    # Start the monitor on the remote host nodes
    # ***************************
    def startMonitor (self) :
        self._monitorCmd = self.launchCmd (self._hnodes, "sudo dio-monitor")
        logger.info ("Monitor started on " + str (self._hnodes))


    # ***************************
    # Wait for the end of the monitor command
    # @assume: startMonitor was executed
    # ***************************
    def joinMonitor (self):
        self.waitCmds ([self._monitorCmd])
        logger.info ("Monitor joined on " + str (self._hnodes))

    # ***************************
    # Kill the running monitors on each nodes
    # @assume: startMonitor was executed
    # ***************************
    def killMonitor (self):
        self.killCmds ([self._monitorCmd])
        logger.info ("Monitor killed on " + str (self._hnodes))

    # ***************************
    # Download the log files of the monitor
    # @params:
    #    - path: where to put the log files (one sub directory is created for each node)
    # @returns: the list of log files
    # ***************************
    def downloadMonitorLogs (self, path = "/tmp/"):
        logs = []
        for h in self._hnodes :
            p = path + "log-" + h.address + ".json"
            self.downloadFiles ([h], ["/var/log/dio/control-log.json"], p)
            logs = logs + [(h.address, p)]
        return logs

    # ================================================================================
    # ================================================================================
    # =========================         CLIENT UTILS         =========================
    # ================================================================================
    # ================================================================================

    # **********************************
    # Start a VM with a given configuration, does not wait vm start
    # @params:
    #    - vmInfo: the information about the VM in dico format
    #    - keyFile: the pub key (actual content of the key)
    #    - loadBalancer: the load balancer that chooses the node on which run the VM (None means always the first node)
    # @returns: the command used to create the VM
    # @example:
    # =======================
    # client = ExecoClient.fromG5K ()
    # client.configureNodes ()
    # client.startMonitor ()
    # 
    # pubkey = ".ssh/id_rsa.pub"
    # prvKey = ".ssh/id_rsa"
    # cmd = client.startVM ({"name" : "v0", "vcpus" : 4, "memory" : 4096, "disk" : 10000, "frequency" : 800}, pubKey, prvKey, loadBalancer = BestFit ())
    # 
    # =======================
    # **********************************
    def startVM (self, vmInfo, pubKey = "./keys/key.pub", loadBalancer = None):
        node = None
        if (loadBalancer == None):
            node = self._hnodes[0]
        else :
            node = loadBalancer.select (self._hnodes, self._runningVMS)

        pubKeyContent = ""
        with open (pubKey, "r") as fp:
            pubKeyContent = fp.read ()[0:-1]
        
        configFile = self.createVMConfigFile (vmInfo, pubKeyContent)
        with open ("/tmp/{0}_config.toml".format(vmInfo["name"]), "w") as fp :
            fp.write (configFile)

        self.uploadFiles ([node], ["/tmp/{0}_config.toml".format (vmInfo["name"])], "./") 
        cmd = self.launchCmd ([node], "dio-client --provision ./{0}_config.toml".format (vmInfo["name"]))
        self._vmInfos[vmInfo["name"]] = node

        return cmd

        
    # **********************************
    # Open a port to connect to the VM via nat
    # @params:
    #    - vmInfo: the information about the VM
    # @returns: a VM host connected to the VM
    # **********************************
    def connectVM (self, vmInfo, prvKey = "./keys/key") :
        node = self._vmInfos [vmInfo["name"]]
        port = self.createUnusedPort (node)
        cmd = self.launchCmd ([node], "dio-client --nat " + str (vmInfo["name"]) + " --host {0} --guest 22".format (port))
        cmd.wait ()

        return Host (node.address, user="phil", keyfile=prvKey, port=port)


    # **********************************
    # Get an unused port on the node node
    # @params:
    #    - node: the node on which the port will be opened
    # **********************************
    def createUnusedPort (self, node):
        port = 2020
        if (node in self._ports):
            port = self._ports[node]
            self._ports[node] = port + 1
        else : 
            self._ports[node] = port + 1
        return port

    # **********************************
    # Kill all the running VMs
    # **********************************
    def killAllVMs (self) :
        for v in self._vmInfos :
            self.launchAndWaitCmd ([self._vmInfos[v]], "dio-client --kill " + str (v))
    
    
    # **********************************
    # Create a file containing the toml configuration of a VM
    # @params:
    #    - vmInfo: the dico containing the information about the VM
    #    - pubKey: the content of the pub key
    # @returns: the content of the configuration file
    # **********************************
    def createVMConfigFile (self, vmInfo, pubKey) :
        s = "[vm]\n"
        s = s + "name = \"{0}\"\n".format (vmInfo ["name"])
        s = s + "image = \"/root/.qcow2/{0}.qcow2\"\n".format (vmInfo["image"])
        s = s + "ssh_key = \"{0}\"\n".format (pubKey)
        s = s + "vcpus = {0}\n".format (vmInfo["vcpus"])
        s = s + "memory = {0}\n".format (vmInfo["memory"])
        s = s + "disk = {0}\n".format (vmInfo["disk"])
        s = s + "frequency = {0}\n".format (vmInfo ["frequency"])
        s = s + "memorySLA = {0}\n".format (vmInfo["memorySLA"])
        return s
        
        
    # ================================================================================
    # ================================================================================
    # =========================        COMMAND UTILS         =========================
    # ================================================================================
    # ================================================================================    

    # ************************************
    # Run a bash command on remote nodes, and waits its completion
    # @params:
    #   - nodes: the list of nodes
    #   - cmd: the command to run
    #   - user: the user that runs the command
    # ************************************
    def launchAndWaitCmd (self, nodes, cmd, user = "root") :
        conn_params = {'user': user}
        cmd_run = Remote (cmd, nodes, conn_params)
        logger.info ("Launch " + cmd + " on " + str (nodes))
        cmd_run.start ()
        status = cmd_run.wait ()
        logger.info ("Done")
        
    # ************************************
    # Run a bash command on remote nodes, but do not wait its completion
    # @params:
    #   - nodes: the list of nodes
    #   - cmd: the command to run
    #   - user: the user that runs the command
    # ************************************
    def launchCmd (self, nodes, cmd, user = "root") :
        conn_params = {'user': user}
        cmd_run = Remote (cmd, nodes, conn_params)
        logger.info ("Launch " + cmd + " on " + str (nodes))
        cmd_run.start ()
        return cmd_run

    # ************************************
    # Kill commands before their completion
    # @params:
    #    - cmds: the list of command to kill
    # ************************************
    def killCmds (self, cmds):
        for c in cmds:
            c.kill ()
    
    # ************************************
    # Wait the completion of commands (do not restart failing commands)
    # @params:
    #    - cmds: the list of command to wait
    # ************************************
    def waitCmds (self, cmds) :
        for c in cmds :
            c.wait ()    
    
    # ************************************
    # Wait the completion of commands, and restart them if they failed for some reason
    # @params:
    #    - cmds: the list of command to wait
    # ************************************
    def waitAndForceCmds (self, cmds) : 
        while True :
            restart = []
            for c in cmds :
                status = c.wait (timeout = get_seconds (0.1))
                if not status.ok :
                    logger.info ("Command failed, restarting : " +  str (c))
                    c.reset ()
                    c.start ()
                    restart = restart + [c]
                elif not status.finished_ok :
                    restart = restart + [c]
                else :
                    logger.info ("Command finished " + str (c))
            if restart != []:
                cmds = restart
            else :
                break

    # ************************************
    # Download image files on remote nodes
    # @params:
    #   - nodes: the list of nodes
    #   - images: the list of images to download (wget)
    #   - user: the user that will download the images
    # ************************************            
    def downloadImages (self, nodes, images, user = "root") :
        conn_params = {'user': user}
        cmd_run = Remote ("mkdir -p .qcow2", nodes, conn_params)
        cmd_run.start ()
        cmd_run.wait ()
    
        for i in images :
            cmd = "wget " + images [i] + " -O .qcow2/" + i + ".qcow2"
            cmd_run = Remote (cmd, nodes, conn_params)
            logger.info ("Launch " + cmd + " on " + str (nodes))
            cmd_run.start ()
            cmd_run.wait ()

        logger.info ("Done")

    # ****************************
    # Upload files located on localhost to remote nodes
    # @params:
    #    - nodes: the list of nodes
    #    - files: the list of files to upload
    #    - directory: where to put the files
    #    - user: the user on the remote nodes
    # ****************************
    def uploadFiles (self, nodes, files, directory,  user = "root") :
        conn_params = {'user': user}
        cmd_run = Put (nodes, files, directory, connection_params=conn_params)
        logger.info ("Upload files on " + str (nodes) + ":" + str (files))
        cmd_run.run ()
        cmd_run.wait ()
        logger.info ("Done")


    # ****************************
    # Download files located on remote nodes to localhost
    # @params:
    #    - nodes: the list of nodes
    #    - files: the list of files to download
    #    - directory: where to put the files
    #    - user: the user on the remote nodes
    # ****************************
    def downloadFiles (self, nodes, files, directory, user = "root") :
        conn_params = {'user': user}
        cmd_run = Get (nodes, files, directory, connection_params=conn_params)
        logger.info ("Download files on " + str (nodes) + ":" + str (files))
        cmd_run.run ()
        cmd_run.wait ()
        logger.info ("Done")        
