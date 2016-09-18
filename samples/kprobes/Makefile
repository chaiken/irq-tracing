# builds the kprobes example kernel modules;
# then to use one (as root):  insmod <module_name.ko>

obj-$(CONFIG_SAMPLE_KPROBES) += kprobe_example.o jprobe_example.o \
				kp_ksoft.o kp_napi_complete_done.o \
				kp_do_current_softirqs.o
obj-$(CONFIG_SAMPLE_KRETPROBES) += kretprobe_example.o
