## LKM GPIO Driver
headers of GPIO are in `#include <linux/gpio.h>`
steps for using GPIO pin (this example is targeted to RPi 3B+)

### Init the GPIO pin
in the init function after creating and allocating char dev and defining the file system for the device in the \_\_init__ method (as in this repo), these steps should be followed to use a GPIO pin

1- **Check if the pin is available**
```c
bool gpio_is_valid(int gpio_number);
```
2- **request the pin**
```c
int gpio_request(unsigned gpio, const char *label)
```
3- **Set the direction (input/output)**
to define as input:
```c
int  gpio_direction_input(unsigned gpio)
```

to define as output:
```c
int  gpio_direction_output(unsigned gpio, int initial_value)
```
**The complete Code**
```c
#ifndef __GPIO_H
#define __GPIO_H

#define CLASS_NAME "GPIO_CLASS"
#define DEVICE_NAME "GPIO_DEVICE"
#define DRIVER_NAME "GPIP_DRIVER"
#define GPIO_PIN (526) //GPIO 15  using gpiochip512

#endif
```

```c
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
    //Checking whether the GPIO is valid or not
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
```

### Control the Pin

1- **get pin value**
```c
int  gpio_state = gpio_get_value(GPIO_PIN); //0 or 1
```

2- **set pin value**
```c
gpio_set_value(GPIO_PIN, 1);
```
**The complete Code**
```c
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
```

### Compile and Use
after executing the `Makefile` by the command make `make` utility. execute the following command

```bash
pi@raspberrypi:~ $ echo "1" | sudo tee /dev/GPIO_DEVICE 
```
or
```bash
pi@raspberrypi:~ $ echo "0" | sudo tee /dev/GPIO_DEVICE 
```

Note:
the `>` and `>>` operators are not command, so we can't use sudo before this operators as sudo is a command that lets a user execute a command as another user (find more in my [repo](https://github.com/ziadasem/Linux-System-Admin/blob/main/3-%20Users%20and%20access%20control/#3-%20sudo%20and%20sudoers%20file.md)), hence we use `tee` command

