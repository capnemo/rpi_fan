
obj-m += fan_control.o

ifeq ($(KERNEL_SRC),)
MAKE_LOC := /lib/modules/$(shell uname -r)/build
else
MAKE_LOC := $(KERNEL_SRC)/build
endif

driver:
	make -C $(MAKE_LOC) M=$(PWD) modules

clean:
	make -C $(MAKE_LOC) M=$(PWD) clean
