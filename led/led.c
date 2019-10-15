#include <linux/module.h>	// needed by all modules
#include <linux/kernel.h>	// needed by kernel functions
#include <linux/init.h>		// needed by modules macros
#include <linux/uaccess.h>	// needed by user space copy functions
#include <linux/fs.h>		// needed by file operations
#include <linux/miscdevice.h>	// needed by misc device driver function

#define DRIVER_AUTHOR	"jisuAn"
#define DRIVER_DESC	"driver for 8-LEDs FPGA"

#define LED_NAME		"led" // /dev/led123
#define LED_MODULES_VERSION	"LED V1.0"
#define LED_ADDR		0x016

//gpio fpga interface provided
extern ssize_t iom_fpga_itf_read(unsigned int addr); // (extern)if symbol exists can use this function
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned short int value);

//global
static int led_in_use = 0;

int led_open(struct inode *pinode, struct file *pfile)
{
	if (led_in_use != 0){
		return -EBUSY;
	}

	led_in_use = 1;

	return 0;
}

int led_release(struct inode *pinode, struct file *pfile)
{
	led_in_use = 0;

	return 0;
}

ssize_t led_write(struct file *pinode, const char *gdata, size_t len, loff_t *off_what)
{
	unsigned char bytevalue; // for user space data (1byte)
	unsigned short wordvalue; // for register data (2byte)
	const char *tmp = NULL;

	tmp = gdata; // gdata is userspace data

	if (copy_from_user(&bytevalue, tmp, 1))
	{
		return -EFAULT;
	}

	wordvalue = (unsigned short) bytevalue; // type cast for 1byte to 2byte
	iom_fpga_itf_write((unsigned int) LED_ADDR, wordvalue); // register write

	return len;
}


ssize_t led_read(struct file *pinode, char *gdata, size_t len, loff_t *off_what)
{
	unsigned char bytevalue;
	unsigned short wordvalue;
	char *tmp = NULL;

	tmp = gdata; // gdata is userspace data;

	wordvalue = iom_fpga_itf_read((unsigned int) LED_ADDR);
	bytevalue = (unsigned char) wordvalue & 0x00FF; // extract 1byte data 

	if (copy_to_user(tmp, &bytevalue, 1)){
		return -EFAULT;
	}

	return len;
}

static struct file_operations led_fops = {
	.owner	= THIS_MODULE,
	.open	= led_open,
	.read	= led_read,
	.write	= led_write,
	.release= led_release,
};

static struct miscdevice led_driver = {
	.fops	= &led_fops,
	.name	= LED_NAME,
	.minor	= MISC_DYNAMIC_MINOR,
};

static int hello_init(void)
{
	misc_register(&led_driver);
	printk("init module hello\n");
	return 0;
}

static void hello_exit(void)
{
	misc_deregister(&led_driver);
	printk("exit module hello\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual BSD/GPL");
