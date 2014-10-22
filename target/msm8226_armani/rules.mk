LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += -I$(LOCAL_DIR)/../msm8226/include -I$(LK_TOP_DIR)/platform/msm_shared
INCLUDES += -I$(LK_TOP_DIR)/dev/gcdb/display -I$(LK_TOP_DIR)/dev/gcdb/display/include

PLATFORM := msm8226

MEMBASE ?= 0x07F00000 # SDRAM
MEMSIZE := 0x00100000 # 1MB

BASE_ADDR        := 0x00000

TAGS_ADDR        := BASE_ADDR+0x00000100
KERNEL_ADDR      := BASE_ADDR+0x00008000
RAMDISK_ADDR     := BASE_ADDR+0x01000000
SCRATCH_ADDR     := 0x10000000
SCRATCH_ADDR_128MAP     := 0x04200000
SCRATCH_ADDR_512MAP     := 0x10000000
SCRATCH_SIZE_128MAP     := 0x03D00000
SCRATCH_SIZE_512MAP     := 0x20000000

DEFINES += DISPLAY_SPLASH_SCREEN=1
DEFINES += DISPLAY_TYPE_MIPI=1
DEFINES += DISPLAY_TYPE_DSI6G=1

MODULES += \
	dev/keys \
	lib/ptable \
	dev/pmic/pm8x41 \
	dev/gcdb/display \
	dev/vib \
	lib/libfdt

DEFINES += \
	MEMSIZE=$(MEMSIZE) \
	MEMBASE=$(MEMBASE) \
	BASE_ADDR=$(BASE_ADDR) \
	TAGS_ADDR=$(TAGS_ADDR) \
	KERNEL_ADDR=$(KERNEL_ADDR) \
	RAMDISK_ADDR=$(RAMDISK_ADDR) \
	SCRATCH_ADDR=$(SCRATCH_ADDR) \
	SCRATCH_ADDR_128MAP=$(SCRATCH_ADDR_128MAP) \
	SCRATCH_ADDR_512MAP=$(SCRATCH_ADDR_512MAP) \
	SCRATCH_SIZE_128MAP=$(SCRATCH_SIZE_128MAP) \
	SCRATCH_SIZE_512MAP=$(SCRATCH_SIZE_512MAP)

ifneq ($(ENABLE_2NDSTAGE_BOOT),1)
OBJS += \
    $(LOCAL_DIR)/../msm8226/target_display.o
endif

OBJS += \
    $(LOCAL_DIR)/../msm8226/init.o \
    $(LOCAL_DIR)/../msm8226/meminfo.o \
    $(LOCAL_DIR)/oem_panel.o