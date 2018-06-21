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
  timerctl.next_time = 0xffffffff; // 最初は作動中のタイマがないので
  timerctl.using = 0;
  for (i = 0; i < MAX_TIMER; i++) {
    timerctl.timers0[i].flags = 0;  // 未使用
  }
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

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, unsigned char data) {
  timer->fifo = fifo;
  timer->data = data;
  return;
} 

void timer_settime(struct TIMER *timer, unsigned int timeout) {
  int e;
  struct TIMER *t, *s;
  timer->timeout = timeout + timerctl.count;
  timer->flags = TIMER_FLAGS_USING;
  e = io_load_eflags();
  io_cli();
  timerctl.using++;
  if (timerctl.using == 1) {
    // 動作中のタイマはコレ一つになる場合
    timerctl.t0 = timer;
    timer->next = 0;  // 次はない
    timerctl.next_time = timer->timeout;
    io_store_eflags(e);
    return;
  }
  t = timerctl.t0;
  if (timer->timeout <= t->timeout) {
    // 先頭に入れる場合
    timerctl.t0 = timer;
    timer->next = t;  // 次は t
    timerctl.next_time = timer->timeout;
    io_store_eflags(e);
    return;
  }
  // どこに入れればいいか探す
  for (;;) {
    s = t;
    t = t->next;
    if (t == 0)
      break;  // 一番うしろになった
    if (timer->timeout <= t->timeout) {
      // s と t の間に入れる場合
      s->next = timer;  // s の 次は timer
      timer->next = t;  // timer の次は t
      io_store_eflags(e);
      return;
    }
  }
  // 一番うしろに入れる場合
  s->next = timer;
  timer->next = 0;
  io_store_eflags(e);
  return;
}

void inthandler20(int *esp) {
  int i;
  struct TIMER *timer;
  io_out8(PIC0_OCW2, 0x60); // IRQ-00 受付完了を PIC に通知
  timerctl.count++;
  if (timerctl.next_time > timerctl.count)
    return; // まだ次の時刻になっていないのでおしまい
  for (i = 0; i < timerctl.using; i++) {
    // timers のタイマは全て作動中のものなので flags を確認しない
    if (timer->timeout > timerctl.count)
      break;
    // タイムアウト
    timer->flags = TIMER_FLAGS_ALLOC;
    fifo32_put(timer->fifo, timer->data);
    timer = timer->next;  // 次のタイマの番地を timer にセット
  }
  // ちょうど i 個目のタイマがタイムアウトした. のこりをずらす
  timerctl.using -= i;

  // 新しいずらし
  timerctl.t0 = timer;

  // timerctl.next の設定
  if (timerctl.using > 0)
    timerctl.next_time = timerctl.t0->timeout;
  else
    timerctl.next_time = 0xffffffff;
  return;
}
