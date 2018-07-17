/*
 * マルチタスク関係
 */
#include "bootpack.h"

struct TIMER *mt_timer;
int mt_tr;

/**
 * mt_timer と mt_tr(trレジスタの代わりのようなもの) の初期化
 */
void mt_init(void) {
  mt_timer = timer_alloc();
  // timer_init は必要ないのでやらない
  timer_settime(mt_timer, 2);
  mt_tr = 3 * 8;
  return;
}

/**
 * mt_tr の値をもとに次の mt_tr の値を計算してタスクをスイッチする
 */
void mt_taskswitch(void) {
  if (mt_tr == 3 * 8) 
    mt_tr = 4 * 8;
  else
    mt_tr = 3 * 8;
  timer_settime(mt_timer, 2);
  farjmp(0, mt_tr);
  return;
}
