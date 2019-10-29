#include <linux/module.h>	// needed by all modules
#include <linux/kernel.h>	// needed by kernel functions
#include <linux/init.h>		// needed by modules macros
#include <linux/uaccess.h>	// needed by user space copy functions
#include <linux/fs.h>		// needed by file operations
#include <linux/miscdevice.h>	// needed by misc device driver function

#define DRIVER_AUTHOR	"jisuAn"
#define DRIVER_DESC	"driver for 7-Segment"

#define SSEG_NAME		"7segment" // /dev/7segment123
#define SSEG_MODULES_VERSION	"7segment V1.0"
#define SSEG_ADDR		0x004

//gpio fpga interface provided
extern ssize_t iom_fpga_itf_read(unsigned int addr); // (extern)if symbol exists can use this function
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned short int value);

//global
static int sseg_in_use = 0;

int sseg_open(struct inode *pinode, struct file *pfile)
{
	if (sseg_in_use != 0){
		return -EBUSY;
	}

	sseg_in_use = 1;

	return 0;
}

int sseg_release(struct inode *pinode, struct file *pfile)
{
	sseg_in_use = 0;

	return 0;
}

ssize_t sseg_write(struct file *pinode, const char *gdata, size_t len, loff_t *off_what)
{
	unsigned char bytevalue[4]; // for user space data (1byte)
	unsigned short wordvalue; // for register data (2byte)
	const char *tmp = NULL;

	tmp = gdata; // gdata is userspace data

	if (copy_from_user(&bytevalue, tmp, 4))
	{
		return -EFAULT;
	}

	// type cast for 1byte to 2byte 
	wordvalue = (bytevalue[0] << 12) | (bytevalue[1] << 8) | 
			(bytevalue[2] << 4) | (bytevalue[3] << 0);
	
	//register write
	iom_fpga_itf_write((unsigned int) SSEG_ADDR, wordvalue);

	return len;
}


ssize_t sseg_read(struct file *pinode, char *gdata, size_t len, loff_t *off_what)
{
	unsigned char bytevalue[4];
	unsigned short wordvalue;
	char *tmp = NULL;

	tmp = gdata; // gdata is userspace data;

	wordvalue = iom_fpga_itf_read((unsigned int) SSEG_ADDR);
	bytevalue[0] = (wordvalue >> 12) & 0xF; // extract 1byte data 
	bytevalue[1] = (wordvalue >> 8) & 0xF; // extract 1byte data 
	bytevalue[2] = (wordvalue >> 4) & 0xF; // extract 1byte data 
	bytevalue[3] = (wordvalue >> 0) & 0xF; // extract 1byte data 

	if (copy_to_user(tmp, &bytevalue, 1)){
		return -EFAULT;
	}

	return len;
}

static struct file_operations sseg_fops = {
	.owner	= THIS_MODULE,
	.open	= sseg_open,
	.read	= sseg_read,
	.write	= sseg_write,
	.release= sseg_release,
};

static struct miscdevice sseg_driver = {
	.fops	= &sseg_fops,
	.name	= SSEG_NAME,
	.minor	= MISC_DYNAMIC_MINOR,
};

static int hello_init(void)
{
	misc_register(&sseg_driver);
	printk("init module hello\n");
	return 0;
}

static void hello_exit(void)
{
	misc_deregister(&sseg_driver);
	printk("exit module hello\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual BSD/GPL");
