export

WSBUILD:=$(shell pwd)

WSROOT:=${WSBUILD}/../..

USE_DEBUG=false
USE_MVM=false
#USE_DEBUG_ROMGEN=true
#USE_DEBUG_ROMGEN_SYMBOLS=true

#javacall
JAVACALL_OSS_DIR:=$(WSROOT)/javacall
CONFIGURATION_PROPERTIES_FILE:=$(WSROOT)/midp/src/configuration/configuration_xml/javacall/properties.xml

JAVACALL_OUTPUT_DIR:=$(WSBUILD)/javacall
#Choose configuration: OSS or commercial
JAVACALL_CONFIG_DIR:=$(JAVACALL_OSS_DIR)/configuration/phoneMEFeature/arm_eabi

#pcsl
PCSL_OUTPUT_DIR:=$(WSBUILD)/pcsl
PCSL_DEBUG_OPTION:=

#cldc
JVMWorkSpace:=$(WSROOT)/cldc
JVMBuildSpace:=$(WSBUILD)/cldc

#midp
MIDP_OUTPUT_DIR:=$(WSBUILD)/midp
CLDC_DIST_DIR:=$(JVMBuildSpace)/javacall_linux_arm_eabi/dist

# Uncomment for slave mode build
#SUBSYSTEM_EVENTS_MODULES=slave_mode

ifeq ($(USE_DEBUG),true)
    DEBUG_OPTION=USE_DEBUG=true
    BUILD=debug
else
    DEBUG_OPTION=USE_DEBUG=false
endif


BUILD_EXT_debug=_g
BUILD_EXT=$(BUILD_EXT_$(BUILD))

JSR_CONFIG:= \
		USE_MIDP=true \
		USE_JSR_75=false \
		USE_JSR_82=false \
		USE_JSR_120=false \
		USE_JSR_135=false \
		USE_JSR_172=false \
		USE_JSR_179=false \
		USE_JSR_177=false \
		USE_JSR_180=false \
		USE_JSR_205=false \
		USE_JSR_211=false \
		USE_JSR_226=false \
		USE_JSR_229=false \
		USE_JSR_234=false \
		USE_JSR_238=false \
		USE_NATIVE_AMS=false
		
# if you set USE_NATIVE_AMS=true, you must set USE_PLATFORM_STORAGE=true
# as well, to avoid compilation errors.		

JSR_DIRS:= \
        ABSTRACTIONS_DIR=$(WSROOT)/abstractions \
		JSR_75_DIR=$(WSROOT)/jsr75 \
		JSR_82_DIR=$(WSROOT)/jsr82 \
		JSR_120_DIR=$(WSROOT)/jsr120 \
		JSR_135_DIR=$(WSROOT)/jsr135 \
		JSR_172_DIR=$(WSROOT)/jsr172 \
		JSR_179_DIR=$(WSROOT)/jsr179 \
		JSR_180_DIR=$(WSROOT)/jsr180 \
		JSR_205_DIR=$(WSROOT)/jsr205 \
		JSR_211_DIR=$(WSROOT)/jsr211 \
		JSR_226_DIR=$(WSROOT)/jsr226 \
		PISCES_DIR=$(WSROOT)/pisces \
		JSR_234_DIR=$(WSROOT)/jsr234 \
		JSR_238_DIR=$(WSROOT)/jsr238 \
		JSR_229_DIR=$(WSROOT)/jsr229

.PHONY: all javacall pcsl cldc midp clean

all: javacall pcsl cldc midp

javacall: 
	$(MAKE) -C $(JAVACALL_CONFIG_DIR) \
		JAVACALL_DIR=$(JAVACALL_OSS_DIR) \
		JAVACALL_OUTPUT_DIR=$(JAVACALL_OUTPUT_DIR) \
		${DEBUG_OPTION} \
		$(PCSL_DEBUG_OPTION) \
		TOOLS_DIR=$(WSROOT)/tools \
		$(subst =,_JC=,$(JSR_CONFIG)) \
		$(JSR_CONFIG) \
		USE_STATIC_PROPERTIES=true

pcsl: 
	$(MAKE) -C $(WSROOT)/pcsl \
		PCSL_PLATFORM=javacall_linux_arm_eabi \
		JAVACALL_OUTPUT_DIR=$(JAVACALL_OUTPUT_DIR) \
		$(DEBUG_OPTION) \
		$(PCSL_DEBUG_OPTION) \
		USE_SERVER_SOCKET=true \
		${DEBUG_OPTION} \
		ENABLE_HIDDEN_DEP_FEATURES=true

cldc:  
	$(MAKE) -C $(WSROOT)/cldc/build/javacall_linux_arm_eabi product\
		${DEBUG_OPTION} \
		ENABLE_PCSL=true \
		ENABLE_ISOLATES=$(USE_MVM) \
		PCSL_OUTPUT_DIR=$(PCSL_OUTPUT_DIR) \
		JAVACALL_OUTPUT_DIR=$(JAVACALL_OUTPUT_DIR)
		#ENABLE_COMPILATION_WARNINGS=true 

midp:
	$(MAKE) -C $(WSROOT)/midp/build/javacall \
		JAVACALL_PLATFORM=linux_arm_eabi \
		JAVACALL_OUTPUT_DIR=$(JAVACALL_OUTPUT_DIR) \
		TOOLS_DIR=$(WSROOT)/tools \
		USE_MULTIPLE_ISOLATES=$(USE_MVM) \
		USE_RESTRICTED_CRYPTO=true \
		USE_SSL=true \
		UNICODE_FILE_SUPPORT=true \
		CASE_INSENSITIVE_FILESYSTEM=true \
		SOFT_KEYBOARD_ENABLED=false \
		${DEBUG_OPTION} \
		RESTRICTED_CRYPTO_DIR=$(WSROOT)/restricted_crypto \
		USE_SERVER_SOCKET=true \
		USE_JAVACALL_SERVER=false \
		IGNORE_CRC_CHECK=false \
		$(JSR_CONFIG) \
		USE_PORTING_DEMOS=true \
		$(JSR_DIRS) \
		USE_PLATFORM_STORAGE=false \
		USE_FILE_SYSTEM=false \
		USE_STATIC_PROPERTIES=true \
		USE_NATIVE_TEXTFIELD=true

clean:
	-rm -rf midp cldc pcsl javacall

