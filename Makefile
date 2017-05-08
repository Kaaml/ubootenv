#
# Makefile for the linux ramfs routines.
#

#obj-y += ubootenv.o
obj-m += ubootenv.o

#file-mmu-y := file-nommu.o
#file-mmu-$(CONFIG_MMU) := file-mmu.o
ubootenv-objs += inode.o file.o #$(file-mmu-y) file.o
