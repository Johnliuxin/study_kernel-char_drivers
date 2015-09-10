#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>        /* Get Major and Minor Numbers */
#include <linux/cdev.h>      /* Register char drivers */
#include <linux/uaccess.h>   /* copy_to/from_user*/
#include <linux/string.h>    /* strlen */
#include <linux/kernel.h>    /* min/max */

/* 
 * Another special macro (MODULE_LICENSE) is used to tell the kernel that this 
 * module bears a free license; without such a declaration, the kernel 
 * complains when the module is loaded.
 */
MODULE_LICENSE("Dual BSD/GPL");

static char char_drivers_value[5] = {'\0'};
static struct cdev *embest_cdev;

static int fops_open(struct inode *inode, struct file *filp)
{
	/* Check for device-specific errors (such as device-not-ready or similar hardware problems) */
	//Nothing needed
	
	/* Initialize the device if it is being opened for the first time */
	//Nothing needed
	
	/* Allocate and fill any data structure to be put in filp->private_data */
	//Nothing needed now
	
	printk(KERN_ALERT "CharDriver: fops_open\n");
	
	return 0;
}

static int fops_release(struct inode *inode, struct file *filp)
{
	/* Deallocate anything that open allocated in filp->private_data */
	//Nothing needed now
	
	/* Shut down the device on last close */
	//Nothing needed
	
	printk(KERN_ALERT "CharDriver: fops_release\n");
	
	return 0;
}

static ssize_t fops_read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
	size_t len = strlen(char_drivers_value);
	len = min(len, count);
	
	printk(KERN_ALERT "CharDriver: fops_read count is %ld, len %ld\n", count, len);
	
	if (len == 0) {
		printk(KERN_ALERT "CharDriver: fops_read no more data\n");
		goto Done;
	}
	
	if (copy_to_user(buff, &char_drivers_value, len)) {
		len = -EFAULT;
		printk(KERN_ERR "CharDriver: fops_read copy_to_user error\n");
	} else {
		/* when read out all the data from char_drivers_value, we need clear char_drivers_value.
		 * so next time read from this device will get no-more-data
		 * otherwise, the "cat" cmd could not work well, it will get data endless
		 */
		char_drivers_value[0] = '\0';
		*offp += len;
	}

Done:
	return len;
}

static ssize_t fops_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
	int len = count;
	printk(KERN_ALERT "CharDriver: fops_write count is %ld\n", count);
	
	if (copy_from_user(&char_drivers_value, buff, len)) {
		len = -EFAULT;
		printk(KERN_ERR "CharDriver: fops_write copy_from_user error\n");
	} else {
		*offp += len;
	}
	
	char_drivers_value[len] = '\0';
	
	return len;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = fops_open,
	.release = fops_release,
	.read = fops_read,
	.write = fops_write,
};

static int __init hello_init(void)
{
	int ret;
	dev_t dev_id;
	
	/*1th: alloc the Major/Minor number*/
	ret = alloc_chrdev_region(&dev_id, 0, 1, "embest_char_driver");
	if (ret < 0) {
		printk(KERN_ERR "alloc chrdev region error %d\n", ret);
		goto alloc_chrdev_region_err;
	}
	
	/*2th: alloc the "struct cdev"*/
	embest_cdev = cdev_alloc();
	if (!embest_cdev) {
		ret = -ENOMEM;
		printk(KERN_ERR "cdev_alloc error %d\n", ret);
		goto cdev_alloc_err;
	}
	
	/*3th: initial cdev->ops pointer*/
	embest_cdev->ops = &fops;
	
	/*4th: add the cdev to the system*/
	ret = cdev_add(embest_cdev, dev_id, 1);
	if (ret) {
		printk(KERN_ERR "cdev_add error %d\n", ret);
		goto cdev_add_err;
	}
	
	/*If all above successful, you can get Major num from /proc/devices, then use mknod to create device node*/
	printk(KERN_ALERT "register char driver successful\n");
	
	return ret;

cdev_add_err:
	cdev_del(embest_cdev);
cdev_alloc_err:
	unregister_chrdev_region(dev_id, 1);
alloc_chrdev_region_err:
	return ret;
}

static void hello_exit(void)
{
	cdev_del(embest_cdev);
	unregister_chrdev_region(embest_cdev->dev, 1);
	
	printk(KERN_ALERT "unregister char driver successful\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("John LiuXin");
MODULE_DESCRIPTION("Example of char driver");