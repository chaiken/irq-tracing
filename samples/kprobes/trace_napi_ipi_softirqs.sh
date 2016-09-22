#!/bin/bash
#alison@she-devel.com
#script to watch NAPI events for i.MX6 in 4.4.4-rt11 linux-rt
#
#Usage: compare timestamp of first napi_poll events in trace buffer to
#that for the first task->comm events for ksoftirqd appear in dmesg.
#The coincidence in time is a sign of NAPI.
#
#Also, upon unloading the module, compare the total incidence of calls
#to the function from __local_bh_enable(), meaning the softirq ran
#immediately after the Eth interrupt, to those from ksoftirqd, meaning
#that the softirq ran in ksoftirqd, once again signalling NAPI.

cd /sys/kernel/debug/tracing/events
cat /dev/null > ../trace

#root@nitrogen6x:/sys/kernel/debug/tracing/events# ps | grep irq/43
#  138 root         0 SW   [irq/43-2188000.]
PID=$(ps | grep irq/43 | grep -v grep | awk {'print $1;'})
echo common_pid==$PID > napi/napi_poll/filter

# skip Timer Broadcast IPIs
echo target_cpus!=7 > ipi/ipi_raise/filter
echo common_pid==$PID >> ipi/ipi_raise/filter
echo "ipi/ipi_raise/filter:"
cat ipi/ipi_raise/filter

echo "napi/napi_poll/filter:"
cat napi/napi_poll/filter

echo 1 > napi/napi_poll/enable
echo 1 > ipi/ipi_raise/enable

modprobe kp_do_current_softirqs chatty=1
dmesg | tail

#from laptop, start instances of ping-flood until dmesg | tail shows
#kprobe output. 

#echo 0 > ipi/ipi_raise/enable
#echo 0 > napi/napi_poll/enable
#modprobe -r kp_do_current_softirqs

