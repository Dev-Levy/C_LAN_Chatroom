// chat_device.c - Kernel module implementing a char device for chat messages
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define DEVICE_NAME "chatdb"
#define CLASS_NAME "chat"
#define MAX_MESSAGES 100
#define MAX_MSG_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A char device driver for chat application database");
MODULE_VERSION("0.1");

// Chat message structure
typedef struct {
    char content[MAX_MSG_SIZE];
    char sender[32];
    char timestamp[20];
} ChatMessage;

// Chat database structure
typedef struct {
    ChatMessage messages[MAX_MESSAGES];
    int msg_count;
    struct mutex lock;
} ChatDatabase;

static int majorNumber;
static struct class* chatClass = NULL;
static struct device* chatDevice = NULL;
static struct cdev chatCdev;
static ChatDatabase* chatDB = NULL;

// Function prototypes
static int dev_open(struct inode*, struct file*);
static int dev_release(struct inode*, struct file*);
static ssize_t dev_read(struct file*, char*, size_t, loff_t*);
static ssize_t dev_write(struct file*, const char*, size_t, loff_t*);

// File operations structure, we nust declare which operations should the driver implement, if we choose to not to implement some feature 
// it fills with NULLS those functions
static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

// Initialize the device
static int __init chat_device_init(void) {
    printk(KERN_INFO "ChatDB: Initializing the Chat Database device\n");

    // Allocate a major number dynamically
    // So basically this registers and allocates a major number for the device
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "ChatDB failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "ChatDB: registered with major number %d\n", majorNumber);

    // Register the device class Dont know what this do :(
    chatClass = class_create(DEVICE_NAME);
    if (IS_ERR(chatClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(chatClass);
    }
    printk(KERN_INFO "ChatDB: device class registered\n");

    // Create the device driver 
    //Basically this does the whole sudo mknod command creates a devices file and link the major number with it
    chatDevice = device_create(chatClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(chatDevice)) {
        class_destroy(chatClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(chatDevice);
    }
    printk(KERN_INFO "ChatDB: device created\n");

    // Initialize cdev
    //void cdev_init(struct cdev *cdev, const struct file_operations *fops);

    cdev_init(&chatCdev, &fops);
    chatCdev.owner = THIS_MODULE;
    //int cdev_add(struct cdev *p, dev_t dev, unsigned count);
    if (cdev_add(&chatCdev, MKDEV(majorNumber, 0), 1) < 0) {
        device_destroy(chatClass, MKDEV(majorNumber, 0));
        class_destroy(chatClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to add cdev\n");
        return -1;
    }

    // Allocate memory for the database
    chatDB = kmalloc(sizeof(ChatDatabase), GFP_KERNEL);
    if (!chatDB) {
        cdev_del(&chatCdev);
        device_destroy(chatClass, MKDEV(majorNumber, 0));
        class_destroy(chatClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to allocate memory for chat database\n");
        return -ENOMEM;
    }

    // Initialize database
    chatDB->msg_count = 0;
    mutex_init(&chatDB->lock);

    printk(KERN_INFO "ChatDB: device initialization complete\n");
    return 0;
}

// Cleanup the device
static void __exit chat_device_exit(void) {
    // Free the database memory
    if (chatDB) {
        kfree(chatDB);
    }

    // Remove the cdev
    cdev_del(&chatCdev);
    
    // Destroy the device
    device_destroy(chatClass, MKDEV(majorNumber, 0));
    
    // Destroy the class
    class_destroy(chatClass);
    
    // Unregister the major number
    unregister_chrdev(majorNumber, DEVICE_NAME);
    
    printk(KERN_INFO "ChatDB: device unregistered\n");
}

// Device open function
static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "ChatDB: Device opened\n");
    return 0;
}

// Device release function
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "ChatDB: Device closed\n");
    return 0;
}

// Device read function - returns chat messages to user
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    int error_count = 0;
    int bytes_to_copy = 0;
    char temp_buffer[MAX_MSG_SIZE + 64];
    int i;
    if (mutex_lock_interruptible(&chatDB->lock)) {
        return -ERESTARTSYS;
    }
    
    // Check if we've already sent all messages
    if (*offset >= chatDB->msg_count) {
        mutex_unlock(&chatDB->lock);
        return 0;
    }
    
    // Get the message at current offset
    i = *offset;
    snprintf(temp_buffer, MAX_MSG_SIZE + 64, "[%s] %s: %s\n", 
             chatDB->messages[i].timestamp, 
             chatDB->messages[i].sender, 
             chatDB->messages[i].content);
    
    bytes_to_copy = strlen(temp_buffer);
    if (bytes_to_copy > len) bytes_to_copy = len;
    
    // Copy to user space
    error_count = copy_to_user(buffer, temp_buffer, bytes_to_copy);
    if (error_count) {
        mutex_unlock(&chatDB->lock);
        printk(KERN_INFO "ChatDB: Failed to send %d characters to user\n", error_count);
        return -EFAULT;
    }
    //asd
    *offset += 1;  // Move to next message
    mutex_unlock(&chatDB->lock);
    printk(KERN_INFO "Hello, you just read something\n");
    return bytes_to_copy;

    //POTENTIAL MAJOR ISSUE: During my testing if you send a message it stores the data and you can read it, however if you want to read it again the whole message back log empties
}

// Device write function - stores new message
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    char temp_buffer[MAX_MSG_SIZE + 64] = {0};
    char msg[MAX_MSG_SIZE] = {0};
    char sender[32] = {0};
    char timestamp[20] = {0};
    int scanned;
    
    // Limit message size
    if (len > MAX_MSG_SIZE + 64) {
        len = MAX_MSG_SIZE + 64;
    }
    
    if (copy_from_user(temp_buffer, buffer, len)) {
        printk(KERN_INFO "ChatDB: Failed to receive message from user\n");
        return -EFAULT;
    }
    
    // Parse input format: "sender:timestamp:message"
    scanned = sscanf(temp_buffer, "%31[^:]:%19[^:]:%255[^\n]", sender, timestamp, msg);
    if (scanned != 3) {
        printk(KERN_INFO "ChatDB: Invalid message format\n");
        return -EINVAL;
    }
    
    if (mutex_lock_interruptible(&chatDB->lock)) {
        return -ERESTARTSYS;
    }
    
    // Check if database is full
    if (chatDB->msg_count >= MAX_MESSAGES) {
        // Shift messages to make room for new one
        memmove(&chatDB->messages[0], &chatDB->messages[1], 
                (MAX_MESSAGES - 1) * sizeof(ChatMessage));
        chatDB->msg_count = MAX_MESSAGES - 1;
    }
    
    // Store the new message
    strncpy(chatDB->messages[chatDB->msg_count].sender, sender, 31);
    strncpy(chatDB->messages[chatDB->msg_count].timestamp, timestamp, 19);
    strncpy(chatDB->messages[chatDB->msg_count].content, msg, MAX_MSG_SIZE - 1);
    
    chatDB->messages[chatDB->msg_count].sender[31] = '\0';
    chatDB->messages[chatDB->msg_count].timestamp[19] = '\0';
    chatDB->messages[chatDB->msg_count].content[MAX_MSG_SIZE - 1] = '\0';
    
    chatDB->msg_count++;
    
    mutex_unlock(&chatDB->lock);
    
    printk(KERN_INFO "ChatDB: Stored message from %s\n", sender);
    return len;
}

module_init(chat_device_init);
module_exit(chat_device_exit);
