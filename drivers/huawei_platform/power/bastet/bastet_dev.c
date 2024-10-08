/* bastet_dev.c
 *
 * Bastet Driver Device.
 *
 * Copyright (C) 2014 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <huawei_platform/power/bastet/bastet.h>
#include <huawei_platform/power/bastet/bastet_utils.h>

#define BASTET_NAME						"bastet"

#define BST_FIRST_MINOR					0
#define BST_DEVICES_NUMBER				1
/* Current Max Traffic report task number is 32 */
#define BST_TRAFFIC_LEN(len)			(len > 0XFF ? 0XFF : len)

dev_t bastet_dev;
struct cdev bastet_cdev;
struct class *bastet_class = NULL;

struct data_packet {
	struct list_head list;
	struct bst_device_ind data;
};

struct bastet_driver_data {
	wait_queue_head_t read_wait;
	spinlock_t read_lock;
	struct list_head read_queue;
};

static struct bastet_driver_data bastet_data;

extern bool bastet_dev_en;

extern void bastet_traffic_flow_init(void);
extern void bastet_utils_init(void);
extern void bastet_utils_exit(void);

#ifdef CONFIG_HUAWEI_BASTET_COMM
extern void bastet_comm_init(void);
extern int bastet_comm_write(u8 *msg, u32 len);
extern int get_modem_rab_id(struct bst_modem_rab_id *info);
#endif

extern int start_bastet_sock(struct bst_set_sock_sync_delay *init_prop);
extern int stop_bastet_sock(struct bst_sock_id *guide);
extern int get_tcp_sock_comm_prop(struct bst_get_sock_comm_prop *get_prop);
extern int set_tcp_sock_sync_prop(struct bst_set_sock_sync_prop *set_prop);
extern int set_tcp_sock_closed(struct bst_sock_comm_prop *guide);
extern int get_tcp_bastet_sock_state(struct bst_get_bastet_sock_state *get_prop);
extern int bind_local_ports(u16 *local_port);
extern int unbind_local_ports(u16 local_port);
extern int adjust_traffic_flow_by_pkg(uint8_t *buf, int cnt);
extern void bastet_wake_up_traffic_flow(void);
extern int set_current_net_device_name(char *iface);

/*
 * Indicate Message Api
 */
int post_indicate_packet(bst_ind_type type, void *info, unsigned int len)
{
	struct data_packet *pkt = NULL;

	if (!bastet_dev_en) {
		BASTET_LOGE("bastet is not opened");
		return -ENOENT;
	}

	pkt = kmalloc(sizeof(struct data_packet) + len, GFP_ATOMIC);
	if (NULL == pkt) {
		BASTET_LOGE("failed to kmalloc");
		return -ENOMEM;
	}
	memset(pkt, 0, sizeof(struct data_packet) + len);

    pkt->data.cons = 0;
	pkt->data.type = type;
	pkt->data.len = len;
	if (NULL != info) {
		memcpy(pkt->data.value, info, len);
	}

	spin_lock(&bastet_data.read_lock);
	list_add_tail(&pkt->list, &bastet_data.read_queue);
	spin_unlock(&bastet_data.read_lock);

	wake_up_interruptible_sync_poll(&bastet_data.read_wait,
								POLLIN | POLLRDNORM | POLLRDBAND);

	return 0;
}

/*
 * this is main method to exchange data with user space,
 * including socket sync, get ip and port, adjust kernel flow
 *
 * return "int" by standard.
 */
static long bastet_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	int rc = -EFAULT;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
		case BST_IOC_SOCK_SYNC_START: {
			struct bst_set_sock_sync_delay init_p;

			if (copy_from_user(&init_p, argp, sizeof(init_p)))
				break;

			rc = start_bastet_sock(&init_p);
			break;
		}
		case BST_IOC_SOCK_SYNC_STOP: {
			struct bst_sock_id guide;

			if (copy_from_user(&guide, argp, sizeof(guide)))
				break;

			rc = stop_bastet_sock(&guide);
			break;
		}
		case BST_IOC_SOCK_SYNC_SET: {
			struct bst_set_sock_sync_prop set_p;

			if (copy_from_user(&set_p, argp, sizeof(set_p)))
				break;

			rc = set_tcp_sock_sync_prop(&set_p);
			break;
		}
		case BST_IOC_SOCK_COMM_GET: {
			struct bst_get_sock_comm_prop get_p;

			if (copy_from_user(&get_p, argp, sizeof(get_p)))
				break;

			rc = get_tcp_sock_comm_prop(&get_p);
			if (rc < 0)
				break;

			if (copy_to_user(argp, &get_p, sizeof(get_p)))
				rc = -EFAULT;
			break;
		}
		case BST_IOC_SOCK_CLOSE_SET: {
			struct bst_sock_comm_prop guide;

			if (copy_from_user(&guide, argp, sizeof(guide)))
				break;

			rc = set_tcp_sock_closed(&guide);
			break;
		}
		case BST_IOC_SOCK_STATE_GET: {
			struct bst_get_bastet_sock_state get_p;

			if (copy_from_user(&get_p, argp, sizeof(get_p)))
				break;

			rc = get_tcp_bastet_sock_state(&get_p);
			if (rc < 0)
				break;

			if (copy_to_user(argp, &get_p, sizeof(get_p)))
				rc = -EFAULT;
			break;
		}
		case BST_IOC_APPLY_LOCAL_PORT: {
			u16 local_port;

			rc = bind_local_ports(&local_port);
			if (rc < 0)
				break;

			if (copy_to_user(argp, &local_port, sizeof(local_port)))
				rc = -EFAULT;
			break;
		}
		case BST_IOC_RELEASE_LOCAL_PORT: {
			u16 local_port;

			if (copy_from_user(&local_port, argp, sizeof(local_port)))
				break;

			rc = unbind_local_ports(local_port);
			break;
		}
		case BST_IOC_SET_TRAFFIC_FLOW: {
			uint8_t *buf = NULL;
			int buf_len = 0;
			struct bst_traffic_flow_pkg flow_p;

			if (copy_from_user(&flow_p, argp, sizeof(struct bst_traffic_flow_pkg))) {
				break;
			}

			if (0 == flow_p.cnt) {
				bastet_wake_up_traffic_flow();
				rc = 0;
				break;
			}
			buf_len = BST_TRAFFIC_LEN(flow_p.cnt);
			buf_len *= sizeof(struct bst_traffic_flow_prop);
			buf = (uint8_t *)kmalloc(buf_len, GFP_KERNEL);
			if (NULL == buf) {
				BASTET_LOGE("kmalloc failed");
				rc = -ENOMEM;
				break;
			}

			if (copy_from_user(buf, argp + sizeof(struct bst_traffic_flow_pkg), buf_len)) {
				BASTET_LOGE("pkg copy_from_user error");
				kfree(buf);
				break;
			}

			rc = adjust_traffic_flow_by_pkg(buf, flow_p.cnt);
			kfree(buf);
			break;
		}
		case BST_IOC_GET_TIMESTAMP_INFO: {
			struct bst_timestamp_info info;

			info.time_now = ((uint32_t)(jiffies));
			info.time_freq = HZ;

			rc = 0;
			if (copy_to_user(argp, &info, sizeof(info)))
				rc = -EFAULT;
			break;
		}
		case BST_IOC_SET_NET_DEV_NAME: {
			struct bst_net_dev_name info;

			if (copy_from_user(&info, argp, sizeof(info)))
				break;

			rc = set_current_net_device_name(info.iface);
			break;
		}
#ifdef CONFIG_HUAWEI_BASTET_COMM
		case BST_IOC_GET_MODEM_RAB_ID: {
			struct bst_modem_rab_id info;

			rc = get_modem_rab_id(&info);
			if (rc < 0)
				break;

			if (copy_to_user(argp, &info, sizeof(info)))
				rc = -EFAULT;
			break;
		}
#endif
		default: {
			BASTET_LOGE("unknown ioctl: %d", cmd);
			break;
		}
	}

	return rc;
}

/* support of 32bit userspace on 64bit platforms */
#ifdef CONFIG_COMPAT
static long compat_bastet_ioctl(struct file *flip, unsigned int cmd, unsigned long arg)
{
	return bastet_ioctl(flip, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static int bastet_open(struct inode *inode, struct file *filp)
{
	spin_lock(&bastet_data.read_lock);

	if (bastet_dev_en) {
		BASTET_LOGE("bastet device has been opened");
		spin_unlock(&bastet_data.read_lock);
		return -EPERM;
	}

	bastet_dev_en = true;

	spin_unlock(&bastet_data.read_lock);
	BASTET_LOGI("success");

	return 0;
}

static int bastet_packet_read(char __user *buf, size_t count)
{
	struct data_packet *pkt = NULL;
	uint8_t *data = NULL;
	bool isfree = false;
	int len = 0;
	int size = 0;

	if (NULL == buf) {
		return -EINVAL;
	}

	spin_lock(&bastet_data.read_lock);
	if (list_empty(&bastet_data.read_queue)) {
		spin_unlock(&bastet_data.read_lock);
		return -EAGAIN;
	}

	pkt = list_first_entry(&bastet_data.read_queue, struct data_packet, list);
	len = sizeof(struct bst_device_ind) - sizeof(pkt->data.cons) + pkt->data.len;
	data = (uint8_t *)(&pkt->data) + sizeof(pkt->data.cons);

	if ((0 == pkt->data.cons) && (count > len)) {
		list_del(&pkt->list);
		size = len;
		isfree = true;
	} else if (((0 == pkt->data.cons) && (count <= len))
		|| ((pkt->data.cons != 0) && (pkt->data.cons + count <= len))) {
		size = count;
		isfree = false;
	} else {
		list_del(&pkt->list);
		size = len - pkt->data.cons;
		isfree = true;
	}

	spin_unlock(&bastet_data.read_lock);
	if (copy_to_user(buf, data + pkt->data.cons, size)) {
		pkt->data.cons = 0;
		if (isfree) {
			kfree(pkt);
		}
		return -EFAULT;
	}
	pkt->data.cons += count;

	if (isfree) {
		kfree(pkt);
	}

	return size;
}

/*
 * blocked read, it will be waiting here until net device state is change.
 *
 * standard arg is "const char __user *buf".
 */
/*lint -e666*/
static ssize_t bastet_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	spin_lock(&bastet_data.read_lock);
	while(list_empty(&bastet_data.read_queue)) {
		spin_unlock(&bastet_data.read_lock);
		ret = wait_event_interruptible(bastet_data.read_wait, !list_empty(&bastet_data.read_queue));
		if (ret) {
			return ret;
		}
		spin_lock(&bastet_data.read_lock);
	}
	spin_unlock(&bastet_data.read_lock);

	return bastet_packet_read(buf, count);
}
/*lint +e666*/

#ifdef CONFIG_HUAWEI_BASTET_COMM
static ssize_t bastet_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	u8 msg[BST_MAX_WRITE_PAYLOAD];

	if (count > BST_MAX_WRITE_PAYLOAD) {
		BASTET_LOGE("write length over BST_MAX_WRITE_PAYLOAD!");
		return -EINVAL;
	}

	if (copy_from_user(msg, buf, count)) {
		BASTET_LOGE("copy_from_user error");
		return -EFAULT;
	}
	bastet_comm_write(msg, count);

	return count;
}
#endif

static unsigned int bastet_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &bastet_data.read_wait, wait);
	mask = !list_empty(&bastet_data.read_queue) ? (POLLIN | POLLRDNORM) : 0;

	return mask;
}

static int bastet_release(struct inode *inode, struct file *filp)
{
	struct list_head *p, *n;
	struct data_packet *pkt = NULL;
	
	spin_lock(&bastet_data.read_lock);

	if (list_empty(&bastet_data.read_queue)) {
		goto out_release;
	}

	list_for_each_safe(p, n, &bastet_data.read_queue) {
		pkt = list_entry(p, struct data_packet, list);
		list_del(&pkt->list);
		kfree(pkt);
	}

out_release:
	bastet_dev_en = false;
	spin_unlock(&bastet_data.read_lock);
	BASTET_LOGI("success");

	return 0;
}

static const struct file_operations bastet_dev_fops = {
	.owner = THIS_MODULE,
	.open = bastet_open,
	.unlocked_ioctl = bastet_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = compat_bastet_ioctl,
#endif
	.read = bastet_read,
#ifdef CONFIG_HUAWEI_BASTET_COMM
	.write = bastet_write,
#endif
	.poll = bastet_poll,
	.release = bastet_release,
};

static void bastet_data_init(void)
{
	spin_lock_init(&bastet_data.read_lock);
	INIT_LIST_HEAD(&bastet_data.read_queue);
	init_waitqueue_head(&bastet_data.read_wait);
}

static int bastet_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device* dev = NULL;

	bastet_data_init();
	bastet_utils_init();
	bastet_traffic_flow_init();
#ifdef CONFIG_HUAWEI_BASTET_COMM
	bastet_comm_init();
#endif

	/* register bastet major and minor number */
	ret = alloc_chrdev_region(&bastet_dev, BST_FIRST_MINOR, BST_DEVICES_NUMBER, BASTET_NAME);
	if (ret) {
		BASTET_LOGE("alloc_chrdev_region error");
		goto fail_region;
	}

	cdev_init(&bastet_cdev, &bastet_dev_fops);
	bastet_cdev.owner = THIS_MODULE;

	ret = cdev_add(&bastet_cdev, bastet_dev, BST_DEVICES_NUMBER);
	if (ret) {
		BASTET_LOGE("cdev_add error");
		goto fail_cdev_add;
	}

	bastet_class = class_create(THIS_MODULE, BASTET_NAME);
	if(IS_ERR(bastet_class)) {
		BASTET_LOGE("class_create error");
		goto fail_class_create;
	}

	dev = device_create(bastet_class, NULL, bastet_dev, NULL, BASTET_NAME);
	if (IS_ERR(dev)) {
		BASTET_LOGE("device_create error");
		goto fail_device_create;
	}

	return 0;

fail_device_create:
	class_destroy(bastet_class);
fail_class_create:
	cdev_del(&bastet_cdev);
fail_cdev_add:
	unregister_chrdev_region(bastet_dev, BST_DEVICES_NUMBER);
fail_region:
	return ret;
}

static int bastet_remove(struct platform_device *pdev)
{
	if (NULL != bastet_class) {
		device_destroy(bastet_class, bastet_dev);
		class_destroy(bastet_class);
	}
	cdev_del(&bastet_cdev);
	unregister_chrdev_region(bastet_dev, BST_DEVICES_NUMBER);
	bastet_utils_exit();

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id of_bastet_match_tbl[] = {
	{
		.compatible = "huawei,bastet",
	},
	{ /* end */ }
};

MODULE_DEVICE_TABLE(of, of_bastet_match_tbl);
#endif

static struct platform_driver bastet_driver = {
	.probe	= bastet_probe,
	.remove	= bastet_remove,
	.driver = {
		.name	= "bastet",
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(of_bastet_match_tbl),
#endif
	},
};

module_platform_driver(bastet_driver);

MODULE_AUTHOR("zhuxiaolong@huawei.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Bastet driver");
