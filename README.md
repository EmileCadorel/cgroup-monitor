# Monitor

## COMPILATION 

The compilation is made using cmake : 

```bash
$ mkdir .build
$ cd .build
$ cmake ..
$ make 
```

It can also be done using vagrant to create a releasable binary : 

```bash
$ cd vagrant
$ ./configure > /tmp/
$ vagrant up
$ vagrant ssh -c "bash -s" < ./build.sh
vagrant@192.168.121.115's password: vagrant

$ ls /tmp/dio/bin/
libdio_1.0.deb
```


## Installation 

The binary can be installed on debian distribution : 

```bash
$ sudo dpkg -i libdio_1.0.deb
```

It gives access to two commands : `dio-monitor` and `dio-client`.

## Dio-monitor

The dio-monitor command is the controller and monitor. It is configured by the file : `/usr/lib/dio/cpu-market.json`

```json
{
    "enable" : true,
    "frequency" : 3000,
    "trigger-increment" : 95.0,
    "trigger-decrement" : 50.0,
    "increment-speed" : 100.0,
    "decrement-speed" : 20.0,
    "window-size" : 100000
}
```

- `enable`: if true, the controller is perfoming frequency capping
- `frequency`: The maximum frequency of the host node
- `trigger-increment`: percentage of usage that trigger increment of the capping of the vCPU frequency
- `trigger-decrement`: percentage of usage that trigger decrement of the capping of the vCPU frequency
- `increment-speed`: percentage of increase of the capping when increment is triggered
- `decrement-speed`: percentage of decrease of the capping when decrement is triggered


The `dio-monitor` is running a tcp server waiting for client commands.
The `dio-monitor` is dumping controlling and monitoring information in file `/var/log/dio/control-log.json`.

## Dio-client

The `dio-client` is the command used to provision and kill VMs. It connects to the `dio-monitor` running on the host node.

### provisionning

Using a vm configuration file.

```bash
$ dio-client --provision example.toml
```

```toml
[vm]
name = "v1"
image = "/home/phil/.qcow2/ubuntu-20.04.qcow2"
ssh_key = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDJe3QVm7nA05wZAVGhcZT4Rv8Uvkox3PlGfisP2KMHQNhdpLseTWGk6iuB/PylnEhP53dLyHucXuYHXk6rbs4ZxtM7/i8AWvEp/Pew1lJshlCO+OT80FLbdohbOtJXYmZuvy6WAZAd5hPXOPqT4IM0Kxqo6ehRXWEovyfO0+drlZFQNuMhNu9OfJaQCQzKILCZJ9yux6haMNo62L3VAOsRUtzC2AdPAdzIhZSMgkz7KQao16fXRkhMJufl0z9qTL6tkzmyBmzSGJK0EpHYapiScz51mdH//zzskp4SVCkxrg/k7ZR4U9uXtN8yfWtrVX+A9I0o4ydFG4irze3sa7Tt emile@emile-XPS-13-7390"
vcpus = 4
memory = 4096
frequency = 1000
disk = 10000
```

The image `/home/phil/.qcow2/ubuntu-20.04.qcow2` must be pre-downloaded. For example: 

```bash
$ wget http://cloud-images.ubuntu.com/releases/focal/release-20210921/ubuntu-20.04-server-cloudimg-amd64.img -O /home/phil/.qcow2/ubuntu-20.04.qcow2
```

The `ssh_key`, is the public part of the a generated ssh key that will be usable to access the VM using ssh.

### Killing

Using the name of the VM to kill:

```bash
$ dio-client --kill v1
```

### Nat 

To open a port in order to access the VM, for example on a machine whose IP is `192.168.158.62` :

```bash
$ dio-client --nat v1 --host 2020 --guest 22
$ ssh phil@192.168.158.62 -p 2020 -i my_private_key
Welcome to Ubuntu 20.04.3 LTS (GNU/Linux 5.4.0-86-generic x86_64)

 * Documentation:  https://help.ubuntu.com
 * Management:     https://landscape.canonical.com
 * Support:        https://ubuntu.com/advantage

  System information as of Tue May 17 09:07:05 UTC 2022

  System load:  1.71               Processes:             137
  Usage of /:   11.3% of 11.43GB   Users logged in:       0
  Memory usage: 5%                 IPv4 address for ens3: 192.168.122.137
  Swap usage:   0%


0 updates can be applied immediately.


The list of available updates is more than a week old.
To check for new updates run: sudo apt update

phil@vv1:~$
```

## Tests

There a files to test the controller, all of them are located in `test` directory. 


