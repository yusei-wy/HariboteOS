/*
 * 割り込み関係
 */

#include "bootpack.h"
#include <stdio.h>

/**
 * PIC の初期化
 */
void init_pic(void) {
  io_out8(PIC0_IMR, 0xff);  // 全ての割り込みを受け付けない
  io_out8(PIC1_IMR, 0xff);  // 全ての割り込みを受け付けない

  io_out8(PIC0_ICW1, 0x11); // エッジトリガモード
  io_out8(PIC0_ICW2, 0x20); // IRQ0-7 は INT20-27 で受け付ける
  io_out8(PIC0_ICW3, 1 << 2); // PIC1 は IRQ2 にて接続
  io_out8(PIC0_ICW4, 0x01); // ノンバッファモード

  io_out8(PIC1_ICW1, 0x11); // エッジトリガモード
  io_out8(PIC1_ICW2, 0x28); // IRQ8-15 は INT28-2f で受け付ける
  io_out8(PIC1_ICW3, 3);    // PIC1 は IRQ2 にて接続
  io_out8(PIC1_ICW4, 0x01); // ノンバッファモード

  io_out8(PIC0_IMR, 0xfb);  // 11111011 PIC1 以外は全て禁止
  io_out8(PIC1_IMR, 0xff);  // 11111111 全ての割り込みを受け付けない

  return;
}

struct KEYBUF keybuf;

#define PORT_KEYDAY   0x0060

/**
 * PS/2 キーボードからの割り込み
 */
void inthandler21(int *esp) {
  unsigned char data;
  io_out8(PIC0_OCW2, 0x61); // IRQ-01 受付完了を PIC に通知
  data = io_in8(PORT_KEYDAY);
  if (keybuf.len < 32) {
    keybuf.data[keybuf.next_w] = data;
    keybuf.len++;
    keybuf.next_w++;
    if (keybuf.next_w == 32) 
      keybuf.next_w = 0;
  }
  return;
}

/**
 * PS/2 マウスからの割り込み
 */
void inthandler2c(int *esp) {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32*8 - 1, 15);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
  for (;;) {
    io_hlt();
  }
}
