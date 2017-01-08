#
# @(#)alt_compiler.make	1.53 06/05/02 14:41:07
#
# Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
#

#----------------------------------------------------------------------
#
# Alternative compiler section for eabi (arm-eabi-gcc-4.3.0)
#
# This file is included by these lines in jvm.make:
#
# ifdef ALT_COMPILER
# include $(SHARE_DIR)/$(ALT_COMPILER)/alt_config.make
# endif
#
# The symbol ALT_COMPILER is defined by the eabi build
# configuration file $(JVMWorkSpace)/build/javacall_arm_eabi/javacall_arm_eabi.config
#----------------------------------------------------------------------

ifeq ($(compiler), eabi)

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

ifneq ($(NO_DEBUG_SYMBOLS), true)
DEBUG_SYMBOLS_FLAGS= 
else
##ZIV DEBUG_SYMBOLS_FLAGS	= -O2 -Otime
DEBUG_SYMBOLS_FLAGS= 
endif

# By default, the default FPU option is used (on ADS 1.2 this would
# be "-fpu softvfp".) You can override it for platforms	with hardware
# FPU support:
# If you're using ENABLE_THUMB_VM=false, try
#     JVM_FPU_FLAGS = -fpu vfp
# If you're using ENABLE_THUMB_VM=true, try
#     JVM_FPU_FLAGS = -fpu softvfp+vfp

ifndef JVM_FPU_FLAGS
JVM_FPU_FLAGS=
endif

ifeq ($(ENABLE_PCSL), true)
PCSL_DIST_DIR           = $(PCSL_OUTPUT_DIR)/javacall_arm

CPP_INCLUDE_DIRS+= -I"$(PCSL_DIST_DIR)/inc" 

ifdef PRK_INCLUDE_DIR
    CPP_INCLUDE_DIRS+= -I$(PRK_INCLUDE_DIR) 
endif
ifdef PR_CONFIG_DIR
    CPP_INCLUDE_DIRS+= -I$(PR_CONFIG_DIR) 
endif
ifdef PRFILE_INCLUDE_DIR
    CPP_INCLUDE_DIRS+= -I$(PRFILE_INCLUDE_DIR) 
endif
ifdef PRK_COMMON_INCLUDE_DIR
    CPP_INCLUDE_DIRS+= -I$(PRK_COMMON_INCLUDE_DIR) 
endif

LIBNDS_DIR=$(DEVKITPRO)/libnds
CPP_INCLUDE_DIRS+=-I$(LIBNDS_DIR)/include/


PCSL_LIBS               = $(PCSL_DIST_DIR)/lib/libpcsl_memory.a    \
                          $(PCSL_DIST_DIR)/lib/libpcsl_print.a     \
                          $(PCSL_DIST_DIR)/lib/libpcsl_network.a   \
                          $(PCSL_DIST_DIR)/lib/libpcsl_string.a    \
                          $(PCSL_DIST_DIR)/lib/libpcsl_file.a
EXTRA_LIBS             += $(PCSL_LIBS)
MAKE_EXPORT_EXTRA_LIBS += $(PCSL_LIBS)
endif

CPP_OPT_FLAGS= 

# TODO - restore this section FIXME JFH
#CPP_OPT_FLAGS_debug	= $(DEBUG_SYMBOLS_FLAGS)
#CPP_OPT_FLAGS_release	= -O2
#ifeq ($(PRODUCT_DEBUG), true)
#CPP_OPT_FLAGS_product    = $(DEBUG_SYMBOLS_FLAGS)
#else 
#CPP_OPT_FLAGS_product	= -O1
#endif

CPP_OPT_FLAGS_debug= -O1 $(DEBUG_SYMBOLS_FLAGS)
CPP_OPT_FLAGS_release= -O1
ifeq ($(PRODUCT_DEBUG), true)
CPP_OPT_FLAGS_product= $(DEBUG_SYMBOLS_FLAGS)
else 
CPP_OPT_FLAGS_product= -O1
endif
CPP_OPT_FLAGS       += $(CPP_OPT_FLAGS_$(BUILD))

CPP_DEF_FLAGS_debug= -D_DEBUG -DAZZERT
CPP_DEF_FLAGS_release=
CPP_DEF_FLAGS_product= -DPRODUCT

ASM_FLAGS +=  -mcpu=arm9tdmi -mthumb-interwork
LINKER_OUTPUT = -o 

CPP_DEF_FLAGS += -MMD -MP -mcpu=arm9tdmi -mtune=arm9tdmi -fomit-frame-pointer \
-ffast-math -DARM9 -fno-exceptions

# Used in SAVE_TEMPS mode
INTERLEAVE_LISTING  = 

APCS_FLAGS = -mthumb-interwork

CPP_DEF_FLAGS       += ${JVM_FPU_FLAGS}
CPP_DEF_FLAGS       += -DUNDER_EABI $(APCS_FLAGS)
CPP_DEF_FLAGS       += $(CPP_DEF_FLAGS_$(BUILD))
CPP_DEF_FLAGS       += $(BUILD_VERSION_CFLAGS)

# You can use USER_CPP_FLAGS to pass additional flags to the C compiler.
# E.g., gnumake USER_CPP_FLAGS="-DMAX_METHOD_TO_COMPILE=400"
ifdef USER_CPP_FLAGS
CPP_DEF_FLAGS       += $(USER_CPP_FLAGS)
endif

CPP_FLAGS_EXPORT= $(CPP_DEF_FLAGS) \
                  $(SAVE_TEMPS_CFLAGS) $(ROMIZING_CFLAGS) \
                  $(ENABLE_CFLAGS)

CPP_FLAGS = $(CPP_OPT_FLAGS) $(CPP_FLAGS_EXPORT) $(CPP_INCLUDE_DIRS)

LINK_OPT_FLAGS_debug=
LINK_OPT_FLAGS_release=
LINK_OPT_FLAGS_product=

LINK_FLAGS       += $(LINK_OPT_FLAGS_$(BUILD))

ASM_FLAGS_debug       += -g
ASM_FLAGS_release     += -g
ASM_FLAGS_product     +=
ASM_FLAGS       += $(ASM_FLAGS_$(BUILD)) $(APCS_FLAGS)

LIB_FLAGS       += cqs

Interpreter_$(arch).o: Interpreter_$(arch).s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	@$(ASM) $(ASM_FLAGS)	-o Interpreter_$(arch).o Interpreter_$(arch).s
	$(A)if test ! -f "$@"; then exit 2; fi

define BUILD_C_TARGET
	$(A)echo " ... $(notdir	$<)"
	$(A)rm -f $@
	$(A)if test "$(SAVE_TEMPS)" = "true"; \
	    then $(CPP)	$(CPP_FLAGS) -S	${INTERLEAVE_LISTING} $<; echo ...; \
	 fi
	$(A)$(CPP) $(CPP_FLAGS)	-c $<
	$(A)if test ! -f "$@"; then exit 2; fi
endef

define BUILD_C_TARGET_ARM_MODE
	$(A)echo " ... $(notdir	$<)"
	$(A)rm -f $@
	$(A)if test "$(SAVE_TEMPS)" = "true"; \
	    then arm-eabi-g++	$(CPP_FLAGS) -S	${INTERLEAVE_LISTING} $<; echo ...; \
	 fi
	$(A)arm-eabi-g++ $(CPP_FLAGS)	-c $<
	$(A)if test ! -f "$@"; then exit 2; fi
endef

HotRoutines1.o: $(WorkSpace)/src/vm/share/runtime/HotRoutines1.cpp
	$(BUILD_C_TARGET_ARM_MODE)

# Definitions to be exported in	$(DIST_LIB_DIR)/$(JVM_MAK_NAME)
MAKE_EXPORT_LINK_OUT_SWITCH1 = -O
MAKE_EXPORT_LINK_OUT_SWITCH2 =
MAKE_EXPORT_EXTRA_LIBS =
MAKE_EXPORT_THREAD_LIBS =

$(DIST_LIB_DIR)/$(JVM_LOG_NAME)::
	$(A)rm -f $@
	$(A)echo TBD > $@

# Split the object files into:
#     -LIB_OBJS: the ones that are exported
#     -EXE_OBJS: the ones that are used only by $(JVM_EXE)
LIB_OBJS := $(Obj_Files) Interpreter_$(arch).o OopMaps.o
LIB_OBJS := $(subst BSDSocket.o,,$(LIB_OBJS))
LIBX_OBJS +=    BSDSocket.o
LIB_OBJS := $(subst ReflectNatives.o,,$(LIB_OBJS))
LIBTEST_OBJS +=    ReflectNatives.o
LIB_OBJS := $(subst jvmspi.o,,$(LIB_OBJS))
EXE_OBJS +=    jvmspi.o
LIB_OBJS := $(subst Main_$(os_family).o,,$(LIB_OBJS))
EXE_OBJS +=    Main_$(os_family).o
LIB_OBJS := $(subst NativesTable.o,,$(LIB_OBJS))
EXE_OBJS +=    NativesTable.o
LIB_OBJS := $(subst ROMImage.o,,$(LIB_OBJS))
EXE_OBJS +=    ROMImage.o

$(JVM_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIB_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(LIB_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMX_LIB): $(BIN_DIR)	$(BUILD_PCH) $(LIBX_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(LIBX_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMTEST_LIB):	$(BIN_DIR) $(BUILD_PCH)	$(LIBTEST_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(LIBTEST_OBJS)
	$(A)echo generated `pwd`/$@

SCATTER_MAP = $(WorkSpace)/src/vm/os/$(os_family)/cldc_vm.load
$(JVM_EXE): $(BIN_DIR) $(CLDC_ZIP) $(EXE_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) \
	    $(JVM_LIB) $(SCATTER_MAP)
ifeq ($(skip_link_image), true)
	$(A)touch $@
else
	$(A)echo "Linking $@..."
	$(A)$(LINK) $(LINK_FLAGS) $(LINKER_OUTPUT) $@ \
	    $(EXE_OBJS) $(JVM_LIB) $(JVMX_LIB) $(JVMTEST_LIB) $(EXTRA_LIBS) \
	    $(EXTRA_DASH)-map $(EXTRA_DASH)-symbols \
	    $(EXTRA_DASH)-list $(JVM_MAP) $(EXTRA_DASH)-entry 0x00008000 \
	    $(EXTRA_DASH)-scatter $(SCATTER_MAP)
endif
	$(A)echo generated `pwd`/$@

PRINT_CONFIG = print_config

print_config:
	$(A)echo "=========================================="
	$(A)echo "COMPILER toolchain   = $(compiler)"
	$(A)echo "CPP                  = $(CPP)"
	$(A)echo "ADS_LINUX_HOST	   = $(ADS_LINUX_HOST)"
	$(A)echo "ENABLE_THUMB_VM	   = $(ENABLE_THUMB_VM)"
	$(A)echo "ENABLE_SOFT_FLOAT	   = $(ENABLE_SOFT_FLOAT)"
	$(A)echo "ENABLE_SOURCE_GENERATORS = $(ENABLE_SOURCE_GENERATORS)"
	$(A)echo "=========================================="

endif
