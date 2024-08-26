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

# Define name of output binary
OUT_NAME=libtee_k3v3.so

# Define name of objects directory.
OBJDIR=obj

CLIENT_STUFF_DIR=Client_Stuff

# Add Client stuff directory to prerequisites search path
vpath %.c $(CLIENT_STUFF_DIR)

# Source files
SRC_FILES=secdev_hisi.c secureapi_tee.c sysos_api.c platform_secmem.c gp_client_enum.c
CLIENT_STUFF_SRC_FILES=$(CLIENT_STUFF_DIR)/secureapi_ree.c

# Prepare names of object files.
OBJ_FILES=$(patsubst %.c,$(OBJDIR)/%.o,$(SRC_FILES)) \
	$(patsubst $(CLIENT_STUFF_DIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_STUFF_SRC_FILES))

# C flags.
CFLAGS=-O2 -g -fpic -c
CFLAGS+=-DENABLE_LIN_SO_BUILD
CFLAGS+=-I../../include
CFLAGS+=-Iinclude
CFLAGS+=-I../../driver/imgvideo/target/include/
CFLAGS+=-I../../driver/imgvideo/include/
CFLAGS+=-I../../platform/include/
CFLAGS+=-I../../vxdio/include/vxd/
CFLAGS+=-I../../secure_api
CFLAGS+=-ITEE_Include/
CFLAGS+=-I../../driver/imgvideo/port_fwrk/include/
CFLAGS+=-I../../platform/linux_common/include/
CFLAGS+=-IClient_Stuff/Include/
CFLAGS+=-I../../../platform/include/

# Linker flags
LDFLAGS=-shared

# Compilation rule for .c files
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@  $<

# Main target
all: $(OUT_NAME)

$(OUT_NAME): setup $(OBJ_FILES)
	$(CC) $(OBJ_FILES) $(LDFLAGS) -o $(OUT_NAME)

# Cleaning
clean:
	rm -rf $(OBJDIR)
	rm $(OUT_NAME)

# Create object directory
setup:
	@if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi

