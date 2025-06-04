PWD := $(shell pwd)
OS_VER := $(shell uname -r)
# OS_VER := 6.8.0-40-generic
KERNEL_DIR ?= /lib/modules/$(OS_VER)/build

SRC_DIR := $(PWD)/src
BUILD_DIR := $(PWD)/build
BUILD_FILES += $(SRC_DIR)/*.ko

CLANG_FORMAT_VERS ?= 18
CLANG_FORMAT := clang-format-$(CLANG_FORMAT_VERS)
CLANG_FORMAT_FLAGS += -i
FORMAT_FILES := $(SRC_DIR)/*.c

CHECKER := sudo ./checker/main.py

$(info PWD: $(PWD))
$(info OS_VER: $(OS_VER))
$(info KERNEL_DIR: $(KERNEL_DIR))

$(shell mkdir -p $(BUILD_DIR))

kbuild:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules
	mv $(BUILD_FILES) $(BUILD_DIR)/.

format:
	$(CLANG_FORMAT) $(CLANG_FORMAT_FLAGS) $(FORMAT_FILES)

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -r $(BUILD_DIR)

check:
	$(CHECKER) hwcb

.PHONY: kbuild format clean check
