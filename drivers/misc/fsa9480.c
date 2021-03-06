/*
 * driver/misc/fsa9480.c - FSA9480 micro USB switch device driver
 *
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Wonguk Jeong <wonguk.jeong@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/fsa9480.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#ifdef CONFIG_MACH_WAVE
#include <linux/regulator/consumer.h>
#endif

/* FSA9480 I2C registers */
#define FSA9480_REG_DEVID		0x01
#define FSA9480_REG_CTRL		0x02
#define FSA9480_REG_INT1		0x03
#define FSA9480_REG_INT2		0x04
#define FSA9480_REG_INT1_MASK		0x05
#define FSA9480_REG_INT2_MASK		0x06
#define FSA9480_REG_ADC			0x07
#define FSA9480_REG_TIMING1		0x08
#define FSA9480_REG_TIMING2		0x09
#define FSA9480_REG_DEV_T1		0x0a
#define FSA9480_REG_DEV_T2		0x0b
#define FSA9480_REG_BTN1		0x0c
#define FSA9480_REG_BTN2		0x0d
#define FSA9480_REG_CK			0x0e
#define FSA9480_REG_CK_INT1		0x0f
#define FSA9480_REG_CK_INT2		0x10
#define FSA9480_REG_CK_INTMASK1		0x11
#define FSA9480_REG_CK_INTMASK2		0x12
#define FSA9480_REG_MANSW1		0x13
#define FSA9480_REG_MANSW2		0x14

/* Control */
#define CON_SWITCH_OPEN		(1 << 4)
#define CON_RAW_DATA		(1 << 3)
#define CON_MANUAL_SW		(1 << 2)
#define CON_WAIT		(1 << 1)
#define CON_INT_MASK		(1 << 0)
#define CON_MASK		(CON_SWITCH_OPEN | CON_RAW_DATA | \
				CON_MANUAL_SW | CON_WAIT)

/* Device Type 1 */
#define DEV_USB_OTG		(1 << 7)
#define DEV_DEDICATED_CHG	(1 << 6)
#define DEV_USB_CHG		(1 << 5)
#define DEV_CAR_KIT		(1 << 4)
#define DEV_UART		(1 << 3)
#define DEV_USB			(1 << 2)
#define DEV_AUDIO_2		(1 << 1)
#define DEV_AUDIO_1		(1 << 0)

#define DEV_T1_USB_MASK		(DEV_USB_OTG | DEV_USB)
#define DEV_T1_UART_MASK	(DEV_UART)
#define DEV_T1_CHARGER_MASK	(DEV_DEDICATED_CHG | DEV_USB_CHG | DEV_CAR_KIT)

/* Device Type 2 */
#define DEV_AV			(1 << 6)
#define DEV_TTY			(1 << 5)
#define DEV_PPD			(1 << 4)
#define DEV_JIG_UART_OFF	(1 << 3)
#define DEV_JIG_UART_ON		(1 << 2)
#define DEV_JIG_USB_OFF		(1 << 1)
#define DEV_JIG_USB_ON		(1 << 0)

#define DEV_T2_USB_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON)
#define DEV_T2_UART_MASK	DEV_JIG_UART_OFF
#define DEV_T2_JIG_MASK		(DEV_JIG_USB_OFF | DEV_JIG_USB_ON | \
				DEV_JIG_UART_OFF)

/*
 * Manual Switch
 * D- [7:5] / D+ [4:2]
 * 000: Open all / 001: USB / 010: AUDIO / 011: UART / 100: V_AUDIO
 */
#define SW_VAUDIO		((4 << 5) | (4 << 2))
#define SW_UART			((3 << 5) | (3 << 2))
#define SW_AUDIO		((2 << 5) | (2 << 2))
#define SW_DHOST		((1 << 5) | (1 << 2))
#define SW_AUTO			((0 << 5) | (0 << 2))

/* Interrupt 1 */
#define INT_DETACH		(1 << 1)
#define INT_ATTACH		(1 << 0)

struct fsa9480_usbsw {
	struct i2c_client		*client;
	struct fsa9480_platform_data	*pdata;
	int				dev1;
	int				dev2;
	int				mansw;
#ifdef CONFIG_MACH_WAVE
	struct regulator *usb_vbus_cp_regulator;
#endif
};

static struct fsa9480_usbsw *local_usbsw;

static ssize_t fsa9480_show_control(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9480_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, FSA9480_REG_CTRL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return sprintf(buf, "CONTROL: %02x\n", value);
}

static ssize_t fsa9480_show_device_type(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct fsa9480_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	int value;

	value = i2c_smbus_read_byte_data(client, FSA9480_REG_DEV_T1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	return sprintf(buf, "DEVICE_TYPE: %02x\n", value);
}

static ssize_t fsa9480_show_manualsw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fsa9480_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	unsigned int value;

	value = i2c_smbus_read_byte_data(client, FSA9480_REG_MANSW1);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if (value == SW_VAUDIO)
		return sprintf(buf, "VAUDIO\n");
	else if (value == SW_UART)
		return sprintf(buf, "UART\n");
	else if (value == SW_AUDIO)
		return sprintf(buf, "AUDIO\n");
	else if (value == SW_DHOST)
		return sprintf(buf, "DHOST\n");
	else if (value == SW_AUTO)
		return sprintf(buf, "AUTO\n");
	else
		return sprintf(buf, "%x", value);
}

static ssize_t fsa9480_set_manualsw(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct fsa9480_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
	unsigned int value;
	unsigned int path = 0;
	int ret;

	value = i2c_smbus_read_byte_data(client, FSA9480_REG_CTRL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return 0;

	if (!strncmp(buf, "VAUDIO", 6)) {
		path = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "UART", 4)) {
		path = SW_UART;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUDIO", 5)) {
		path = SW_AUDIO;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "DHOST", 5)) {
		path = SW_DHOST;
		value &= ~CON_MANUAL_SW;
	} else if (!strncmp(buf, "AUTO", 4)) {
		path = SW_AUTO;
		value |= CON_MANUAL_SW;
	} else {
		dev_err(dev, "Wrong command\n");
		return 0;
	}

	usbsw->mansw = path;

	ret = i2c_smbus_write_byte_data(client, FSA9480_REG_MANSW1, path);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client, FSA9480_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

#ifdef CONFIG_MACH_WAVE
	if(path == SW_VAUDIO)
	{
		printk("Enabling USB_VBUS_CP\n");
		regulator_enable(usbsw->usb_vbus_cp_regulator);
	}
	else
	{
		printk("Disabling USB_VBUS_CP\n");
		regulator_disable(usbsw->usb_vbus_cp_regulator);
	}
#endif

	return count;
}

static DEVICE_ATTR(control, S_IRUGO, fsa9480_show_control, NULL);
static DEVICE_ATTR(device_type, S_IRUGO, fsa9480_show_device_type, NULL);
static DEVICE_ATTR(switch, S_IRUGO | S_IWUSR,
		fsa9480_show_manualsw, fsa9480_set_manualsw);

static struct attribute *fsa9480_attributes[] = {
	&dev_attr_control.attr,
	&dev_attr_device_type.attr,
	&dev_attr_switch.attr,
	NULL
};

static const struct attribute_group fsa9480_group = {
	.attrs = fsa9480_attributes,
};

#if defined (CONFIG_MACH_ARIES)|| defined(CONFIG_MACH_WAVE)
static int cardock_enable = 0;
static int deskdock_enable = 0;

static ssize_t cardock_enable_show(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf)
{
	return sprintf(buf, "%d\n", cardock_enable);
}
static ssize_t cardock_enable_set(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	sscanf(buf, "%d\n", &cardock_enable);
	return size;
}

static ssize_t deskdock_enable_show(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf)
{
	return sprintf(buf, "%d\n", deskdock_enable);
}
static ssize_t deskdock_enable_set(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t size)
{
	sscanf(buf, "%d\n", &deskdock_enable);
	return size;
}

static DEVICE_ATTR(cardock_enable, S_IRUGO | S_IWUGO,
		cardock_enable_show, cardock_enable_set);
static DEVICE_ATTR(deskdock_enable, S_IRUGO | S_IWUGO,
		deskdock_enable_show, deskdock_enable_set);

static struct attribute *dockaudio_attributes[] = {
	&dev_attr_cardock_enable,
	&dev_attr_deskdock_enable,
	NULL
};

static struct attribute_group dockaudio_group = {
	.attrs = dockaudio_attributes,
};

static struct miscdevice dockaudio_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "dockaudio",
};

int cardock_status = 0;
int deskdock_status = 0;
#endif

void fsa9480_manual_switching(int path)
{
	struct i2c_client *client = local_usbsw->client;
	unsigned int value;
	unsigned int data = 0;
	int ret;

	value = i2c_smbus_read_byte_data(client, FSA9480_REG_CTRL);
	if (value < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, value);

	if ((value & ~CON_MANUAL_SW) !=
			(CON_SWITCH_OPEN | CON_RAW_DATA | CON_WAIT))
		return ;

	if (path == SWITCH_V_Audio_Port) {
		data = SW_VAUDIO;
		value &= ~CON_MANUAL_SW;
		printk("%s: SWITCH_V_Audio_Port (%d)\n", __func__, SWITCH_V_Audio_Port);
	} else if (path ==  SWITCH_UART_Port) {
		data = SW_UART;
		value &= ~CON_MANUAL_SW;
		printk("%s: SWITCH_UART_Port (%d)\n", __func__, SWITCH_UART_Port);
	} else if (path ==  SWITCH_Audio_Port) {
		data = SW_AUDIO;
		value &= ~CON_MANUAL_SW;
		printk("%s: SWITCH_Audio_Port (%d)\n", __func__, SWITCH_Audio_Port);
	} else if (path ==  SWITCH_USB_Port) {
		data = SW_DHOST;
		value &= ~CON_MANUAL_SW;
		printk("%s: SWITCH_USB_Port (%d)\n", __func__, SWITCH_USB_Port);
	} else if (path ==  AUTO_SWITCH) {
		data = SW_AUTO;
		value |= CON_MANUAL_SW;
		printk("%s: AUTO_SWITCH (%d)\n", __func__, AUTO_SWITCH);
	} else {
		printk("%s: wrong path (%d)\n", __func__, path);
		return ;
	}

	local_usbsw->mansw = data;

	ret = i2c_smbus_write_byte_data(client, FSA9480_REG_MANSW1, data);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	ret = i2c_smbus_write_byte_data(client, FSA9480_REG_CTRL, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

}

static void fsa9480_detect_dev(struct fsa9480_usbsw *usbsw)
{
	int device_type, ret;
	unsigned char val1, val2;
	struct fsa9480_platform_data *pdata = usbsw->pdata;
	struct i2c_client *client = usbsw->client;

	device_type = i2c_smbus_read_word_data(client, FSA9480_REG_DEV_T1);
	if (device_type < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, device_type);

	val1 = device_type & 0xff;
	val2 = device_type >> 8;
	dev_info(&client->dev, "prev_dev1: 0x%x, prev_dev2: 0x%x\n", usbsw->dev1, usbsw->dev2);
	dev_info(&client->dev, "new_dev1: 0x%x, new_dev2: 0x%x\n", val1, val2);

	/* Attached */
	if (val1 || val2) {
		/* USB */
		if (val1 & DEV_T1_USB_MASK || val2 & DEV_T2_USB_MASK) {
			if (pdata->usb_cb)
				pdata->usb_cb(FSA9480_ATTACHED);
			if (usbsw->mansw) {
				ret = i2c_smbus_write_byte_data(client,
					FSA9480_REG_MANSW1, usbsw->mansw);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			}
		/* UART */
		} else if (val1 & DEV_T1_UART_MASK || val2 & DEV_T2_UART_MASK) {
			if (pdata->uart_cb)
				pdata->uart_cb(FSA9480_ATTACHED);
#if defined(CONFIG_MACH_ARIES) || defined (CONFIG_MACH_WAVE)
			if (usbsw->mansw) {
				ret = i2c_smbus_write_byte_data(client,
					FSA9480_REG_MANSW1, SW_UART);
				if (ret < 0)
					dev_err(&client->dev,
						"%s: err %d\n", __func__, ret);
			}
#endif
		/* CHARGER */
		} else if (val1 & DEV_T1_CHARGER_MASK) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9480_ATTACHED);
		/* JIG */
		} else if (val2 & DEV_T2_JIG_MASK) {
			if (pdata->jig_cb)
				pdata->jig_cb(FSA9480_ATTACHED);
		/* Desk Dock */
		} else if (val2 & DEV_AV) {
			if (pdata->deskdock_cb)
				pdata->deskdock_cb(FSA9480_ATTACHED);

#if defined(CONFIG_MACH_ARIES) || defined(CONFIG_MACH_WAVE)
			deskdock_status = 1;
#if defined(CONFIG_SAMSUNG_CAPTIVATE) || defined(CONFIG_SAMSUNG_FASCINATE)
            ret = i2c_smbus_write_byte_data(client,
                            FSA9480_REG_MANSW1, SW_AUDIO);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);
#else
            ret = i2c_smbus_write_byte_data(client,
                            FSA9480_REG_MANSW1, SW_VAUDIO);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);
#endif
			ret = i2c_smbus_read_byte_data(client,
					FSA9480_REG_CTRL);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: err %d\n", __func__, ret);

			ret = i2c_smbus_write_byte_data(client,
				FSA9480_REG_CTRL, ret & ~CON_MANUAL_SW);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: err %d\n", __func__, ret);
#endif //CONFIG_MACH_ARIES || CONFIG_MACH_WAVE
		/* Car Dock */
		} else if (val2 & DEV_JIG_UART_ON) {
			if (pdata->cardock_cb)
				pdata->cardock_cb(FSA9480_ATTACHED);

#if defined(CONFIG_MACH_ARIES) ||defined(CONFIG_MACH_WAVE)
			cardock_status = 1;
#if defined(CONFIG_SAMSUNG_CAPTIVATE) || defined(CONFIG_SAMSUNG_FASCINATE)

            ret = i2c_smbus_write_byte_data(client,
                            FSA9480_REG_MANSW1, SW_AUDIO);

            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);
#else
            ret = i2c_smbus_write_byte_data(client,
                            FSA9480_REG_MANSW1, SW_VAUDIO);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);
#endif
            ret = i2c_smbus_read_byte_data(client,
                            FSA9480_REG_CTRL);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);

            ret = i2c_smbus_write_byte_data(client,
                    FSA9480_REG_CTRL, ret & ~CON_MANUAL_SW);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);
#endif //CONFIG_MACH_ARIES || CONFIG_MACH_WAVE
		}
	/* Detached */
	} else {
		/* USB */
		if (usbsw->dev1 & DEV_T1_USB_MASK ||
				usbsw->dev2 & DEV_T2_USB_MASK) {
			if (pdata->usb_cb)
				pdata->usb_cb(FSA9480_DETACHED);
		/* UART */
		} else if (usbsw->dev1 & DEV_T1_UART_MASK ||
				usbsw->dev2 & DEV_T2_UART_MASK) {
			if (pdata->uart_cb)
				pdata->uart_cb(FSA9480_DETACHED);
		/* CHARGER */
		} else if (usbsw->dev1 & DEV_T1_CHARGER_MASK) {
			if (pdata->charger_cb)
				pdata->charger_cb(FSA9480_DETACHED);
		/* JIG */
		} else if (usbsw->dev2 & DEV_T2_JIG_MASK) {
			if (pdata->jig_cb)
				pdata->jig_cb(FSA9480_DETACHED);
		/* Desk Dock */
		} else if (usbsw->dev2 & DEV_AV) {
			if (pdata->deskdock_cb)
				pdata->deskdock_cb(FSA9480_DETACHED);
#if defined(CONFIG_MACH_ARIES) || defined(CONFIG_MACH_WAVE)
			deskdock_status = 0;

			ret = i2c_smbus_read_byte_data(client,
					FSA9480_REG_CTRL);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: err %d\n", __func__, ret);

			ret = i2c_smbus_write_byte_data(client,
					FSA9480_REG_CTRL, ret | CON_MANUAL_SW);
			if (ret < 0)
				dev_err(&client->dev,
					"%s: err %d\n", __func__, ret);
#endif
		/* Car Dock */
		} else if (usbsw->dev2 & DEV_JIG_UART_ON) {
			if (pdata->cardock_cb)
				pdata->cardock_cb(FSA9480_DETACHED);

#if defined(CONFIG_MACH_ARIES) || defined(CONFIG_MACH_WAVE)
			cardock_status = 0;

            ret = i2c_smbus_read_byte_data(client,
                            FSA9480_REG_CTRL);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);

            ret = i2c_smbus_write_byte_data(client,
                            FSA9480_REG_CTRL, ret | CON_MANUAL_SW);
            if (ret < 0)
                    dev_err(&client->dev,
                            "%s: err %d\n", __func__, ret);
#endif
		}
	}

	usbsw->dev1 = val1;
	usbsw->dev2 = val2;
}

#if defined(CONFIG_MACH_ARIES) || defined(CONFIG_MACH_WAVE)
int fsa9480_get_dock_status(void)
{
	if ((cardock_status && cardock_enable) ||
	    (deskdock_status && deskdock_enable))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(fsa9480_get_dock_status);
#endif

static void fsa9480_reg_init(struct fsa9480_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	unsigned int ctrl = CON_MASK;
	int ret;

	/* mask interrupts (unmask attach/detach only) */
	ret = i2c_smbus_write_word_data(client, FSA9480_REG_INT1_MASK, 0x1ffc);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* mask all car kit interrupts */
	ret = i2c_smbus_write_word_data(client, FSA9480_REG_CK_INTMASK1, 0x07ff);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	/* ADC Detect Time: 500ms */
	ret = i2c_smbus_write_byte_data(client, FSA9480_REG_TIMING1, 0x6);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	usbsw->mansw = i2c_smbus_read_byte_data(client, FSA9480_REG_MANSW1);
	if (usbsw->mansw < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, usbsw->mansw);

	if (usbsw->mansw)
		ctrl &= ~CON_MANUAL_SW;	/* Manual Switching Mode */

	ret = i2c_smbus_write_byte_data(client, FSA9480_REG_CTRL, ctrl);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);
}

static irqreturn_t fsa9480_irq_thread(int irq, void *data)
{
	struct fsa9480_usbsw *usbsw = data;
	struct i2c_client *client = usbsw->client;
	int intr;
	int max_events = 100;
	int events_seen = 0;

	/*
	 * the fsa could have queued up a few events if we haven't processed
	 * them promptly
	 */
	while (max_events-- > 0) {
		intr = i2c_smbus_read_word_data(client, FSA9480_REG_INT1);
		if (intr < 0)
			dev_err(&client->dev, "%s: err %d\n", __func__, intr);
		else if (intr == 0)
			break;
		else if (intr > 0)
			events_seen++;
	}
	if (!max_events)
		dev_warn(&client->dev, "too many events. fsa hosed?\n");

	if (!events_seen) {
		/*
		 * interrupt was fired, but no status bits were set,
		 * so device was reset. In this case, the registers were
		 * reset to defaults so they need to be reinitialised.
		 */
		fsa9480_reg_init(usbsw);
	}

	/*
	 * fsa may take some time to update the dev_type reg after reading
	 * the int reg.
	 */
	usleep_range(200, 300);

	/* device detection */
	fsa9480_detect_dev(usbsw);

	return IRQ_HANDLED;
}

static int fsa9480_irq_init(struct fsa9480_usbsw *usbsw)
{
	struct i2c_client *client = usbsw->client;
	int ret;

	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL,
			fsa9480_irq_thread, IRQF_TRIGGER_LOW | IRQF_ONESHOT,
			"fsa9480 micro USB", usbsw);
		if (ret) {
			dev_err(&client->dev, "failed to reqeust IRQ\n");
			return ret;
		}

		ret = enable_irq_wake(client->irq);
		if (ret < 0)
			dev_err(&client->dev,
				"failed to enable wakeup src %d\n", ret);
	}

	return 0;
}

static int __devinit fsa9480_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fsa9480_usbsw *usbsw;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	usbsw = kzalloc(sizeof(struct fsa9480_usbsw), GFP_KERNEL);
	if (!usbsw) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		return -ENOMEM;
	}

	usbsw->client = client;
	usbsw->pdata = client->dev.platform_data;
	if (!usbsw->pdata)
		goto fail1;

	i2c_set_clientdata(client, usbsw);

	if (usbsw->pdata->cfg_gpio)
		usbsw->pdata->cfg_gpio();

	fsa9480_reg_init(usbsw);
	local_usbsw = usbsw;  // temp

	ret = fsa9480_irq_init(usbsw);
	if (ret)
		goto fail1;

	ret = sysfs_create_group(&client->dev.kobj, &fsa9480_group);
	if (ret) {
		dev_err(&client->dev,
				"failed to create fsa9480 attribute group\n");
		goto fail2;
	}

	if (usbsw->pdata->reset_cb)
		usbsw->pdata->reset_cb();

#ifdef CONFIG_MACH_WAVE
	if (IS_ERR_OR_NULL(usbsw->usb_vbus_cp_regulator)) {
		usbsw->usb_vbus_cp_regulator = regulator_get(NULL, "usb_vbus_cp");
		if (IS_ERR_OR_NULL(usbsw->usb_vbus_cp_regulator)) {
			pr_err("failed to get usb_vbus_cp regulator");
			goto fail2;
		}
	}
#endif
	/* device detection */
	fsa9480_detect_dev(usbsw);

#if defined(CONFIG_SAMSUNG_CAPTIVATE) || defined(CONFIG_SAMSUNG_FASCINATE)
	if (misc_register(&dockaudio_device))
		printk("%s misc_register(%s) failed\n", __FUNCTION__, dockaudio_device.name);
	else {
		if (sysfs_create_group(&dockaudio_device.this_device->kobj, &dockaudio_group) < 0)
			dev_err("failed to create sysfs group for device %s\n", dockaudio_device.name);
	}
#endif

	return 0;

fail2:
	if (client->irq)
		free_irq(client->irq, usbsw);
fail1:
	i2c_set_clientdata(client, NULL);
	kfree(usbsw);
	return ret;
}

static int __devexit fsa9480_remove(struct i2c_client *client)
{
	struct fsa9480_usbsw *usbsw = i2c_get_clientdata(client);

#if defined(CONFIG_SAMSUNG_CAPTIVATE) || defined(CONFIG_SAMSUNG_FASCINATE)
	misc_deregister(&dockaudio_device);
#endif

	if (client->irq) {
		disable_irq_wake(client->irq);
		free_irq(client->irq, usbsw);
	}
	i2c_set_clientdata(client, NULL);

	sysfs_remove_group(&client->dev.kobj, &fsa9480_group);
	kfree(usbsw);
	return 0;
}

#ifdef CONFIG_PM
static int fsa9480_resume(struct device* dev)
{
	struct fsa9480_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;

	if (client->irq)
		enable_irq(client->irq);
	/* device detection */
	fsa9480_detect_dev(usbsw);

	return 0;
}

static int fsa9480_suspend(struct device* dev)
{
	struct fsa9480_usbsw *usbsw = dev_get_drvdata(dev);
	struct i2c_client *client = usbsw->client;
 
	if (client->irq)
		disable_irq(client->irq);

	return 0;
}

static const struct dev_pm_ops fsa9480_pm_ops = {
	.suspend = fsa9480_suspend,
	.resume = fsa9480_resume,
};
#endif /* CONFIG_PM */

static const struct i2c_device_id fsa9480_id[] = {
	{"fsa9480", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fsa9480_id);

static struct i2c_driver fsa9480_i2c_driver = {
	.driver = {
		.name = "fsa9480",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &fsa9480_pm_ops,
#endif
	},
	.probe = fsa9480_probe,
	.remove = __devexit_p(fsa9480_remove),
	.id_table = fsa9480_id,
};

static int __init fsa9480_init(void)
{
	return i2c_add_driver(&fsa9480_i2c_driver);
}
module_init(fsa9480_init);

static void __exit fsa9480_exit(void)
{
	i2c_del_driver(&fsa9480_i2c_driver);
}
module_exit(fsa9480_exit);

MODULE_AUTHOR("Minkyu Kang <mk7.kang@samsung.com>");
MODULE_DESCRIPTION("FSA9480 USB Switch driver");
MODULE_LICENSE("GPL");
