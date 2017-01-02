#!/bin/bash
x86_64-softmmu/qemu-system-x86_64 ../vms/ubuntu-16.04.qcow2 -enable-kvm -m 2048 -smp 4 -net nic,macaddr=52:54:01:15:a1:05,model=e1000 -net tap,ifname=tap1,script=/etc/qemu-ifup,downscript=no --nographic
