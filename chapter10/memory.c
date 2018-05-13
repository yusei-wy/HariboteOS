#include "bootpack.h"

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000

/**
 * メモリチェックの間だけキャッシュを OFF にする
 */
unsigned int memtest(unsigned int start, unsigned int end) {
  char flg486 = 0;
  unsigned int eflg, cr0, i;

  // 386か486以降なのか？
  eflg = io_load_eflags();
  eflg |= EFLAGS_AC_BIT; // AC-bit = 1
  io_store_eflags(eflg);
  eflg = io_load_eflags();
  if ((eflg & EFLAGS_AC_BIT) != 0) // 386では AC=1 にしても自動で0に戻ってしまう
    flg486 = 1;
  eflg &= ~EFLAGS_AC_BIT; // AC-bit = 0
  io_store_eflags(eflg);

  if (flg486 != 0) {
    cr0 = load_cr0();
    cr0 |= CR0_CACHE_DISABLE; // キャッシュ禁止
    store_cr0(cr0);
  }

  i = memtest_sub(start, end);

  if (flg486 != 0) {
    cr0 = load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE; // キャッシュ許可
    store_cr0(cr0);
  }

  return i;
}

/**
 * memman を初期化して空に設定する
 */
void memman_init(struct MEMMAN *man) {
  man->frees = 0;     // 空き情報の個数
  man->maxfrees = 0;  // 状況観察用 : frees の最大値
  man->lostsize = 0;  // 解放に失敗した合計サイズ
  man->losts = 0;     // 解放に失敗した回数
  return;
}

/**
 * 空きメモリサイズの合計を計算する
 */
unsigned int memman_total(struct MEMMAN *man) {
  unsigned int i, t = 0;
  for (i = 0; i < man->frees; i++)
    t += man->free[i].size;
  return t;
}

/**
 * 指定したサイズの空きメモリを確保する
 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
  unsigned int i, a;
  for (i = 0; i < man->frees; i++) {
    if (man->free[i].size >= size) {
      // 十分な広さの空きを発見
      a = man->free[i].addr;
      man->free[i].addr += size;
      man->free[i].size -= size;
      if (man->free[i].size == 0) {
        // free[i] がなくなったので前へつめる
        man->frees--;
        for (; i < man->frees; i++)
          man->free[i] = man->free[i + 1];  // 構造体の代入
      }
      return a;
    }
  }
  return 0; // 空きがない
}

/**
 * 空きメモリを開放する
 */
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
  int i, j;
  // まとめやすさを考えると free[] や addr 順に並んでいる方がいい
  // だからまずどこに入れるべきかを決める
  for (i = 0; i < man->frees; i++) {
    if (man->free[i].addr > addr)
      break;
  }
  // free[i - 1].addr < addr < free[i].addr
  if (i > 0) {
    // 前がある
    if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
      // 前の空き領域にまとめられる
      man->free[i - 1].size += size;
      if (i < man->frees) {
        // 後ろもある
        if (addr + size == man->free[i].addr) {
          // man->free[i] の削除
          // free[i] がなくなったので前へ詰める
          man->frees--;
          for (; i < man->frees; i++)
            man->free[i] = man->free[i + 1];  // 構造体の代入
        }
      }
      return 0; // 成功終了
    }
  }
  // 前とはまとめられなかった
  if (i < man->frees) {
    // 後ろがある
    if (addr + size == man->free[i].addr) {
      // 後ろとはまとめられる
      man->free[i].addr = addr;
      man->free[i].size += size;
      return 0; // 成功終了
    }
  }
  // 前にも後ろにもまとめられれない
  if (man->frees < MEMMAN_FREES) {
    // free[i] より後ろを後ろへずらして隙間を作る
    for (j = man->frees; j > i; j--) 
      man->free[j] = man->free[j - 1];
    man->frees++;
    if (man->maxfrees < man->frees)
      man->maxfrees = man->frees; // 最大値を更新
    man->free[i].addr = addr;
    man->free[i].size = size;
    return 0; // 成功終了
  }
  // 後ろにずらせなかった
  man->losts++;
  man->lostsize += size;
  return -1;  // 失敗終了
}

/**
 * 0x1000 バイト単位でメモリを確保する
 */
unsigned int meman_alloc_4k(struct MEMMAN *man, unsigned int size) {
  unsigned int a;
  size = (size + 0xfff) & 0xfffff0000;
  a = memman_alloc(man, size);
  return a;
}

/**
 * 0x1000 バイト単位でメモリを解放する
 */
unsigned int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
  int i;
  size = (size + 0xfff) & 0xfffff0000;
  i = memman_free(man, addr, size);
  return i;
}
