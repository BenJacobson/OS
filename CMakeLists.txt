cmake_minimum_required(VERSION 3.6)
project(OS C)

set(SOURCE_FILES src/os345.c src/os345.h src/os345config.h src/os345fat.c src/os345fat.h src/os345interrupts.c src/os345lc3.c src/os345lc3.h src/os345mmu.c src/os345p1.c src/os345p2.c src/os345p3.c src/os345p4.c src/os345p5.c src/os345p6.c src/os345park.c src/os345park.h src/os345semaphores.c src/os345signals.c src/os345signals.h src/os345tasks.c)
add_executable(OS ${SOURCE_FILES})