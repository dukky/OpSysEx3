KERNELDIR=/lib/modules/`uname -r`/build
#ARCH=i386
#KERNELDIR=/usr/src/kernels/`uname -r`-i686

MODULES = firewallExtension.ko findExecutable.ko kernelWrite.ko

obj-m += firewallExtension.o findExecutable.o kernelWrite.o

all: firewallSetup
	make -C  $(KERNELDIR) M=$(PWD) modules

firewallSetup:
	gcc -o firewallSetup -Werror -Wall firewallSetup.c

clean:
	make -C $(KERNELDIR) M=$(PWD) clean

install:
	make -C $(KERNELDIR) M=$(PWD) modules_install

quickInstall:
	cp $(MODULES) /lib/modules/`uname -r`/extra
