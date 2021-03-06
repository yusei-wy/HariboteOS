#include "bootpack.h"

#define PIT_CTRL  0x0043
#define PIT_CNT0  0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC   1 // 確保した状態
#define TIMER_FLAGS_USING   2 // タイマ作動中

/**
 * タイマーの初期化. 100ms ごとに呼び出される
 */
void init_pit(void) {
  int i;
  io_out8(PIT_CTRL, 0x34);
  io_out8(PIT_CNT0, 0x9c);
  io_out8(PIT_CNT0, 0x2e);
  timerctl.count = 0;
  timerctl.next = 0xffffffff; // 最初は作動中のタイマがないので
  timerctl._using = 0;
  for (i = 0; i < MAX_TIMER; i++)
    timerctl.timers0[i].flags = 0;  // 未使用
  return;
}

struct TIMER *timer_alloc(void) {
  int i;
  for (i = 0; i < MAX_TIMER; i++) {
    if (timerctl.timers0[i].flags == 0) {
      timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
      return &timerctl.timers0[i];
    }
  }
  return 0; // 見つからなかった
}

void timer_free(struct TIMER *timer) {
  timer->flags = 0; // 未使用
  return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data) {
  timer->fifo = fifo;
  timer->data = data;
  return;
} 

void timer_settime(struct TIMER *timer, unsigned int timeout) {
  int e, i, j;
  timer->timeout = timeout + timerctl.count;
  timer->flags = TIMER_FLAGS_USING;
  e = io_load_eflags();
  io_cli();
  // どこに入ればいいかを探す
  for (i = 0; i < timerctl._using; i++) {
    if (timerctl.timers[i]->timeout >= timer->timeout)
      break;
  }
  timerctl._using++;
  // 後ろをずらす
  for (j = 0; j < timerctl._using; j++) 
    timerctl.timers[j] = timerctl.timers[j - 1];
  // 空いた隙間に入れる
  timerctl.timers[i] = timer;
  timerctl.next = timerctl.timers[0]->timeout;
  io_store_eflags(e);
  return;
}

void inthandler20(int *esp) {
  int i, j;
  io_out8(PIC0_OCW2, 0x60); // IRQ-00 受付完了を PIC に通知
  timerctl.count++;
  if (timerctl.next > timerctl.count)
    return; // まだ次の時刻になっていないのでおしまい
  for (i = 0; i < timerctl._using; i++) {
    // timers のタイマは全て作動中のものなので flags を確認しない
    if (timerctl.timers[i]->timeout > timerctl.count)
      break;
    // タイムアウト
    timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
    fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
  }
  // ちょうど i 個目のタイマがタイムアウトした. のこりをずらす
  timerctl._using -= i;
  for (j = 0; j < timerctl._using; j++) 
    timerctl.timers[j] = timerctl.timers[i + j];
  if (timerctl._using > 0)
    timerctl.next = timerctl.timers[0]->timeout;
  else
    timerctl.next = 0xffffffff;
  return;
}
