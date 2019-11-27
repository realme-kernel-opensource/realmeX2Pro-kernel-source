/*For OEM project monitor RF cable connection status,
 * and config different RF configuration
 */

#include <linux/export.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/sys_soc.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/types.h>
#include <soc/oppo/oppo_project.h>
#include <soc/oppo/boot_mode.h>
#include <linux/soc/qcom/smem.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/kobject.h>
#include <linux/pm_wakeup.h>

#define RF_CABLE_OUT		0
#define RF_CABLE_IN			1

#define CABLE_0				0
#define CABLE_1				1

extern void op_restart_modem(void);

#define PAGESIZE 512
#define SMEM_DRAM_TYPE 136

struct rf_info_type
{
	unsigned char		rf_cable_sts;
	unsigned int        nproject;
	unsigned char		nmodem;
	unsigned char		noperator;
	unsigned char		npcbversion;
	unsigned char		noppobootmode;
};

struct rf_cable_data {
    int cable0_irq;
    int cable1_irq;
    int cable0_gpio;
    int cable1_gpio;
    struct work_struct cable0_sts_work;
	struct work_struct cable1_sts_work;
    struct device *dev;
    struct wakeup_source cable_ws;
	int cable0_irq_type;
	int cable0_val;
	int cable0_val_pre;
	int cable1_irq_type;
	int cable1_val;
	int cable1_val_pre;
	struct mutex cable_mutex;
    struct pinctrl *gpio_pinctrl;
    struct pinctrl_state *gpio_pinctrl_active;
    struct pinctrl_state *gpio_pinctrl_suspend;
	struct kobject	kobj;
};

enum
{
  SMEM_APPS         =  0,                     /**< Apps Processor */
  SMEM_MODEM        =  1,                     /**< Modem processor */
  SMEM_ADSP         =  2,                     /**< ADSP processor */
  SMEM_SSC          =  3,                     /**< Sensor processor */
  SMEM_WCN          =  4,                     /**< WCN processor */
  SMEM_CDSP         =  5,                     /**< Reserved */
  SMEM_RPM          =  6,                     /**< RPM processor */
  SMEM_TZ           =  7,                     /**< TZ processor */
  SMEM_SPSS         =  8,                     /**< Secure processor */
  SMEM_HYP          =  9,                     /**< Hypervisor */
  SMEM_NUM_HOSTS    = 10,                     /**< Max number of host in target */
};

static struct rf_cable_data *the_rf_data = NULL;
static struct rf_info_type *the_rf_format = NULL;

static irqreturn_t cable0_interrupt(int irq, void *_dev);
static irqreturn_t cable1_interrupt(int irq, void *_dev);

static int rf_cable_initial_request_irq(struct rf_cable_data *rf_data, int cable_num)
{
	int rc = 0;
	
	if (cable_num == 0) {
		if (rf_data->cable0_val) {
			rc = request_irq(rf_data->cable0_irq, cable0_interrupt,
				IRQF_TRIGGER_FALLING, "rf_cable0_irq", NULL);
			if (rc) {
				pr_err("%s cable_num:%d, request falling fail\n",
					__func__, cable_num);
				return rc;
			} else {
				rf_data->cable0_irq_type = IRQF_TRIGGER_FALLING;
				pr_err("%s cable_num:%d, request falling success\n",
					__func__, cable_num);
			}
			enable_irq_wake(rf_data->cable0_irq);
		} else {
			rc = request_irq(rf_data->cable0_irq, cable0_interrupt,
				IRQF_TRIGGER_RISING, "rf_cable0_irq", NULL);
			if (rc) {
				pr_err("%s cable_num:%d, request rising fail\n",
					__func__, cable_num);
				return rc;
			} else {
				rf_data->cable0_irq_type = IRQF_TRIGGER_RISING;
				pr_err("%s cable_num:%d, request rising success\n",
					__func__, cable_num);
			}
			//enable_irq_wake(rf_data->cable0_irq);
		}
	} else {
		if (rf_data->cable1_val) {
			rc = request_irq(rf_data->cable1_irq, cable1_interrupt,
				IRQF_TRIGGER_FALLING, "rf_cable1_irq", NULL);
			if (rc) {
				pr_err("%s cable_num:%d, request falling fail\n",
					__func__, cable_num);
				return rc;
			} else {
				rf_data->cable1_irq_type = IRQF_TRIGGER_FALLING;
				pr_err("%s cable_num:%d, request falling success\n",
					__func__, cable_num);
			}
			enable_irq_wake(rf_data->cable1_irq);
		} else {
			rc = request_irq(rf_data->cable1_irq, cable1_interrupt,
				IRQF_TRIGGER_RISING, "rf_cable1_irq", NULL);
			if (rc) {
				pr_err("%s cable_num:%d, request rising fail\n",
					__func__, cable_num);
				return rc;
			} else {
				rf_data->cable1_irq_type = IRQF_TRIGGER_RISING;
				pr_err("%s cable_num:%d, request rising success\n",
					__func__, cable_num);
			}
			//enable_irq_wake(rf_data->cable1_irq);
		}
	}
	return 0;
}



static void modify_rf_cable_smem_info(struct rf_cable_data *rf_data, int cable_num)
{
	if (!the_rf_format) {
		pr_err("%s the_rf_format NULL, cable_num:%d\n", __func__, cable_num);
		return;
	}

	mutex_lock(&rf_data->cable_mutex);
	pr_err("modify num:%d, cable0:%d, cable0_pre:%d, cable1:%d, cable1_pre:%d\n",
		cable_num, rf_data->cable0_val, rf_data->cable0_val_pre,
		rf_data->cable1_val, rf_data->cable1_val_pre);
	if (cable_num == CABLE_0) {
		if (rf_data->cable0_val != rf_data->cable0_val_pre) {
			the_rf_format->rf_cable_sts = !!rf_data->cable0_val;
			//op_restart_modem();
			rf_data->cable0_val_pre = rf_data->cable0_val;
		}
	} else if (cable_num == CABLE_1) {
		if (rf_data->cable1_val != rf_data->cable1_val_pre) {
			the_rf_format->rf_cable_sts = !!rf_data->cable1_val;
			//op_restart_modem();
			rf_data->cable1_val_pre = rf_data->cable1_val;
		}
	}
	mutex_unlock(&rf_data->cable_mutex);
}

static void rf_cable0_work(struct work_struct *work)
{
	struct rf_cable_data *rf_data =
  		container_of(work, struct rf_cable_data, cable0_sts_work);
	int gpio_val = 0;

	switch(rf_data->cable0_irq_type) {
		case IRQF_TRIGGER_RISING:
			usleep_range(50000, 50050);
			gpio_val = gpio_get_value(rf_data->cable0_gpio);
			if (gpio_val) {
				rf_data->cable0_val = gpio_val;
				modify_rf_cable_smem_info(rf_data, CABLE_0);
				pr_err("%s ready to irq_type:%d, gpio_val:%d\n", __func__,
					rf_data->cable0_irq_type, gpio_val);
				irq_set_irq_type(rf_data->cable0_irq, IRQF_TRIGGER_FALLING);
				rf_data->cable0_irq_type = IRQF_TRIGGER_FALLING;
				enable_irq(rf_data->cable0_irq);
			} else {
				pr_err("%s irq_type invalid:%d, gpio_val:%d\n", __func__, 
					rf_data->cable0_irq_type, gpio_val);
				enable_irq(rf_data->cable0_irq);
			}
			break;
		case IRQF_TRIGGER_FALLING:
			usleep_range(50000, 50050);
			gpio_val = gpio_get_value(rf_data->cable0_gpio);
			if (gpio_val == 0) {
				rf_data->cable0_val = gpio_val;
				modify_rf_cable_smem_info(rf_data, CABLE_0);
				pr_err("%s ready to irq_type:%d, gpio_val:%d\n", __func__,
					rf_data->cable0_irq_type, gpio_val);
				irq_set_irq_type(rf_data->cable0_irq, IRQF_TRIGGER_RISING);
				rf_data->cable0_irq_type = IRQF_TRIGGER_RISING;
				enable_irq(rf_data->cable0_irq);
			} else {
				pr_err("%s irq_type invalid:%d, gpio_val:%d\n", __func__, 
					rf_data->cable0_irq_type, gpio_val);
				enable_irq(rf_data->cable0_irq);
			}
			break;
		default:
			pr_err("%s irq_type:%d, gpio_val:%d error\n", __func__,
				rf_data->cable0_irq_type, gpio_val);
			break;
	}
	__pm_relax(&rf_data->cable_ws);
}

static void rf_cable1_work(struct work_struct *work)
{
	struct rf_cable_data *rf_data =
  		container_of(work, struct rf_cable_data, cable1_sts_work);
	int gpio_val = 0;

	switch(rf_data->cable1_irq_type) {
		case IRQF_TRIGGER_RISING:
			usleep_range(50000, 50050);
			gpio_val = gpio_get_value(rf_data->cable1_gpio);
			if (gpio_val) {
				rf_data->cable1_val = gpio_val;
				modify_rf_cable_smem_info(rf_data, CABLE_1);
				pr_err("%s ready to irq_type:%d, gpio_val:%d\n", __func__,
					rf_data->cable1_irq_type, gpio_val);
				irq_set_irq_type(rf_data->cable1_irq, IRQF_TRIGGER_FALLING);
				rf_data->cable1_irq_type = IRQF_TRIGGER_FALLING;
				enable_irq(rf_data->cable1_irq);
			} else {
				pr_err("%s irq_type invalid:%d, gpio_val:%d\n", __func__, 
					rf_data->cable1_irq_type, gpio_val);
				enable_irq(rf_data->cable1_irq);
			}
			break;
		case IRQF_TRIGGER_FALLING:
			usleep_range(50000, 50050);
			gpio_val = gpio_get_value(rf_data->cable1_gpio);
			if (gpio_val == 0) {
				rf_data->cable1_val = gpio_val;
				modify_rf_cable_smem_info(rf_data, CABLE_1);
				pr_err("%s ready to irq_type:%d, gpio_val:%d\n", __func__,
					rf_data->cable1_irq_type, gpio_val);
				irq_set_irq_type(rf_data->cable1_irq, IRQF_TRIGGER_RISING);
				rf_data->cable1_irq_type = IRQF_TRIGGER_RISING;
				enable_irq(rf_data->cable1_irq);
			} else {
				pr_err("%s irq_type invalid:%d, gpio_val:%d\n", __func__, 
					rf_data->cable1_irq_type, gpio_val);
				enable_irq(rf_data->cable1_irq);
			}
			break;
		default:
			pr_err("%s default irq_type:%d, gpio_val:%d\n", __func__,
				rf_data->cable1_irq_type, gpio_val);
			break;
	}
	__pm_relax(&rf_data->cable_ws);
}

static irqreturn_t cable0_interrupt(int irq, void *_dev)
{
	static bool print_log = false; 

	pr_err("%s enter\n", __func__);
	if (!the_rf_data) {
		if (print_log == false) {
			pr_err("%s the_rf_data null\n", __func__);
		}
		print_log = true;
		return IRQ_HANDLED;
	}
	disable_irq_nosync(the_rf_data->cable0_irq);
	__pm_stay_awake(&the_rf_data->cable_ws);
	schedule_work(&the_rf_data->cable0_sts_work);

    return IRQ_HANDLED;
}

static irqreturn_t cable1_interrupt(int irq, void *_dev)
{
	static bool print_log = false; 

	pr_err("%s enter\n", __func__);
	if (!the_rf_data) {
		if (print_log == false) {
			pr_err("%s the_rf_data null\n", __func__);
		}
		print_log = true;
		return IRQ_HANDLED;
	}
	disable_irq_nosync(the_rf_data->cable1_irq);
	__pm_stay_awake(&the_rf_data->cable_ws);
	schedule_work(&the_rf_data->cable1_sts_work);

    return IRQ_HANDLED;
}

static int rf_cable_gpio_pinctrl_init
	(struct platform_device *pdev, struct device_node *np, struct rf_cable_data *rf_data)
{
    int retval = 0;

	//request gpio 0 .gpio 1.
    rf_data->cable0_gpio = of_get_named_gpio(np, "rf,cable0-gpio", 0);
    if (!gpio_is_valid(rf_data->cable0_gpio)) {
        pr_err("%s: cable0_gpio invalid\n",
            __func__);
        goto exit_gpio;
    }
    rf_data->cable1_gpio = of_get_named_gpio(np, "rf,cable1-gpio", 0);
    if (!gpio_is_valid(rf_data->cable1_gpio)) {
        pr_err("%s: cable1_gpio invalid\n",
            __func__);
        goto exit_gpio;
    }
	
    rf_data->gpio_pinctrl = devm_pinctrl_get(&(pdev->dev));
	
	if (rf_data->gpio_pinctrl == NULL) {
        retval = PTR_ERR(rf_data->gpio_pinctrl);
		pr_err("%s get gpio_pinctrl null, retval:%d\n", __func__, retval);
        goto err_pinctrl_get;
    }
		
	if (IS_ERR_OR_NULL(rf_data->gpio_pinctrl)) {
        retval = PTR_ERR(rf_data->gpio_pinctrl);
		pr_err("%s get gpio_pinctrl fail, retval:%d\n", __func__, retval);
        goto err_pinctrl_get;
    }

    rf_data->gpio_pinctrl_active
        		= pinctrl_lookup_state(rf_data->gpio_pinctrl, "rf_cable_active");
    if (IS_ERR_OR_NULL(rf_data->gpio_pinctrl_active)) {
        retval = PTR_ERR(rf_data->gpio_pinctrl_active);
		pr_err("%s get rf_cable_active fail, retval:%d\n", __func__, retval);
        goto err_pinctrl_lookup;
    }

    if (rf_data->gpio_pinctrl) {
        retval = pinctrl_select_state(rf_data->gpio_pinctrl,
                    rf_data->gpio_pinctrl_active);
        if (retval < 0) {
            pr_err("%s select pinctrl fail, retval:%d\n", __func__, retval);
			goto err_pinctrl_lookup;
        }
    }
	gpio_direction_input(rf_data->cable0_gpio);
    gpio_direction_input(rf_data->cable1_gpio);
	
    return 0;

err_pinctrl_lookup:
    devm_pinctrl_put(rf_data->gpio_pinctrl);
err_pinctrl_get:
    rf_data->gpio_pinctrl = NULL;
exit_gpio:
    return -1;
}

static int op_rf_cable_probe(struct platform_device *pdev)
{
    int rc = 0, ret = 0;
	size_t smem_size;
    struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct rf_cable_data *rf_data = NULL;
	unsigned int len = (sizeof(struct rf_info_type) + 3)&(~0x3);

	pr_err("%s enter!\n", __func__);
	#if defined(PROJECT_19688)
	
	#else
	if ((is_project(OPPO_18501) || is_project(OPPO_18503)) && get_Operator_Version() == OPERATOR_FOREIGN) {
		//support rf_cable detect
	} else {
		pr_err("%s operator:%d, pcb_ver:%d, no rf_cable_gpio\n", __func__,
			get_Operator_Version(), get_PCB_Version());
		return -1;
	}
	#endif
	ret = qcom_smem_alloc(SMEM_APPS, SMEM_DRAM_TYPE, len);
	if (ret < 0 && ret != -EEXIST) {
		pr_err("%s smem_alloc fail\n", __func__);
		return -EFAULT;
	}
	
	the_rf_format = (struct rf_info_type*)qcom_smem_get(SMEM_APPS, SMEM_DRAM_TYPE, &smem_size);
	if (IS_ERR(the_rf_format)) {
		pr_err("%s smem_get fail\n", __func__);
		return -EFAULT;
	}
	
    if (get_boot_mode() == MSM_BOOT_MODE__RF || get_boot_mode() == MSM_BOOT_MODE__WLAN) {
        pr_err("%s: rf/wlan mode FOUND! use 1 always\n", __func__);
		the_rf_format->rf_cable_sts = RF_CABLE_IN;
		return 0;
    }
	
    rf_data = kzalloc(sizeof(struct rf_cable_data), GFP_KERNEL);
    if (!rf_data) {
        pr_err("%s: failed to allocate memory\n", __func__);
        rc = -ENOMEM;
        goto exit_nomem;
    }

    rf_data->dev = dev;
    dev_set_drvdata(dev, rf_data);
	wakeup_source_init(&rf_data->cable_ws, "rf_cable_wake_lock");
    mutex_init(&rf_data->cable_mutex);
    INIT_WORK(&rf_data->cable0_sts_work, rf_cable0_work);
	INIT_WORK(&rf_data->cable1_sts_work, rf_cable1_work);
	
	rc = rf_cable_gpio_pinctrl_init(pdev, np, rf_data);
	if (rc) {
		pr_err("%s gpio_init fail\n", __func__);
		goto exit;
	}

    //set input  and gpio to irq.
    rf_data->cable0_irq = gpio_to_irq(rf_data->cable0_gpio);
    if (rf_data->cable0_irq < 0) {
        pr_err("Unable to get irq number for GPIO %d, error %d\n",
            rf_data->cable0_gpio, rf_data->cable0_irq);
        rc = -1;
        goto exit;
    }

    rf_data->cable1_irq = gpio_to_irq(rf_data->cable1_gpio);
    if (rf_data->cable1_irq < 0) {
        pr_err("Unable to get irq number for GPIO %d, error %d\n",
            rf_data->cable1_gpio, rf_data->cable1_irq);
		rc = -1;
        goto exit;
    }

	mdelay(5);
	rf_data->cable0_val = gpio_get_value(rf_data->cable0_gpio);
	rf_data->cable1_val = gpio_get_value(rf_data->cable1_gpio);
	if (rf_data->cable0_val || rf_data->cable1_val) {
		the_rf_format->rf_cable_sts = RF_CABLE_IN;
	} else {
		the_rf_format->rf_cable_sts = RF_CABLE_OUT;
	}
	rf_data->cable0_val_pre = rf_data->cable0_val;
	rf_data->cable1_val_pre = rf_data->cable1_val;		
	rc = rf_cable_initial_request_irq(rf_data, CABLE_0);
	if (rc) {
		pr_err("could not request cable0_irq:%d\n", rf_data->cable0_irq);
        goto exit;
	}
	rc = rf_cable_initial_request_irq(rf_data, CABLE_1);
	if (rc) {
		pr_err("could not request cable1_irq:%d\n", rf_data->cable1_irq);
        goto exit;
	}
#if 0
	kobject_init(&rf_data->kobj, rfcable_kobj_type);
	kobject_add(&rf_data->kobj, &dev->kobj, "rfcable_sts");
	kobject_uevent(&rf_data->kobj, KOBJ_CHANGE);
	add_uevent_var(&rf_data->kobj, "rfcable_sts");
#endif
	//kobject_init_and_add(&rf_data->kobj, &rfcable_sts_ktype,
	//	NULL, "rfcable_sts");
	
	the_rf_data = rf_data;
	enable_irq(rf_data->cable0_irq);
	enable_irq(rf_data->cable1_irq);
	
    pr_err("%s: probe ok, SMEM_RF_INFO:%d, sts:%d, 0_val:%d, 1_val:%d, 0_irq:%d, 1_irq:%d\n",
		__func__, SMEM_DRAM_TYPE, the_rf_format->rf_cable_sts,
		rf_data->cable0_val, rf_data->cable1_val,
		rf_data->cable0_irq, rf_data->cable1_irq);
    return 0;

exit:
	wakeup_source_trash(&rf_data->cable_ws);
	kfree(rf_data);
exit_nomem:
	the_rf_data = NULL;
	the_rf_format = NULL;
    pr_err("%s: probe Fail!\n", __func__);

    return rc;
}

static const struct of_device_id rf_of_match[] = {
    { .compatible = "oppo,rf_cable", },
    {}
};
MODULE_DEVICE_TABLE(of, rf_of_match);

static struct platform_driver op_rf_cable_driver = {
    .driver = {
        .name       = "rf_cable",
        .owner      = THIS_MODULE,
        .of_match_table = rf_of_match,
    },
    .probe = op_rf_cable_probe,
};

static int __init op_rf_cable_init(void)
{
    int ret;
	
    ret = platform_driver_register(&op_rf_cable_driver);
    if (ret)
        pr_err("rf_cable_driver register failed: %d\n", ret);
	
    return ret;
}

MODULE_LICENSE("GPL v2");
//subsys_initcall(op_rf_cable_init);
late_initcall(op_rf_cable_init);
