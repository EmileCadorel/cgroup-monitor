#!/bin/bash

replace=$1
cat Vagrantfile.template | sed -e "s%YMIR_HOME%$replace%g" > Vagrantfile

