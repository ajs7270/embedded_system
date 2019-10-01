CC = /home/image/workspace/android_imx8mq/android_build/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-gcc

LD = /home/image/workspace/android_imx8mq/android_build/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-ld

obj-m := hello.o

KDIR := /home/image/workspace/android_imx8mq/android_build/out/target/product/evk_8mq/obj/KERNEL_OBJ/

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod.*
	rm -f .*.cmd
	rm -f moudles.order Module.symvers
