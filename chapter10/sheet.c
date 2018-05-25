/*
 * マウスやウィンドウの重ね合わせ処理
 */

#include "bootpack.h"

#define SHEET_USE 1

/**
 * シートの初期化
 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
  struct SHTCTL *ctl;
  int i;
  ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
  if (ctl == 0)
    goto err;
  ctl->vram = vram;
  ctl->xsize = xsize;
  ctl->ysize = ysize;
  ctl->top = -1;  // シートは１枚もない
  for (i = 0; i < MAX_SHEETS; i++)
    ctl->sheets0[i].flags = 0;  // 未使用マーク
err:
  return ctl;
}

/**
 * 新規に未使用のシートをもらいにいく
 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
  struct SHEET *sht;
  int i;
  for (i = 0; i < MAX_SHEETS; i++) {
    if (ctl->sheets0[i].flags == 0) {
      sht = &ctl->sheets0[i];
      sht->flags = SHEET_USE; // 使用中マーク
      sht->height = -1; // 非表示中
      return sht;
    }
  }
  return 0; // 全てのシートが使用中だった
}

/**
 * シートのバッファの大きさや透明色を設定する
 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
  sht->buf = buf;
  sht->bxsize = xsize;
  sht->bysize = ysize;
  sht->col_inv = col_inv;
  return;
}

/**
 * 指定した範囲のみシートをリフレッシュする
 */
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1) {
  int h, bx, by, vx,vy;
  unsigned char *buf, c, *vram = ctl->vram;
  struct SHEET *sht;
  for (h = 0; h <= ctl->top; h++) {
    sht = ctl->sheets[h];
    buf = sht->buf;
    for (by = 0; by < sht->bysize; by++) {
      vy = sht->vy0 + by;
      for (bx = 0; bx < sht->bxsize; bx++) {
        vx = sht->vx0 + bx;
        if (vx0 <= vx && vx < vx1 && vy0 <= vy && vy < vy1) {
          c = buf[by*sht->bxsize + bx];
          if (c != sht->col_inv)
            vram[vy*ctl->xsize + vx] = c;
        }
      }
    }
  }
  return;
}

/**
 * シートの高さを設定する
 */
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height) {
  int h, old = sht->height; // 設定前の高さを記憶する

  // 指定が低すぎや高すぎだったら修正する
  if (height > ctl->top + 1) 
    height = ctl->top + 1;
  if (height < -1)
    height = -1;
  sht->height = height; // 高さを設定

  // 以下は主に sheets[] の並び替え
  if (old > height) { // 以前よりも低くなる
    if (height >= 0) {
      // 間のものを引き上げる
      for (h = old; h > height; h--)  {
        ctl->sheets[h] = ctl->sheets[h - 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else { // 非表示化
      if (ctl->top > old)  {
        // 上になっているものを下ろす
        for (h = old; h < ctl->top; h++) {
          ctl->sheets[h] = ctl->sheets[h + 1];
          ctl->sheets[h]->height = h;
        }
      }
      ctl->top--; // 非表示中の下敷きが１つ減るので一番上の高さが減る
    }
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
  } else if (old < height) {  // 以前よりも高くなる
    if (old >= 0) {
      // 間のものを押し下げる
      for (h = old; h < height; h++) {
        ctl->sheets[h] = ctl->sheets[h + 1];
        ctl->sheets[h]->height = h;
      }
      ctl->sheets[height] = sht;
    } else {  // 非表示状態から表示状態へ上になるものを持ち上げる
      // 上になっているものを持ち上げる
      for (h = ctl->top; h >= height; h--) {
        ctl->sheets[h + 1] = ctl->sheets[h];
        ctl->sheets[h + 1]->height = h + 1;
      }
      ctl->sheets[height] = sht;
      ctl->top++; // 表示中の下敷きが１つ増えるので一番上の高さが増える
    }
    sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
  }
  return;
} 

/**
 * 画面をリフレッシュ
 */
void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
  if (sht->height >= 0) // もしも表示中なら新しい下敷きの情報に沿って画面を顔着直す
    sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
  return;
}

/**
 * 下敷きを上下左右に動かす
 */
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
  int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
  sht->vx0 = vx0;
  sht->vy0 = vy0;
  if (sht->height >= 0) { // もしも表示中なら
    sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
    sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
  }
  return;
}

/**
 * 使い終わった下敷きを解放
 */
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht) {
  if (sht->height >= 0)
    sheet_updown(ctl, sht, -1); // 表示中ならまず非表示にする
  sht->flags = 0; // 未使用マーク
  return;
}
