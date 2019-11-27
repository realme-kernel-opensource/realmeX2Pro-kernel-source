/************************************************************************************
** 
** VENDOR_EDIT
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
**
** Description:
**          for oppo short IC solution
**
** Version: 1.0
** Date created: 21:03:46, 05/24/2018
** Author: Tongfeng.Huang@ProDrv.CHG
**
** --------------------------- Revision History: ------------------------------------------------------------
* <version>           <date>                <author>                                <desc>
* Revision 1.0        2018-05-24        Tongfeng.Huang@ProDrv.CHG            Created for new architecture
************************************************************************************************************/

#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#ifdef CONFIG_OPPO_CHARGER_MTK
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <linux/xlog.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <soc/oppo/device_info.h>

#else
#include <linux/i2c.h>
#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <soc/oppo/device_info.h>
#endif
#include "../oppo_charger.h"
#include "oppo_short_ic.h"
#include "../oppo_vooc.h"

int oppo_short_ic_set_volt_threshold(struct oppo_chg_chip *chip)
{
        return 0;
}
bool oppo_short_ic_is_exist(struct oppo_chg_chip *chip)
{
        return false;
}

bool oppo_short_ic_otp_check(void)
{
        return true;
}
int oppo_short_ic_get_otp_error_value(struct oppo_chg_chip *chip)
{
        return 0;
}

