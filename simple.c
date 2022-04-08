#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/io.h>

#define KBD_IRQ             1       /* IRQ number for keyboard (i8042) */
#define KBD_DATA_REG        0x60    /* I/O port for keyboard data */
#define KBD_SCANCODE_MASK   0x7f
#define KBD_STATUS_MASK     0x80
#define BUFFER_SIZE 50

static dev_t dev = 0;
static struct class *dev_class;
static struct cdev my_device;
static char buffer[BUFFER_SIZE];
static int head = 0, tail = 0;

static irqreturn_t kbd_handler(int irq, void *dev_id)
{
  char scancode;

  scancode = inb(KBD_DATA_REG);
  /* NOTE: i/o ops take a lot of time thus must be avoided in HW ISRs */
  printk(KERN_INFO "Scan Code %x %s\n",
	 scancode & KBD_SCANCODE_MASK,
	 scancode & KBD_STATUS_MASK ? "Released" : "Pressed");
            
  // TODO - SAVE THE SCANCODE INSIDE A BUFFER
  if (head + 1 != tail) {
    buffer[head] = scancode;
    head = (head + 1) % BUFFER_SIZE;
  }   
  return IRQ_HANDLED;
}


/**
 * @brief Read data out of the buffer
 */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {

  // TODO - COPY THE CONTENT OF THE BUFFER INSIDE user_buffer, CAREFUl TO CHECK count
  size_t num_count = 0;
  while (num_count < count && (head != tail)) {
    user_buffer[num_count] = buffer[tail];
    tail = (tail + 1) & BUFFER_SIZE;
  }
  // RETURN HOW MUCH DATA COPIED.
  return num_count;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_open(struct inode *device_file, struct file *instance) {
  printk("dev_nr - open was called!\n");
  return 0;
}

/**
 * @brief This function is called, when the device file is opened
 */
static int driver_close(struct inode *device_file, struct file *instance) {
  printk("dev_nr - close was called!\n");
  return 0;
}





static struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = driver_open,
  .release = driver_close,
  .read = driver_read
};




static int __init kbd_init(void)
{
  int result = alloc_chrdev_region(&dev, 0, 1, "kl");
  if(result < 0) {
    printk(KERN_ERR "failed to alloc chrdev region\n");
    return -1;
  }
    
  /*Creating struct class*/
  if((dev_class = class_create(THIS_MODULE,"kl_class")) == NULL){
    printk(KERN_ERR "Cannot create the struct class for device\n");
    return -1;
  }
 
  /*Creating device*/
  if((device_create(dev_class,NULL,dev,NULL,"kl_device")) == NULL){
    printk(KERN_ERR "Cannot create the Device\n");
    return -1;
  }
       
        
  /* Initialize device file */
  cdev_init(&my_device, &fops);

  /* Regisering device to kernel */
  if(cdev_add(&my_device, dev, 1) == -1) {
    printk(KERN_ERR "Registering of device to kernel failed!\n");
    return -1;
  }
  printk(KERN_INFO "Kernel Module Inserted Successfully...\n");
    
  return request_irq(KBD_IRQ, kbd_handler, IRQF_SHARED, "kbd", (void *) kbd_handler);
}

static void __exit kbd_exit(void)
{
  free_irq(KBD_IRQ, (void *)kbd_handler);
  cdev_del(&my_device);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  unregister_chrdev_region(dev, 1);
  printk(KERN_INFO "Kernel Module Finished Successfully...\n");
}

module_init(kbd_init);
module_exit(kbd_exit);

MODULE_LICENSE("GPL");
