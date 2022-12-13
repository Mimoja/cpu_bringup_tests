#!/bin/bash

qemu-system-x86_64 -machine q35 -vga std -net none -usb -device usb-tablet \
	-m 1G -cpu host -enable-kvm -nodefaults -smp 16 \
  -serial file:debug.log -monitor stdio -global isa-debugcon.iobase=0x402 \
  -drive if=pflash,format=raw,unit=0,readonly=on,file=/usr/share/ovmf/OVMF.fd \
  -drive file=fat:rw:hd,format=raw \
  -gdb tcp::1234 #-S
