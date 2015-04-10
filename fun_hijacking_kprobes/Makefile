obj-m:=anima.o
KERNEL_SOURCE := /usr/src/linux
PWD := $(shell pwd)

all: default

default:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean
