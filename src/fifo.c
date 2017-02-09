/* FIFO ライブラリ */
#include "bootpack.h"
#define FLAGS_OVERRUN   0x0001

/* FIFO バッファの初期化 */
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf){
    fifo->size = size;
    fifo->buf = buf;    /* バッファの番地 */
    fifo->free = size;  /* 現在何バイト空いているか */
    fifo->flags = 0;    /* あふれたことを知らせるフラグ */
    fifo->p = 0;        /* 書き込み位置 */
    fifo->q = 0;        /* 読み込み位置 */
    return;
}

/* FIFO バッファへ 1 バイトのデータ記録する */
int fifo8_put(struct FIFO8 *fifo, unsigned char data){
    /* 空きがなくてあふれた */
    if(fifo->free == 0){
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if(fifo->p == fifo->size){
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

/* FIFO から 1 バイトのデータをもらってくる */
int fifo8_get(struct FIFO8 *fifo){
    int data;

    /* バッファが空の時は -1 を返す */
    if(fifo->free == fifo->size){
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if(fifo->q == fifo->size){
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

/* どのくらいデータが溜まっているかを返す */
int fifo8_status(struct FIFO8 *fifo){
    return fifo->size - fifo->free;
}
