
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <linux/miscdevice.h>	// needed by misc device driver function
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#define MAX_BUTTON 9

#define PUSH_BUTTON_MAJOR 265			
#define PUSH_BUTTON_NAME "push_button"

#define PUSH_BUTTON_ADDRESS 0x050			// pysical address
extern ssize_t iom_fpga_itf_read(unsigned int addr);
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned char value);

static int push_button_port_usage = 0;
static unsigned char *push_button_addr;
static unsigned char *iom_demo_addr;

ssize_t push_button_read(struct file *inode, char *gdata, size_t length, loff_t *off_what); 
int push_button_open(struct inode *minode, struct file *mfile);
int push_button_release(struct inode *minode, struct file *mfile);
int push_button_open(struct inode *minode, struct file *mfile) 
{	
	if(push_button_port_usage != 0) return -EBUSY;

	push_button_port_usage = 1;


	return 0;
}

int push_button_release(struct inode *minode, struct file *mfile) 
{
	push_button_port_usage = 0;

	return 0;
}



ssize_t push_button_read(struct file *inode, char *gdata, size_t length, loff_t *off_what) 
{
	int i;
	unsigned char push_sw_value[MAX_BUTTON];	
    unsigned short int _s_value;

	for(i=0;i<length;i++) 
    {
		_s_value = iom_fpga_itf_read((unsigned int)PUSH_BUTTON_ADDRESS+i*2);
        	push_sw_value[i] = _s_value &0xFF;
    }

	if (copy_to_user(gdata, &push_sw_value, length))
		return -EFAULT;

	return length;	
}

struct file_operations push_button_fops =
{
	owner:		THIS_MODULE,
	open:		push_button_open,
	read:		push_button_read,	
	release:	push_button_release,
};

static struct miscdevice push_switch_driver = {
	.fops	= &push_button_fops,
	.name	= PUSH_BUTTON_NAME,
	.minor	= MISC_DYNAMIC_MINOR,
};


int __init push_button_init(void)
{
	misc_register(&push_switch_driver);
	return 0;
}


void __exit push_button_exit(void) 
{
	unregister_chrdev(PUSH_BUTTON_MAJOR, PUSH_BUTTON_NAME);
}

module_init(push_button_init);
module_exit(push_button_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
