PWD := $(shell pwd)
# OS_VER := $(shell uname -r)
OS_VER := 6.1.130
KERNEL_DIR ?= /lib/modules/$(OS_VER)/build

KSRC_DIR := $(PWD)/km
USRC_DIR := $(PWD)/um
INT_DIR := $(PWD)/temp
BUILD_DIR := $(PWD)/build
INT_FILES := $(PWD)/.*.cmd $(PWD)/*.o $(PWD)/*.mod $(PWD)/*.mod.c $(PWD)/*.symvers $(PWD)/*.order
INT_FILES1 := $(KSRC_DIR)/*.o $(KSRC_DIR)/.*.cmd
# BUILD_FILES += $(SRC_DIR)/*.ko
BUILD_FILES := $(PWD)/*.ko
UMODNAME := pthread
UMODNAME_OLD := pthread-2.11

CLANG_FORMAT_VERS ?= 18
CLANG_FORMAT := clang-format-$(CLANG_FORMAT_VERS)
CLANG_FORMAT_FLAGS += -i
FORMAT_FILES := $(KSRC_DIR)/*.c

$(info PWD: $(PWD))
$(info OS_VER: $(OS_VER))
$(info KERNEL_DIR: $(KERNEL_DIR))

$(shell mkdir -p $(INT_DIR)/km)
$(shell mkdir -p $(BUILD_DIR))

kbuild:
	clear
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	mv $(INT_FILES) $(INT_DIR)/.
	mv $(INT_FILES1) $(INT_DIR)/km/.
	mv $(BUILD_FILES) $(BUILD_DIR)/.

ubuild:
	clear
	gcc -DUSERMODE -DUBUILD_MODNAME=\"pthread\" $(USRC_DIR)/*.c -o $(BUILD_DIR)/$(UMODNAME) -I./inc -lpthread

ubuild_old:
	clear
	gcc -DUSERMODE -DUBUILD_MODNAME=\"pthread\" $(USRC_DIR)/*.c -o $(BUILD_DIR)/$(UMODNAME_OLD) -I./inc -lpthread -L/usr/toolchain/x86_64-pc-linux-gnu_gcc7.3.0_glibc2.11/x86_64-pc-linux-gnu/lib #-Wl,--dynamic-linker=/usr/toolchain/x86_64-pc-linux-gnu_gcc7.3.0_glibc2.11/x86_64-pc-linux-gnu/lib/ld-linux-x86-64.so.2

format:
	$(CLANG_FORMAT) $(CLANG_FORMAT_FLAGS) $(FORMAT_FILES)

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -r $(INT_DIR)
	rm -r $(BUILD_DIR)

.PHONY: kbuild ubuild format clean
