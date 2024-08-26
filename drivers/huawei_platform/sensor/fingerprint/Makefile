fpc1020-objs := fpc1020_main.o fpc1020_common.o fpc1020_capture.o fpc1020_navlib.o


ifeq ($(CONFIG_FINGERPRINT_FPC1150),y)
    fpc1020-objs += fpc1150_touch.o
else
    fpc1020-objs += fpc1020_touch.o
endif

obj-$(CONFIG_FINGERPRINT)  += fpc1020.o
