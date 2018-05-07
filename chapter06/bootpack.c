#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyinfo;

void HariMain(void) {
  struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
  char s[40], mcursor[256], keybuf[32];
  int mx, my, i;

  init_gdtidt();
  init_pic();
  io_sti(); // IDT/PIC の初期化が終わったので CPU の割り込み禁止を解除

  fifo8_init(&keyinfo, 32, keybuf);
  io_out8(PIC0_IMR, 0xf9);  // PIC1 とキーボードを許可
  io_out8(PIC1_IMR, 0xef);  // マウスを許可(11101111)

  init_palette(); // パレットを設定
  init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
  mx = (binfo->scrnx - 16)/2; // 画面中央になるように座標を計算
  my = (binfo->scrny - 28 - 16)/2;
  init_mouse_cursor8(mcursor, COL8_008484);
  putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
  sprintf(s, "(%d, %d)", mx, my);
  putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

  for (;;) {
    io_cli(); // 割り込み禁止
    if (fifo8_status(&keyinfo) == 0) {
      io_stihlt();
    } else {
      i = fifo8_get(&keyinfo);
      io_sti();
      sprintf(s, "%x", i);
      boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
      putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
    }
    io_hlt();
  }
}
