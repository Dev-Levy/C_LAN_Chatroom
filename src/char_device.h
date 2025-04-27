 #ifndef CHAR_DEVICE_H
 #define CHAR_DEVICE_H
 
 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/fs.h>
 #include <linux/cdev.h>
 #include <linux/device.h>
 #include <linux/uaccess.h>
 #include <linux/mutex.h>
 #include <linux/slab.h>

#include "model.h"
 
#define DEVICE_NAME "chatdb"
#define CLASS_NAME "chat"

 static int dev_open(struct inode*, struct file*);
 static int dev_release(struct inode*, struct file*);
 static ssize_t dev_read(struct file*, char*, size_t, loff_t*);
 static ssize_t dev_write(struct file*, const char*, size_t, loff_t*);
 static loff_t dev_llseek(struct file *filep, loff_t offset, int whence);


static int majorNumber;
static struct class* chatClass = NULL;
static struct device* chatDevice = NULL;
static struct cdev chatCdev;
static ChatDatabase* chatDB = NULL;

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
    .llseek = dev_llseek
};
 
 #endif /* CHAR_DEVICE_H */