#
# @(#)alt_platform.make	1.9 06/01/26 12:30:55
#
# Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
#

#----------------------------------------------------------------------
#
# Alternative platform configuration section for eabi
#
# This file is included by these lines in jvm.make:
#
# ifdef ALT_PLATFORM
# include $(SHARE_DIR)/$(ALT_PLATFORM)/alt_platform.make
# endif
#
# The symbol ALT_PLATFORM is defined by the eabi build
# configuration file $(JVMWorkSpace)/build/javacall_arm_eabi/javacall_arm_eabi.config
#----------------------------------------------------------------------

MakeDepsMain_eabi     = UnixPlatform
MakeDepsOpts_eabi     = -resolveVpath true
LOOP_GEN_ARG_eabi     =

ASM_eabi              = arm-eabi-as

ifeq ($(ENABLE_THUMB_VM), true)
CPP_eabi              = arm-eabi-g++
CC_eabi               = arm-eabi-gcc
else
CPP_eabi              = arm-eabi-g++
CC_eabi               = arm-eabi-gcc
endif

LINK_eabi             = arm-eabi-gcc
LIBMGR_eabi           = arm-eabi-ar

SAVE_TEMPS_eabi       =
OBJ_SUFFIX_eabi       = .o
LIB_PREFIX_eabi       =
LIB_SUFFIX_eabi       = .a
EXE_SUFFIX_eabi       = .elf
ASM_SUFFIX_eabi       = .s

EXTRA_CLEAN_eabi     = Interpreter_$(arch).s
ROMIZING_CFLAGS_eabi = -DROMIZING=1

MakeDepsSources_eabi = \
	$(WorkSpace)/src/tools/MakeDeps/UnixPlatform.java
