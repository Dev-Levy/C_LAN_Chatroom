#include "char_device.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marhakosztaga");
MODULE_DESCRIPTION("A char device driver for chat application database");
MODULE_VERSION("0.1");

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

// In char_device.c (kernel module)
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    char temp_buffer[MAX_MSG_LEN + 64] = {0};
    int bytes_to_copy = 0;
    
    if (mutex_lock_interruptible(&chatDB->lock))
        return -ERESTARTSYS;

    // Check if we've read all messages
    if (*offset >= chatDB->msg_count) {
        mutex_unlock(&chatDB->lock);
        return 0;  // EOF
    }

    // Format: "TIMESTAMP=2025-05-01 21:53:00|SENDER=Marci|MESSAGE=Na mivan\n"
    snprintf(temp_buffer, sizeof(temp_buffer), 
             "TIMESTAMP=%s|SENDER=%s|MESSAGE=%s\n",
             chatDB->messages[*offset].timestamp,
             chatDB->messages[*offset].sender,
             chatDB->messages[*offset].message);

    bytes_to_copy = strlen(temp_buffer);
    
    if (bytes_to_copy > len) {
        mutex_unlock(&chatDB->lock);
        return -EFAULT;
    }

    if (copy_to_user(buffer, temp_buffer, bytes_to_copy)) {
        mutex_unlock(&chatDB->lock);
        return -EFAULT;
    }

    *offset += 1;
    mutex_unlock(&chatDB->lock);
    return bytes_to_copy;
}


// Device write function - stores new message
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    char temp_buffer[MAX_MSG_LEN + 64] = {0}; 
    // char msg[MAX_MSG_LEN] = {0};
    // char sender[32] = {0};
    // char timestamp[20] = {0};
    // int scanned;
    
    // Limit message size
    // if (len > MAX_MSG_LEN + 64) {
    //     len = MAX_MSG_LEN + 64;
    // }
    if (copy_from_user(temp_buffer, buffer, min(len, sizeof(temp_buffer)-1))){
        printk(KERN_INFO "ChatDB: Failed to receive message from user\n");
        return -EFAULT;
    }

    if (mutex_lock_interruptible(&chatDB->lock)) {
        return -ERESTARTSYS;
    }
    char *timestamp = strstr(temp_buffer, "TIMESTAMP=");
    char *sender = strstr(temp_buffer, "SENDER=");
    char *message = strstr(temp_buffer, "MESSAGE=");

    if (!timestamp || !sender || !message)
        return -EINVAL;

    // Advance pointers past the labels
    timestamp += 10; // Skip "TIMESTAMP="
    sender += 7;     // Skip "SENDER="
    message += 8;    // Skip "MESSAGE="

    // Find delimiters
    char *timestamp_end = strchr(timestamp, '|');
    char *sender_end = strchr(sender, '|');
    
    if (!timestamp_end || !sender_end)
        return -EINVAL;

    // Null-terminate each field
    *timestamp_end = '\0';
    *sender_end = '\0';
    
    // Check if database is full
    if (chatDB->msg_count >= MAX_MESSAGES) {
        // Shift messages to make room for new one
        memmove(&chatDB->messages[0], &chatDB->messages[1], 
                (MAX_MESSAGES - 1) * sizeof(ChatMessage));
        chatDB->msg_count = MAX_MESSAGES - 1;
    }
    
    // Store the new message
    strncpy(chatDB->messages[chatDB->msg_count].sender, sender, MAX_SENDER_LEN-1);
    strncpy(chatDB->messages[chatDB->msg_count].timestamp, timestamp, TIMESTAMP_SIZE-1);
    strncpy(chatDB->messages[chatDB->msg_count].message, message, MAX_MSG_LEN - 1);
    
    chatDB->messages[chatDB->msg_count].sender[MAX_SENDER_LEN-1] = '\0';
    chatDB->messages[chatDB->msg_count].timestamp[TIMESTAMP_SIZE-1] = '\0';
    chatDB->messages[chatDB->msg_count].message[MAX_MSG_LEN - 1] = '\0';
    
    chatDB->msg_count++;
    
    mutex_unlock(&chatDB->lock);
    
    printk(KERN_INFO "ChatDB: Stored message from %s\n", sender);
    return len;
}

static loff_t dev_llseek(struct file *filep, loff_t offset, int whence)
{
    loff_t newpos = 0;
    
    switch(whence) {
        case SEEK_SET:
            newpos = offset;
            break;
        case SEEK_CUR:
            newpos = filep->f_pos + offset;
            break;
        case SEEK_END:
            // If you want to support seeking from end, define what "end" means
            // For a chat DB, it could be the number of messages
            newpos = chatDB->msg_count;
            break;
        default:
            return -EINVAL;
    }
    
    if (newpos < 0)
        return -EINVAL;
        
    filep->f_pos = newpos;
    return newpos;
}

module_init(chat_device_init);
module_exit(chat_device_exit);
