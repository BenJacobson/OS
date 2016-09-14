lcc -g2 src\os345.c -o obj\os345.obj
lcc -g2 src\os345interrupts.c -o obj\os345interrupts.obj
lcc -g2 src\os345semaphores.c -o obj\os345semaphores.obj
lcc -g2 src\os345signals.c -o obj\os345signals.obj
lcc -g2 src\os345tasks.c -o obj\os345tasks.obj
lcc -g2 src\os345p1.c -o obj\os345p1.obj
lcc -g2 src\os345p2.c -o obj\os345p2.obj
lcc -g2 src\os345p3.c -o obj\os345p3.obj
lcc -g2 src\os345park.c -o obj\os345park.obj
lcc -g2 src\os345p4.c -o obj\os345p4.obj
lcc -g2 src\os345lc3.c -o obj\os345lc3.obj
lcc -g2 src\os345mmu.c -o obj\os345mmu.obj
lcc -g2 src\os345p5.c -o obj\os345p5.obj
lcc -g2 src\os345p6.c -o obj\os345p6.obj
lcc -g2 src\os345fat.c -o obj\os345fat.obj

lcclnk obj\os345.obj obj\os345interrupts.obj obj\os345signals.obj obj\os345tasks.obj obj\os345semaphores.obj obj\os345p1.obj obj\os345p2.obj obj\os345p3.obj obj\os345park.obj obj\os345p4.obj obj\os345lc3.obj obj\os345mmu.obj obj\os345p5.obj obj\os345p6.obj obj\os345fat.obj -o os345debug.exe

del obj\*.obj