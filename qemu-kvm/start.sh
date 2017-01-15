#!/bin/bash
if [ "$#" -ne 1 ]; then
   echo "Illegal usage"
   exit 1
fi 
server_type=start server_idx=$1 group_size=3 config_path=$PWD/nodes.local.cfg mgid=ff0e::ffff:e101:101 dare_log_file=$PWD/dare.log ./x86_64-softmmu/qemu-system-x86_64 /opt/vms/ubuntu-16.04.qcow2 -enable-kvm -m 2048 -smp 4 -netdev tap,id=hn0,script=/etc/qemu-ifup,downscript=/etc/qemu-ifown -device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66 --nographic 2>stderr
#
#server_type=start server_idx=1 group_size=3 config_path=/home/xusheng/VPB/qemu-kvm/nodes.local.cfg mgid=ff0e::ffff:e101:101 dare_log_file=$PWD/dare.log ./x86_64-softmmu/qemu-system-x86_64 /home/xusheng/vms/ubuntu-16.04.qcow2 -enable-kvm -m 2048 -smp 4 -netdev tap,id=hn0,script=/etc/qemu-ifup,downscript=/etc/qemu-ifown -device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66 -netdev tap,id=hn1,script=/etc/qemu-ifup,downscript=/etc/qemu-ifdown -device e1000,id=e1,netdev=hn1,mac=a8:98:2a:d0:f2:6d --nographic 2>stderr
#
#server_type=start server_idx=0 group_size=3 config_path=/home/xusheng/VPB/qemu-kvm/nodes.local.cfg mgid=ff0e::ffff:e101:101 dare_log_file=$PWD/dare.log ./x86_64-softmmu/qemu-system-x86_64 /home/xusheng/vms/ubuntu-16.04.qcow2 -enable-kvm -m 2048 -smp 4 -netdev tap,id=hn0,script=/etc/qemu-ifup,downscript=/etc/qemu-ifown -device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66 --nographic 2>stderr
#
#x86_64-softmmu/qemu-system-x86_64 /home/xusheng/vms/ubuntu-16.04.qcow2 -enable-kvm -m 2048 -smp 4 -netdev tap,id=hn0,script=/etc/qemu-ifup,downscript=/etc/qemu-ifown -device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66 --nographic
#x86_64-softmmu/qemu-system-x86_64 /home/xusheng/vms/ubuntu-16.04.qcow2 -enable-kvm -m 2048 -smp 4 -netdev tap,id=hn0,script=/etc/qemu-ifup,downscript=/etc/qemu-ifown -device e1000,id=e0,netdev=hn0,mac=52:a4:00:12:78:66 -netdev tap,id=hn1,script=/etc/qemu-ifup,downscript=/etc/qemu-ifown -device e1000,id=e1,netdev=hn1,mac=a8:98:2a:d0:f2:6d --nographic
