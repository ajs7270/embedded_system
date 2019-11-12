#include <linux/module.h>	// needed by all modules
#include <linux/kernel.h>	// needed by kernel functions
#include <linux/init.h>		// needed by modules macros
#include <linux/uaccess.h>	// needed by user space copy functions
#include <linux/fs.h>		// needed by file operations
#include <linux/miscdevice.h>	// needed by misc device driver function
#include <asm/ioctl.h>		// needed by ioctl functions and macros

#define DRIVER_AUTHOR	"jisuAn"
#define DRIVER_DESC	"driver for lcd"

#define LCD_NAME		"lcd" // /dev/lcd123
#define LCD_MODULES_VERSION	"lcd V1.0"
#define LCD_ADDR		0x090
#define LCD_NUM_CHARS		32

#define LCD_MAGIC		0xBD
#define LCD_SET_CURSOR_POS		_IOW(LCD_MAGIC, 0, int)


//gpio fpga interface provided
extern ssize_t iom_fpga_itf_read(unsigned int addr); // (extern)if symbol exists can use this function
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned short int value);

//global
static int lcd_in_use = 0;
static int lcd_cursor_pos = 0;

int lcd_open(struct inode *pinode, struct file *pfile)
{
	if (lcd_in_use != 0){
		return -EBUSY;
	}

	lcd_in_use = 1;

	return 0;
}

int lcd_release(struct inode *pinode, struct file *pfile)
{
	lcd_in_use = 0;

	return 0;
}

void __lcd_write(unsigned short wordvalue, int pos)
{
	if (pos >= 0 && pos < LCD_NUM_CHARS) {
		iom_fpga_itf_write((unsigned int) LCD_ADDR + pos, wordvalue);
	}
}

ssize_t lcd_write(struct file *pinode, const char *gdata, size_t len, loff_t *off_what)
{
	int ret, i;
	unsigned char buf[LCD_NUM_CHARS]; // for user space data 
	unsigned short wordvalue; // for register data (2byte = 16bit)
	const char *tmp = NULL;

	tmp = gdata; // gdata is userspace data

	if(len > LCD_NUM_CHARS) {
		printk(KERN_WARNING "only %d byte data will be processed\n",
				LCD_NUM_CHARS);
		len = LCD_NUM_CHARS;
	}

	memset(buf, ' ', LCD_NUM_CHARS);
	ret = copy_from_user(buf, tmp, LCD_NUM_CHARS);// userspace data to kernel space

	if (ret) {
		return -EFAULT;
	}

	for (i=0; i< len; i+=2){
		wordvalue = ((unsigned short)buf[i]) << 8 & 0xFF00;
		wordvalue = wordvalue | ((unsigned short)buf[i+1]) & 0x00FF;

		__lcd_write(wordvalue, lcd_cursor_pos);

		lcd_cursor_pos += 2;
		lcd_cursor_pos = lcd_cursor_pos % LCD_NUM_CHARS;
	}

	return len;
}

static long lcd_ioctl(struct file *pinode, unsigned int cmd, unsigned long data)
{
	int ret, param;

	switch (cmd){
	case LCD_SET_CURSOR_POS:
		ret = copy_from_user(&param, (void*) data, _IOC_SIZE(LCD_SET_CURSOR_POS));
		if(ret< 0){
			printk(KERN_ERR "data copy form userspace failed\n");
			return -EFAULT;
		}

		if(param < 0 && param >= LCD_NUM_CHARS) {
			printk(KERN_ERR "invalid cursor position \n");
			return -EFAULT;
		}

		lcd_cursor_pos = (param >> 1) << 1;
		printk(KERN_DEBUG "param: %d, pos: %d\n",param, lcd_cursor_pos);
		break;
	}

	return 0;
}


static struct file_operations lcd_fops = {
	.owner	= THIS_MODULE,
	.open	= lcd_open,
	.write	= lcd_write,
	.unlocked_ioctl	= lcd_ioctl,
	.release= lcd_release,
};

static struct miscdevice lcd_driver = {
	.fops	= &lcd_fops,
	.name	= LCD_NAME,
	.minor	= MISC_DYNAMIC_MINOR,
};

static int hello_init(void)
{
	misc_register(&lcd_driver);
	printk("init module hello\n");
	return 0;
}

static void hello_exit(void)
{
	misc_deregister(&lcd_driver);
	printk("exit module hello\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual BSD/GPL");
