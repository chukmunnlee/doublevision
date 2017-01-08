# 
# Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
#


ifeq ($(JAVACALL_OSS_DIR),)
$(error JAVACALL_OSS_DIR is not set.)
endif

#For each project specified environment shall be selected
include environment.gmk

#For each project location for basic definitions shall be specified
#(location of the used javacall_platform_defs.h)
JAVACALL_PLATFORM_DEFS_DIR=$(JAVACALL_OSS_DIR)/implementation/nds

#For each project specified toolchain shall be selected
include $(JAVACALL_OSS_DIR)/build/eabi/rules.gmk

#Specific targets can be achieved before and after achievement of common targets
#Specify these targets after including 'rules.gmk' file.
pre_target: 
post_target:
