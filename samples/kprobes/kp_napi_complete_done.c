/*
 * kp_napi_complete.c
 * Based on kernel's kprobe_example.c, but modified for ARM32 and
 * PREEMPT_RT_FULL NAPI tracing.
 *
 * Alison Chaiken, alison@she-devel.com
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <asm/ptrace.h>
#include <linux/moduleparam.h>

static unsigned long proc_mode;
unsigned int id;
static int eth_irq_procid = 9;
static int can_irq_procid = 10;
module_param(eth_irq_procid, int, 0);
module_param(can_irq_procid, int, 0);
MODULE_PARM_DESC(eth_irq_procid, "Set to the number of the core where eth IRQ runs.");
MODULE_PARM_DESC(can_irq_procid, "Set to the number of the core where CAN IRQ runs.");
MODULE_DESCRIPTION("Print count of NAPI complete events in PREEMPT_RT_FULL");
MODULE_LICENSE("GPL");

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp = {
	.symbol_name	= "napi_complete_done",
};

/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
	proc_mode = processor_mode(regs);
	p->call_count++;

	/* change id to that where the eth IRQ is pinned */
	if (id == eth_irq_procid)
		WARN_ONCE(1, "ETH NAPI.\n");
#ifdef CONFIG_CAN
	/* change id to that where the CAN IRQ is pinned */
	if (id == can_irq_procid)
		WARN_ONCE(1, "CAN NAPI.\n");
#endif
	return 0;
}

/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs,
				unsigned long flags)
{
	id = smp_processor_id();
	proc_mode = processor_mode(regs);

#ifdef CONFIG_ARM
	if (p->call_count == 1)
		pr_info("post_handler: p->addr = 0x%p, lr = 0x%lx, interrupts enabled: %s, IRQ_MODE %s\n",
			p->addr, regs->ARM_lr, interrupts_enabled(regs) ?
			"yes" : "no", (proc_mode == IRQ_MODE) ? "ON" : "OFF");
#endif
}

/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
	pr_info("fault_handler: p->addr = 0x%p, trap #%dn",
		p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}

static int __init kprobe_init(void)
{
	int ret;

	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;

	ret = register_kprobe(&kp);
	if (ret < 0) {
		pr_info("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_info("Planted kprobe at %p\n", kp.addr);
	return 0;
}

static void __exit kprobe_exit(void)
{
	pr_info("napi_complete_done: %u hits", kp.call_count);
	unregister_kprobe(&kp);
	pr_info("kprobe at %p unregistered\n", kp.addr);
}

module_init(kprobe_init)
module_exit(kprobe_exit)
MODULE_LICENSE("GPL");
