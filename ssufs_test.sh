#!/bin/bash

gcc -o -g ssufs_test ssufs_test3.c ssufs-ops.c ssufs-disk.c
./ssufs_test

rm -f ssufs
rm -f ssufs_test

