###############################################################################
#
# Copyright (c) Imagination Technologies Ltd.
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a 
# copy of this software and associated documentation files (the "Software"), 
# to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
# THE SOFTWARE.
# 
# Alternatively, the contents of this file may be used under the terms of the 
# GNU General Public License Version 2 ("GPL")in which case the provisions of
# GPL are applicable instead of those above. 
# 
# If you wish to allow use of your version of this file only under the terms 
# of GPL, and not to allow others to use your version of this file under the 
# terms of the MIT license, indicate your decision by deleting the provisions 
# above and replace them with the notice and other provisions required by GPL 
# as set out in the file called “GPLHEADER” included in this distribution. If 
# you do not delete the provisions above, a recipient may use your version of 
# this file under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called 
# "MIT_COPYING".
#
###############################################################################

### Module name
MODULE = tee_vxd

### Required objects
OBJS = driver/secureapi.o
OBJS += vxdio/code/msvdx_io.o vxdio/code/mtxio.o
OBJS += bspp/bspp.o
OBJS += bspp/h264_secure_parser.o bspp/h264_secure_sei_parser.o
OBJS += bspp/swsr.o
OBJS += bspp/pixel_api.o
OBJS += driver/imgvideo/tal/code/tal.o
OBJS += driver/imgvideo/port_fwrk/libraries/idgen_api.o
# This code could be replaced
OBJS += driver/imgvideo/list_utils/lst.o
# Check if we can remove these libraries
OBJS += driver/imgvideo/target/code/target.o

### Flags required by the compiler
KBUILD_CFLAGS := $(filter-out -Werror,$(KBUILD_CFLAGS))

ifneq ( ,$(findstring arm64,$(ARCH)))
ccflags-y += -fno-pic
endif

ifeq (OFF,ON)
ccflags-y += -DPOST_TEST
endif

ifeq (Release,Debug)
ccflags-y += -g -D__RELEASE__="debug" -D__RELEASE_DEBUG__ -DIMG_DEBUG_FLAG -DDEBUG
else
ccflags-y += -D__RELEASE__="release" -D__RELEASE_RELEASE__ -DNO_REGIO_CHECK_FIELD_VALUE -DNDEBUG
endif
ccflags-y += -DVDEC -DUSE_SHARING -DUSE_FW_CTX_TRIMMING -DUSE_FW_RELOC_INFO_PACKING
ccflags-y += -DH264_ENABLE_MVC -DH264_SECOND_PASS_DEBLOCK -DENABLE_SEPARATE_COLOUR_PLANES
ccflags-y += -DVDEC_MSVDX_HARDWARE
ccflags-y += -D__linux__
ccflags-y += -DFAKE_DEVIF -DENABLE_NULL_DEVIF
ccflags-y += -DOMX_SHARED
ccflags-y += -D__PORT_FWRK__
ccflags-y += -DUSE_REAL_FW
ccflags-y += -DSEC_USE_REAL_FW -DUSE_MTX_DMA_LOAD
ccflags-y += -DSECURE_MEDIA_REPORTING
ccflags-y += -DSEC_USE_FW_RELOC_INFO_PACKING -DSEC_USE_FW_CTX_TRIMMING
ccflags-y += -D__TALMMU_USE_SYSAPIS__ -D__TALMMU_NO_TAL__
ccflags-y += -DIMG_KERNEL_MODULE
ccflags-y += -D_FILE_OFFSET_BITS=64
ccflags-y += -DBSPP_KM

ccflags-y += -I$(src)/include
ccflags-y += -I$(src)/include/firmware/include
ccflags-y += -I$(src)/include/firmware/share
ccflags-y += -I$(src)/include/firmware/share/c99
ccflags-y += -I$(src)/driver/imgvideo/include
ccflags-y += -I$(src)/driver/imgvideo/tal/include
ccflags-y += -I$(src)/driver/imgvideo/port_fwrk/include
ccflags-y += -I$(src)/driver/imgvideo/target/include
ccflags-y += -I$(src)/include/regdefs
ccflags-y += -I$(src)/bspp/include
ccflags-y += -I$(src)/vxdio/include
ccflags-y += -I$(src)/vxdio/include/vxd
ccflags-y += -I$(src)/secure_api
ccflags-y += -I$(src)/platform/include

include $(src)/platform/makefile.inc

obj-m := $(MODULE).o

$(MODULE)-objs := $(OBJS)

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
		$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

# Clean locally
clean:
		rm -f *.o *.ko Module.symvers modules.order *.mod.c $(OBJS)

# Show all the binary segments information from all the objects included in the
# build. It uses the list contained in $(OBJS)
objsizes:
		objdump -h $(OBJS) 

