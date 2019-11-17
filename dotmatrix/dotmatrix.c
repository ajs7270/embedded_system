#include <linux/module.h>	// needed by all modules
#include <linux/kernel.h>	// needed by kernel functions
#include <linux/init.h>		// needed by modules macros
#include <linux/uaccess.h>	// needed by user space copy functions
#include <linux/fs.h>		// needed by file operations
#include <linux/miscdevice.h>	// needed by misc device driver function
#include <asm/ioctl.h>	// needed by ioctl functions and macros

#define DRIVER_AUTHOR	"jisuAn"
#define DRIVER_DESC	"driver for dotmatrix"

#define DOTM_NAME		"dotmatrix" // /dev/7segment123
#define DOTM_MODULES_VERSION	"dotmatrix V1.0"
#define DOTM_ADDR		0x210

#define DOTM_MAGIC		0xBC
#define DOTM_SET_ALL		_IOW(DOTM_MAGIC, 0, int)
#define DOTM_SET_CLEAR		_IOW(DOTM_MAGIC, 1, int)
#define DOTM_RIGHT_SHIFT	_IOW(DOTM_MAGIC, 2, int)
#define DOTM_LEFT_SHIFT		_IOW(DOTM_MAGIC, 3, int)

#define DOT_WIDTH		8
unsigned char buffer[100] = {0, };
unsigned char current_word[10] = {0, };
int current_state = 0; // 8 => 1 word
int word_len = 0;


//gpio fpga interface provided
extern ssize_t iom_fpga_itf_read(unsigned int addr); // (extern)if symbol exists can use this function
extern ssize_t iom_fpga_itf_write(unsigned int addr, unsigned short int value);

//global
static int dotm_in_use = 0;

// dotmatrix fonts
unsigned char dotm_fontmap_decimal[10][10] = {
        {0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
        {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
        {0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
        {0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
        {0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
        {0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
        {0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
        {0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
        {0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
        {0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
};

unsigned char dotm_fontmap_full[10] = {
        0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

unsigned char dotm_fontmap_empty[10] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

int dotm_open(struct inode *pinode, struct file *pfile)
{
	if (dotm_in_use != 0){
		return -EBUSY;
	}

	dotm_in_use = 1;

	return 0;
}

int dotm_release(struct inode *pinode, struct file *pfile)
{
	dotm_in_use = 0;

	return 0;
}

ssize_t dotm_write(struct file *pinode, const char *gdata, size_t len, loff_t *off_what)
{
	int ret, i;
	//unsigned char num; // for user space data (1byte)
	unsigned short wordvalue; // for register data (2byte = 16bit)
	const char *tmp = NULL;

	tmp = gdata; // gdata is userspace data

	/*
	if(len > 1) {
		printk(KERN_WARNING "only 1 byte data will be processed\n");
		len = 1;
	}
	*/

	ret = copy_from_user(buffer, tmp, len);// userspace data to kernel space

	word_len = len;

	if (ret) {
		return -EFAULT;
	}

	for (i=0; i< 10; i++){
		wordvalue = dotm_fontmap_decimal[buffer[0]][i] & 0x7F;
		current_word[i] = dotm_fontmap_decimal[buffer[0]][i] & 0x7F;
		iom_fpga_itf_write((unsigned int) DOTM_ADDR+(i*2), wordvalue);
	}

	return len;
}

static long dotm_ioctl(struct file *pinode, unsigned int cmd, unsigned long data)
{
	int i;
	unsigned short wordvalue;

	switch (cmd){
	case DOTM_SET_ALL:
		for (i=0; i<10;i++){
			wordvalue = dotm_fontmap_full[i] & 0x7F;
			iom_fpga_itf_write((unsigned int) DOTM_ADDR+(i*2), wordvalue);
		}
		break;

	case DOTM_SET_CLEAR:
		for (i=0; i<10;i++){
			wordvalue = dotm_fontmap_empty[i] & 0x7F;
			iom_fpga_itf_write((unsigned int) DOTM_ADDR+(i*2), wordvalue);
		}
		break;
	
	case DOTM_RIGHT_SHIFT:
		current_state--;
		unsigned char adj_word_buffer[10];
		int adj_index = ((current_state/DOT_WIDTH)-1) % word_len;
		for(i=0; i< 10; i++){
			adj_word_buffer[i] = (dotm_fontmap_decimal[buffer[adj_index][i]]
					<< (current_state % DOT_WIDTH)) & 0x7F;
		}

		for (i=0; i<10;i++){
			wordvalue = (current_word[i] >> 1) & 0x7F;
			wordvalue = (current_word[i] | adj_word_buffer[i]) & 0x7F;
			current_word[i] = wordvalue;
			iom_fpga_itf_write((unsigned int) DOTM_ADDR+(i*2), wordvalue);
		}
		break;

	case DOTM_LEFT_SHIFT:
		unsigned char adj_word_buffer[10];
		int adj_index = ((current_state/DOT_WIDTH)) % word_len;
		for(i=0; i< 10; i++){
			adj_word_buffer[i] = (dotm_fontmap_decimal[buffer[adj_index][i]]
					<< (current_state % DOT_WIDTH)) & 0x7F;
		}

		for (i=0; i<10;i++){
			wordvalue = (current_word[i] << 1) & 0x7F;
			wordvalue = (current_word[i] | adj_word_buffer[i]) & 0x7F;
			current_word[i] = wordvalue;
			iom_fpga_itf_write((unsigned int) DOTM_ADDR+(i*2), wordvalue);
		}
		break;

	}

	return 0;
}

static struct file_operations dotm_fops = {
	.owner	= THIS_MODULE,
	.open	= dotm_open,
	.write	= dotm_write,
	.unlocked_ioctl	= dotm_ioctl,
	.release= dotm_release,
};

static struct miscdevice dotm_driver = {
	.fops	= &dotm_fops,
	.name	= DOTM_NAME,
	.minor	= MISC_DYNAMIC_MINOR,
};

static int hello_init(void)
{
	misc_register(&dotm_driver);
	printk("init module hello\n");
	return 0;
}

static void hello_exit(void)
{
	misc_deregister(&dotm_driver);
	printk("exit module hello\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual BSD/GPL");
