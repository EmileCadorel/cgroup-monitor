# Monitor

The monitor is used to manage the virtual frequency, and virtual RAM scaling

## Configuration file

- Monitor
```
[monitor] # configuration of the monitor
report="127.0.0.1:8080" # tcp server sending reports
daemon="127.0.0.1:8081" # tcp server waiting for new VM infos
tick=1.0 # tick the monitor in seconds
format="json" # the format of the report
slope-history=5 # the number of reports used to compute the slope of the frequency of the VM
cpu-max-frequency=3000 # The maximal frequency in MHz of the CPU of the host

[market]
trigger-increment=95.0 # Increment the frequency of the VM if the usage is higher than 95%
trigger-decrement=50.0 # Decrement the frequency of the VM if the usage is lower than 50%
increasing-speed=100.0 # Add 100% of the frequency when incrementing
decreasing-speed=20.0 # Remove 20% of the frequency when decrementing the frequency
window-size=100000 # Number of cycle to send at each bidding
```

- Client (might change in the future)

```
[monitor]
daemon="127.0.0.1:8081" # the tcp server to which the VM infos are sent

[vms]
v0 = 200 # the nominal frequency in MHz of the VM named v0
bob = 2500 # the nominal frequency MHz of the VM named bob
```

## Usage

1) Compiling : 
```
$ cd .build
$ cmake ..
$ make
```

2) Starting the monitor
```
$ ./monitor ../resources/default.toml
```

3) Sending VM infos to the monitor (assuming that the VMs are running):

```
./client ../resources/client.toml
```

4) Acquiring results : 

```
nc localhost 8080 | tee out.json
```

# Virtual frequency


# Virtual RAM

## Create swap image

### On the host 

qemu-img create -f raw example-vm-swap.img 10G
virsh attach-disk vv0 /tmp/example-vm-swap.img --target vdb --persistent

### In the VM

cfdisk /dev/vdb # creation of the partition, maybe use parted to automate this
mkswap /dev/vdb1
swapon /dev/vdb1

## Log the free memory of the VM

The free memory outside the VM and inside the VM are not the same. To
my knowledge there is no way to acquire the real memory conso of a VM
from the host side.

So to get around that problem, i will develop a small daemon that send
the result of 'free -m' in the daemon socket of the monitor of the host.
The protocol still needs to be defined

