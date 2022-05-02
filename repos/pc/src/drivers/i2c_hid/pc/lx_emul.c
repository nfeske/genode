/*
 * \brief  Linux emulation environment specific to this driver
 * \author Christian Helmuth
 * \date   2022-05-02
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <lx_emul.h>

#include <linux/cdev.h>

int cdev_device_add(struct cdev * cdev,struct device * dev)
{
	lx_emul_trace(__func__);
	return device_add(dev);
}


#include <linux/cdev.h>

void cdev_device_del(struct cdev * cdev,struct device * dev)
{
	lx_emul_trace(__func__);
	device_del(dev);
}


#include <linux/task_work.h>

int task_work_add(struct task_struct * task,struct callback_head * work,enum task_work_notify_mode notify)
{
	printk("%s: task: %p work: %p notify: %u\n", __func__, task, work, notify);
	return -1;
}


#include <linux/random.h>
#include <lx_emul/random.h>

u32 __get_random_u32_below(u32 ceil)
{
	/**
	 * Returns a random number from the half-open interval [0, ceil)
	 * with uniform distribution.
	 *
	 * The idea here is to split [0, 2^32) into #ceil bins. By dividing a random
	 * number from the 32-bit interval, we can determine into which bin the number
	 * fell.
	 */

	/* determine divisor to determine bin number by dividing 2^32 by ceil */
	u32 div = 0x100000000ULL / ceil;

	/**
	 * In case the above division has a remainder, we will end up with an
	 * additional (but smaller) bin at the end of the 32-bit interval. We'll
	 * discard the result if the number fell into this bin and repeat.
	 */
	u32 result = ceil;
	while (result >= ceil)
		result = lx_emul_random_gen_u32() / div;

	return result;
}


#include <linux/clkdev.h>

#define MAX_DEV_ID	20
#define MAX_CON_ID	16

struct clk_lookup_alloc {
	struct clk_lookup cl;
	char	dev_id[MAX_DEV_ID];
	char	con_id[MAX_CON_ID];
};

struct clk_lookup *clkdev_create(struct clk *clk, const char *con_id, const char *dev_fmt, ...)
{
	struct clk_lookup_alloc *cla;

	cla = kzalloc(sizeof(*cla), GFP_KERNEL);
	if (!cla)
		return NULL;

//	cla->cl.clk_hw = hw;
	if (con_id) {
		strscpy(cla->con_id, con_id, sizeof(cla->con_id));
		cla->cl.con_id = cla->con_id;
	}

	if (dev_fmt) {
		va_list ap;
		va_start(ap, dev_fmt);
		vscnprintf(cla->dev_id, sizeof(cla->dev_id), dev_fmt, ap);
		va_end(ap);
		cla->cl.dev_id = cla->dev_id;
	}

	return &cla->cl;
}
