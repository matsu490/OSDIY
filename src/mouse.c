#include "bootpack.h"
#include <stdio.h>

struct FIFO8 mousefifo;

// マウスの有効化
void enable_mouse(struct MOUSE_DECODE *mdec){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // うまくいくと ACK (0xfa) が送信される
    mdec->phase = 0;
    return;
}

int mouse_decode(struct MOUSE_DECODE *mdec, unsigned char dat){
    // 初期段階
    // マウスが有効化されたら次の段階に移行する
    if(mdec->phase == 0){
        if(dat == 0xfa)         // 0xfa ということはマウスが有効化されている
            mdec->phase = 1;
        return 0;
    }
    // マウスの 1 バイト目を待っている段階
    if(mdec->phase == 1){
        if((dat & 0xc8) == 0x08){  // 正しい 1 バイト目かチェックする
            // dat の上位桁は 0x0 ~ 0x3
            // dat の下位桁は 0x8 ~ 0xf である必要がある
            // 0xc (1100) との & が 0x0 (0000) ならば dat の上位桁は 0x0 ~ 0x3 (0000 ~ 0011)
            // 0x8 (1000) との & が 0x8 (1000) ならば dat の上位桁は 0x8 ~ 0xf (1000 ~ 1111)
            mdec->buf[0] = dat;    // 単一クリックとクリックの組み合わせで変化する
            mdec->phase = 2;
        }
        return 0;
    }
    // マウスの 2 バイト目を待っている段階
    if(mdec->phase == 2){
        mdec->buf[1] = dat;    // 横移動で変化する
        mdec->phase = 3;
        return 0;
    }
    // マウスの 3 バイト目を待っている段階
    if(mdec->phase == 3){
        mdec->buf[2] = dat;    // 縦移動で変化する
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;    // buf[0] の下位 3 桁にあるボタンの状態を取り出す
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if((mdec->buf[0] & 0x10) != 0)
            mdec->x |= 0xffffff00;
        if((mdec->buf[0] & 0x20) != 0)
            mdec->y |= 0xffffff00;
        mdec->y = -mdec->y;     // マウスでは y 方向の符号が画面と反対
        return 1;
    }
    return -1;      // ここにくることはないはず
}

// マウスからの割り込み
void inthandler2c(int *esp){
    unsigned char data;

    // IRQ-12 受付完了を PIC1 に通知する
    io_out8(PIC1_OCW2, 0x64);

    // IRQ-02 受付完了を PIC0 に通知する
    io_out8(PIC0_OCW2, 0x62);

    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}
