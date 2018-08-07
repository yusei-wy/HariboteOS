OBJS_BOOTPACK = hankaku.o nasmfunc.o mysprintf.o mystrcmp.o graphic.o dsctbl.o int.o fifo.o keyboard.o mouse.o memory.o sheet.o timer.o mtask.o

default:
	make img

ipl10.bin : ipl10.asm Makefile
	nasm ipl10.asm -o ipl10.bin -l ipl10.lst

asmhead.bin : asmhead.asm Makefile
	nasm asmhead.asm -o asmhead.bin -l asmhead.lst

nasmfunc.o : nasmfunc.asm Makefile		# nasmfunc.asm のバイナリファイル生成
	nasm -g -f elf nasmfunc.asm -o nasmfunc.o

hankaku.c : makefont.c
	gcc -Wall -m32 -fno-pic -o makefont.o makefont.c && ./makefont

%.o: %.c
	gcc -c -m32 -fno-pic -nostdlib -o $*.o $*.c

bootpack.hrb : $(OBJS_BOOTPACK) Makefile		# Cファイルをリンカスクリプトを用いてコンパイル
	gcc -march=i486 -m32 -fno-pic -nostdlib -T har.ld bootpack.c $(OBJS_BOOTPACK) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::
	mcopy ipl10.asm -i haribote.img ::

asm :
	make -r ipl10.bin

img :
	make -r haribote.img

run :
	make img
	qemu-system-i386 -m 32 -fda haribote.img  # "-fda" for floppy disk

debug :
	make img
	qemu-system-i386 -m 32 -fda haribote.img -gdb tcp::10000 -S &

debug2 :
	qemu-system-i386 -m 32 -localtime -vga std -fda haribote.img -gdb tcp::10000 -S &

clean :
	rm *.lst *.bin *.sys *.img *.hrb *.o
