#!/usr/bin/env bash

#gcc -DUSERMODE -DUBUILD_MODNAME=\"pthread\" ./trace.c ./utils.c ./pthread.c -o pthread -I../inc
gcc -DUSERMODE -DUBUILD_MODNAME=\"pthread\" ./trace.c ./utils.c ./pthread.c -o pthread_2.11 -I../inc -lpthread -L/usr/toolchain/x86_64-pc-linux-gnu_gcc7.3.0_glibc2.11/x86_64-pc-linux-gnu/lib #-Wl,--dynamic-linker=/usr/toolchain/x86_64-pc-linux-gnu_gcc7.3.0_glibc2.11/x86_64-pc-linux-gnu/lib/ld-linux-x86-64.so.2
