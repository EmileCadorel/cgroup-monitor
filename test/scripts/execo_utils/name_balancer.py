
# ***********************************************
# The by name balancer class is selecting the node according to the name given in the description of the VM
# ***********************************************
class ByNameBalancer :


    def __init__ (self) :
        pass

    # ***********************************************
    # Select the node that will be used to host the new VM
    # @params:
    #     - nodes: the list of running nodes
    #     - placement: the placement of the VM that are already running on the nodes
    #     - vmInfos: the list of VMs already running
    #     - newVM: the vm to place
    # @returns: the node that will host the VM (in 'nodes')
    # ***********************************************
    def select (self, nodes, placement, vmInfos, newVM) :
        for i in nodes :
            print (i)
        return nodes[0]
        
    
    
