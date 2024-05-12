obj-m += GPIO.o

all:
	make -C  /lib/modules/$(shell  uname -r)/build M=$(PWD) modules

clear:
	make -C  /lib/modules/$(shell  uname -r)/build M=$(PWD) clear