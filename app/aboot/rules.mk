LOCAL_DIR := $(GET_LOCAL_DIR)

MODULES += \
	lib/ext4 \
	lib/zlib \
	lib/tar

INCLUDES += -I$(LK_TOP_DIR)/platform/msm_shared/include

DEFINES += GRUB_LOADING_ADDRESS=$(GRUB_LOADING_ADDRESS)
ifneq ($(GRUB_BOOT_PARTITION),)
CFLAGS += -DGRUB_BOOT_PARTITION=\"$(GRUB_BOOT_PARTITION)\"
endif

ifeq ($(ENABLE_2NDSTAGE_BOOT),1)
DEFINES += BOOT_2NDSTAGE=1
endif

OBJS += \
	$(LOCAL_DIR)/aboot.o \
	$(LOCAL_DIR)/fastboot.o \
	$(LOCAL_DIR)/recovery.o \
	$(LOCAL_DIR)/grub.o

include $(LOCAL_DIR)/uboot_api/rules.mk
