#include <linux/module.h>       /* Needed by all LKMs */
#include <linux/kernel.h>       /* Needed for KERN_ALERT */
#include <linux/init.h>         /* Needed for the macros */
#include <linux/fs.h>           /* for struct file_operations */
#include <asm/uaccess.h>        /* for put_user */

/* Documentation macros.  These are not used by the kernel, but it
 * may complain if they are not provided. */

MODULE_AUTHOR("Nicholas R. Howe");
MODULE_DESCRIPTION("Simple Kernel Module");
MODULE_LICENSE("GPL");

/*  
 *  Prototypes - this would normally go in a .h file
 */
static int __init buffer_init_module(void);
static void __exit buffer_exit_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "buffer"    /* Dev name as it appears in /proc/devices   */
#define BUF_LEN 50              /* Max length of the message from the device */

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;               /* Major number assigned to our device driver */
static int Device_Open = 0;     /* Is device open?  
                                 * Used to prevent multiple access to device */
static char Buf[BUF_LEN+1]      /* The buffer to store last message */
= "The buffer has not been filled yet.  Hello World!\n";
static char *Buf_Ptr;           /* A pointer to the buffer */
static int Buf_Char = 50;       /* The number of characters stored */
static int Bytes_Read = 0;      /* Number of bytes read since open */

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

/*
 * Functions
 */

int buffer_init_module(void)
{
  Major = register_chrdev(0, DEVICE_NAME, &fops);

  if (Major < 0) {
    printk("Registering the character device failed with %d\n",
	   Major);
    return Major;
  }

  printk("<1>I was assigned major number %d.  To talk to\n", Major);
  printk("<1>the driver, create a dev file with\n");
  printk("'sudo mknod /dev/buffer c %d 0'.\n", Major);
  printk("<1>Try various minor numbers.  Try to cat and echo to\n");
  printk("the device file.\n");
  printk("<1>Remove the device file and module when done.\n");

  return 0;
}

void buffer_exit_module(void)
{
  /* 
   * Unregister the device 
   */
  int ret = unregister_chrdev(Major, DEVICE_NAME);
  if (ret < 0)
    printk("Error in unregister_chrdev: %d\n", ret);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
  if (Device_Open)
    return -EBUSY;
  Device_Open++;
  Buf_Ptr = Buf;
  try_module_get(THIS_MODULE);
  Bytes_Read = 0;

  return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
  Device_Open--;          /* We're now ready for our next caller */

  /* 
   * Decrement the usage count, or else once you opened the file, you'll
   * never get get rid of the module. 
   */
  module_put(THIS_MODULE);

  return 0;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
  int already_read = Bytes_Read;  /* keep track of how many read already */

  /*
   * If we're at the end of the message, 
   * return 0 signifying end of file 
   */
  if (Bytes_Read >= Buf_Char)
    return 0;

  /* 
   * Actually put the data into the buffer 
   */
  while (length && (Bytes_Read < Buf_Char)) {

    /* 
     * The buffer is in the user data segment, not the kernel 
     * segment so "*" assignment won't work.  We have to use 
     * put_user which copies data from the kernel data segment to
     * the user data segment. 
     */
    put_user(Buf_Ptr[Bytes_Read], buffer+Bytes_Read);

    length--;
    Bytes_Read++;
  }

  /* 
   * Most read functions return the number of bytes put into the buffer
   */
  return Bytes_Read-already_read;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *buffer, size_t length, loff_t * off)
{
  /*
   * Number of bytes actually written to the buffer 
   */
  Buf_Char = 0;

  /*
   * If we're at the end of the message, 
   * return 0 signifying end of file 
   */
  if (Buf_Char >= BUF_LEN) {
    return 0;
  }

  /* 
   * Actually put the data into the buffer 
   */
  while (length && (Buf_Char < BUF_LEN)) {

    /* 
     * The buffer is in the user data segment, not the kernel 
     * segment so "*" assignment won't work.  We have to use 
     * put_user which copies data from the kernel data segment to
     * the user data segment. 
     */
    get_user(Buf_Ptr[Buf_Char],buffer+Buf_Char);

    length--;
    Buf_Char++;
  }

  /* 
   * Most write functions return the number of bytes put into the buffer
   */
  return Buf_Char;
}

/* The macros below register the init and exit functions with the kernel */
module_init(buffer_init_module);
module_exit(buffer_exit_module);
