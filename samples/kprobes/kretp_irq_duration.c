/*
 * kretp_irq_duration.c
 *
 * Based on Joel Fernandes' example in his ELCE2016 talk.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/sched.h>
#include <asm/ptrace.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>

/* copied from samples/bpf/bpf_helpers.h */
#ifdef CONFIG_X86_64
#define PT_REGS_PARM1(x) ((x)->di)
#elif defined(CONFIG_S390X)
#define PT_REGS_PARM1(x) ((x)->gprs[2])
#elif defined(CONFIG_AARCH64)
#define PT_REGS_PARM1(x) ((x)->regs[0])
#endif

static char func_name[NAME_MAX] = "handle_irq_event";
static int irq_duration = 1000;
module_param(irq_duration, int, 0);
MODULE_PARM_DESC(irq_duration, "Minimum duration of a reported IRQ handler in nS.");
MODULE_DESCRIPTION("Measure duration of irq handlers using kretprobe.");
MODULE_LICENSE("GPL");

/* per-instance private data */
struct irqprobe_data {
	ktime_t entry_stamp;
	char irqname[NAME_MAX];
};

/* Here we use the entry_hanlder to timestamp function entry */
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	struct irqprobe_data *data;
	struct irq_desc *desc;
	char *str;

	data = (struct irqprobe_data *)ri->data;
	data->entry_stamp = ktime_get();
	str = data->irqname;
	/*
	 * PT_REGS_PARM1() is defined in samples/bpf/bpf_helpers.h, but
	 * only for x86_64, s390x and aarch64.
	 *
	 * desc is the single argument to handle_irq_event() in
	 * kernel/irq/chip.c, so struct irq_desc is in its registers.
	 */
#if (defined(CONFIG_X86_64) || defined(CONFIG_ARM64))
	desc = (struct irq_desc *)PT_REGS_PARM1(regs);
#elif defined(CONFIG_ARM)
	desc = (struct irq_desc *)regs_get_register(regs, 0);
#else
	pr_err("Unsupported architecture");
	return -ENOEXEC;
#endif
	str[0] = 0;
	if (desc->action)
		sprint_symbol_no_offset(str,
					(unsigned long)desc->action->handler);
	/*
	 * Instead, skip str altogether and just use
	 * strncpy(data->irqname, desc->action->name, MAX_NAME);
	 * ?
	 */
	return 0;
}

/*
 * Return-probe handler: Log the return value and duration. Duration may turn
 * out to be zero consistently, depending upon the granularity of time
 * accounting on the platform.
 */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
/*	int retval = regs_return_value(regs); */
	struct irqprobe_data *data = (struct irqprobe_data *)ri->data;
	s64 delta;
	ktime_t now;

	now = ktime_get();
	delta = ktime_to_ns(ktime_sub(now, data->entry_stamp));
	if (delta > irq_duration)
		pr_err("IRQ: %s took %lld ns to execute\n",
			data->irqname,
			(long long)delta);
	return 0;
}

static struct kretprobe irq_kretprobe = {
	.handler		= ret_handler,
	.entry_handler		= entry_handler,
	.data_size		= sizeof(struct irqprobe_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static int __init kretprobe_init(void)
{
	int ret;

	irq_kretprobe.kp.symbol_name = func_name;
	ret = register_kretprobe(&irq_kretprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_kretprobe failed, returned %d\n",
				ret);
		return -1;
	}
	printk(KERN_INFO "Planted return probe at %s: %p\n",
			irq_kretprobe.kp.symbol_name, irq_kretprobe.kp.addr);
	return 0;
}

static void __exit kretprobe_exit(void)
{
	unregister_kretprobe(&irq_kretprobe);
	printk(KERN_INFO "kretprobe at %p unregistered\n",
			irq_kretprobe.kp.addr);

	/* nmissed > 0 suggests that maxactive was set too low. */
	printk(KERN_INFO "Missed probing %d instances of %s\n",
		irq_kretprobe.nmissed, irq_kretprobe.kp.symbol_name);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");
