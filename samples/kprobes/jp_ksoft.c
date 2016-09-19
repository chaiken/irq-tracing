/*
 * jp_ksoft.c
 * Based on kernel's jprobe_example.c, but modified for ARM32 and
 * non-RT kernel NAPI tracing.
 *
 * Alison Chaiken, alison@she-devel.com
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>

/*
 * Jumper probe for __raise_softirq_irqoff().
 * Mirror principle enables access to arguments of the probed routine
 * from the probe handler.
 */

static long j__raise_softirq_irqoff(unsigned int nr);

static struct jprobe my_jprobe = {
	.entry			= j__raise_softirq_irqoff,
	.kp = {
		.symbol_name	= "__raise_softirq_irqoff",
	},
};

/* Proxy routine having the same arguments as actual
   __raise_softirq_irqoff() routine */
static long j__raise_softirq_irqoff(unsigned int nr)
{
	if (nr == NET_RX_SOFTIRQ)
		my_jprobe.kp.call_count++;

	/* Always end with a call to jprobe_return(). */
	jprobe_return();
	return 0;
}

static int __init jprobe_init(void)
{
	int ret;

	ret = register_jprobe(&my_jprobe);
	if (ret < 0) {
		printk(KERN_INFO "register_jprobe failed, returned %d\n", ret);
		return -1;
	}
	pr_info("Planted jprobe at %p, handler addr %p\n",
	       my_jprobe.kp.addr, my_jprobe.entry);
	return 0;
}

static void __exit jprobe_exit(void)
{
	pr_info("jp_ksoft: %d calls to __raise_softirq_irqoff for NET_RX\n", my_jprobe.kp.call_count);
	unregister_jprobe(&my_jprobe);
	pr_info("jprobe at %p unregistered\n", my_jprobe.kp.addr);
}

MODULE_DESCRIPTION("Print count of NAPI packet handling events from __raise_softirq_irqoff for non-RT kernel.");
MODULE_LICENSE("GPL");
module_init(jprobe_init)
module_exit(jprobe_exit)
