/*
 * \brief  Linux emulation environment: GPIO ACPI shortcuts
 * \author Christian Helmuth
 * \date   2022-05-04
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is distributed under the terms of the GNU General Public License
 * version 2.
 */

#include <lx_emul.h>

#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/gpio/machine.h>
#include <gpiolib.h>
#include <gpiolib-acpi.h>


void acpi_gpiochip_request_interrupts(struct gpio_chip *chip)
{
	/* XXX only used for _AEI */
	lx_emul_trace(__func__);
}


void acpi_gpiochip_free_interrupts(struct gpio_chip *chip)
{
	lx_emul_trace(__func__);
}


void acpi_gpiochip_remove(struct gpio_chip *chip)
{
	lx_emul_trace_and_stop(__func__);
}


void acpi_gpiochip_add(struct gpio_chip *chip)
{
	struct acpi_device *adev;

	if (!chip || !chip->parent)
		return;

	adev = ACPI_COMPANION(chip->parent);
	if (!adev)
		return;
}


void acpi_gpio_dev_init(struct gpio_chip *gc, struct gpio_device *gdev)
{
	lx_emul_trace(__func__);

	/* Set default fwnode to parent's one if present */
	if (gc->parent)
		ACPI_COMPANION_SET(&gdev->dev, ACPI_COMPANION(gc->parent));
}


static int find_match_name(struct gpio_chip *gc, void *data)
{
	char const *name = data;

	return !strcmp(gc->label, name);
}


int acpi_dev_gpio_irq_wake_get_by(struct acpi_device *adev, const char *name, int index, bool *wake_capable)
{
	int irq, ret;
	struct gpio_chip *chip;
	struct gpio_desc *desc;
	char label[32];
	unsigned long lflags = GPIO_ACTIVE_LOW | GPIO_PERSISTENT;
	enum gpiod_flags dflags = GPIOD_IN;

	if (index != 0)
		return -ENOENT;

	/* most interesting part happens in gpiod_to_irq(desc) */
	chip = gpiochip_find("INT34C5:00", find_match_name);
	desc = gpiochip_get_desc(chip , 266); /* Fujitsu U7411 */
//	desc = gpiochip_get_desc(chip ,   3); /* Framework */
	irq  = gpiod_to_irq(desc);

	snprintf(label, sizeof(label), "GpioInt() %d", index);
	ret = gpiod_configure_flags(desc, label, lflags, dflags);
	if (ret < 0)
		return ret;

	ret = gpio_set_debounce_timeout(desc, 0);
	if (ret < 0)
		return ret;

	irq_set_irq_type(irq, IRQ_TYPE_LEVEL_LOW);

	return irq;
}


struct gpio_desc * acpi_find_gpio(struct device *dev, const char *con_id,
                                  unsigned int idx, enum gpiod_flags *dflags,
                                  unsigned long *lookupflags)
{
	lx_emul_trace(__func__);

	/* called from designware i2c - trace always resulted in -ENOENT */
	return ERR_PTR(-ENOENT);
}
