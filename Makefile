obj-m := fake_mouse_kernel.o

# 换成你自己系统的内核版本
KVERSION := $(shell uname -r)

.PHONY: all clean

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
