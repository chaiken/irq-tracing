# irq-tracing
Example code for tracing IRQs in Linux kernel
Posted as part of Embedded Linux Conference talk, Tuesday, October 11, 2016,
"IRQs: the Hard, the Soft, the Threaded and the Preemptible", http://sched.co/7rrr

Tested on Boundary Devices Nitrogren i.MX6Q board with 4.4.4-rt11 kernel, but should work
as intended on any recent PREEMPT_RT_FULL kernel with pinned Ethernet and CAN IRQs.

To trigger NAPI path of execution, I did the following:
-- started two instances of 'ping -f <board IP address>';
-- ran a shell command that triggers dozens of scp's of a large file:
   while true; do  scp /boot/vmlinuz root@<board ip addr>:/tmp; done
After about 30 seconds, the kprobe indicating NAPI path is hit.


I'd like to declare a license of GPLv2 or greater for this code, like the upstream Linux kernel,
but apparently Github does not permit this.

