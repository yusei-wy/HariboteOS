OBJS_BOOTPACK = bootpack.o hankaku.o nasmfunc.o mysprintf.o mystrcmp.o graphic.o dsctbl.o int.o fifo.o keyboard.o mouse.o memory.o sheet.o timer.o mtask.o window.o console.o file.o

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

hello.hrb : hello.asm Makefile
	nasm hello.asm -o hello.hrb

hello2.hrb : hello2.asm Makefile
	nasm hello2.asm -o hello2.hrb

bootpack.bin : $(OBJS_BOOTPACK)
	ld -Map bootpack.map -m elf_i386 -e HariMain -n -Tos.ld -static -o bootpack.bin $(OBJS_BOOTPACK)
	hexdump -C bootpack.bin > bootpack.bin.dmp

a.hrb : a.o Makefile
	nasm -g -f elf a_nasm.asm -o a_nasm.o
	ld -m elf_i386 -e HariMain -n -Tapi.ld -static -o a.hrb a.o a_nasm.o

hello3.hrb : hello3.o a_nasm.o Makefile
	ld -m elf_i386 -e HariMain -n -Tapi.ld -static -o hello3.hrb hello3.o a_nasm.o

haribote.sys : asmhead.bin bootpack.bin Makefile
	cat asmhead.bin bootpack.bin > haribote.sys

haribote.img : ipl10.bin haribote.sys hello.hrb hello2.hrb a.hrb hello3.hrb Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::
	mcopy ipl10.asm -i haribote.img ::
	mcopy hello.hrb -i haribote.img ::
	mcopy hello2.hrb -i haribote.img ::
	mcopy a.hrb -i haribote.img ::
	mcopy hello3.hrb -i haribote.img ::
	mcopy test.sh -i haribote.img ::

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
	rm *.lst *.bin *.sys *.img *.hrb *.o *.map
