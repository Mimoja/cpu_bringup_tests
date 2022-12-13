LANG="C"

ARCH := x86_64
TARGET = efi-app-$(ARCH)

GNU_EFI_INC = gnu-efi/inc

BUILD_DIR = build
GEFI_DIR = gnu-efi/lib

CLANG := clang
CC := ${CLANG}

DEBUG_FLAGS = -g3 -O0 -DEFI_DEBUG=1
CFLAGS += -I $(GNU_EFI_INC) $(DEBUG_FLAGS) -Wall \
	-fno-strict-aliasing -fno-stack-protector -Wno-pointer-sign \
		-target x86_64-unknown-windows \
        -ffreestanding \
        -fshort-wchar \
        -mno-red-zone

LDFLAGS += -nostdlib $(DEBUG_FLAGS) \
		-g \
        -target x86_64-unknown-windows \
        -nostdlib \
        -Wl,-entry:efi_main \
        -Wl,-subsystem:efi_application \
        -fuse-ld=lld-link

SRCS = $(wildcard *.S *.c ) $(wildcard gnu-efi/lib/*.c ) $(wildcard gnu-efi/lib/runtime/*.c ) $(wildcard gnu-efi/lib/$(ARCH)/*.c )

OBJS:=$(SRCS:.c=.o)

.PHONY: all

all: $(BUILD_DIR) $(GEFI_DIR) $(BUILD_DIR)/main.efi

$(BUILD_DIR):
	if [ ! -d $@ ]; then mkdir $@; fi

$(GEFI_DIR): $(BUILD_DIR)
	if [ ! -d $@ ]; then mkdir -p $@; fi
	if [ ! -d $@/x86_64 ]; then mkdir -p $@/x86_64; fi
	if [ ! -d $@/runtime ]; then mkdir -p $@/runtime; fi

$(BUILD_DIR)/%.efi: $(OBJS)
	clang $(LDFLAGS) $?  -o $@ 
	
$(BUILD_DIR)/%.o: %.c
	clang $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	clang $(CFLAGS) -fno-merge-constants  -c $< -o $@

run: all
	./efi-x86_64.sh

clean:
	$(RM) -rf $(BUILD_DIR)

