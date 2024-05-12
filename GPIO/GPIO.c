#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include<linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/gpio.h>

#include "GPIO.h"

dev_t device_numeber ;
struct cdev st_characterDevice;
struct class* gpio_class ;
struct device *gpio_device; 

static int openCallback (struct inode* node, struct file* file);
static int releaseCallback (struct inode* node, struct file* file);
ssize_t readCallback (struct file* file, char __user * userbuffer, size_t count, loff_t *offset);
ssize_t writeCallback(struct file* file, const char * _shared_buffer, size_t count, loff_t *off);


struct  file_operations fops =
{
    .owner = THIS_MODULE,
    .open = openCallback,
    .release= releaseCallback,
    .read = readCallback,
    .write = writeCallback,
};



static int __init GPIO_init(void){
    //1- init char device
    int retval;
    retval = alloc_chrdev_region(&device_numeber, 0, 1,  DRIVER_NAME);
    if (retval != 0){ //failed
        pr_err("can't allocate char device \n");
        goto ALLOC_CHARDEV ;
    }

    //init a charachter device
    cdev_init(&st_characterDevice, &fops); //append fops to char device
    retval = cdev_add(&st_characterDevice,  device_numeber, 1);

    if (retval != 0 ){ //not sucess
        pr_err("can't add char device file \n");
        goto CHAR_ERROR;
    }

    //create a file customized to a class
    gpio_class = class_create(CLASS_NAME);
    if (gpio_class == NULL){
        pr_err("can't allocate class \n");
        goto CLASS_ERROR ;
    }

    gpio_device = device_create(gpio_class, NULL, device_numeber, NULL, DEVICE_NAME);
    if (gpio_device == NULL){
        pr_err("can't allocate device \n");
        goto DEVICE_ERROR   ;    
    }
    

    //2- init GPIO  
    //Checking the GPIO is valid or not
    if(! gpio_is_valid(GPIO_PIN)){
        pr_err("GPIO %d is not valid\n", GPIO_PIN);
        goto GPIO_NOT_VALID;
    }
  
    //Requesting the GPIO
    retval = gpio_request(GPIO_PIN,"GPIO_15") ;
    if(retval != 0){
        pr_err("ERROR: GPIO %d request\n", GPIO_PIN);
        goto GPIO_NOT_REQUESTED;
    }
  
    //configure the GPIO as output with init value = 0
    retval = gpio_direction_output(GPIO_PIN, 0);
    if(retval != 0){
        pr_err("can't set pin %d to output \n", GPIO_PIN);
        goto GPIO_NOT_SET;
    }
    return 0 ;

    GPIO_NOT_SET:
    GPIO_NOT_REQUESTED :
    GPIO_NOT_VALID :
        gpio_free(GPIO_PIN);
        device_destroy(gpio_class, device_numeber);
    DEVICE_ERROR:
        class_destroy(gpio_class);
    CLASS_ERROR:
        class_destroy(gpio_class);
        cdev_del(&st_characterDevice);        
    CHAR_ERROR:
        unregister_chrdev_region(device_numeber, 1);
       
    ALLOC_CHARDEV:
        return -1 ;
}



static void __exit GPIO_exit(void){

    gpio_free(GPIO_PIN);
    device_destroy(gpio_class, device_numeber);
    class_destroy(gpio_class);
    class_destroy(gpio_class);
    cdev_del(&st_characterDevice);        
    unregister_chrdev_region(device_numeber, 1);

    printk("Module is closed!\n");
}


static int openCallback (struct inode* node, struct file* file){
    printk("%s file is opened! \n", __FUNCTION__);
    return 0;
}

static int releaseCallback (struct inode* node, struct file* file){
    printk("%s file is released! \n", __FUNCTION__);
    return 0;
}

ssize_t readCallback (struct file* file, char __user * userbuffer, size_t count, loff_t *offset){
    
    int gpio_state = gpio_get_value(GPIO_PIN);  //0 or 1
    int ret ;
    ret = copy_to_user(userbuffer, &gpio_state, count);
    if (ret!= 0){
        return -1;
    }
    return count ;  //success bytes
}

ssize_t writeCallback(struct file* file, const char * _shared_buffer, size_t count, loff_t *off){
    int ret ;
    
    uint8_t buffer[10] = {0};
    ret = copy_from_user(&buffer[*off], _shared_buffer, count);
    
    if (ret!= 0){
        pr_err("can't read from user \n");
        return -1;
    }
    
    if (buffer[0]=='1') {
        //set the GPIO value to HIGH
        gpio_set_value(GPIO_PIN, 1);
    } else if (buffer[0]=='0') {
        //set the GPIO value to LOW
        gpio_set_value(GPIO_PIN, 0);
    } else {
        pr_err("undefined value, should be 0 or 1 \n");
    }
    return count;
}

module_init(GPIO_init);
module_exit(GPIO_exit);


/*Meta information*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ziad Assem");
MODULE_DESCRIPTION("a simple driver for GPIO, especially for RPi");

