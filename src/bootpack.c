#include "bootpack.h"
#include <stdio.h>

void make_window8(unsigned char *buf, int xsize, int ysize, char *title){
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@",
    };
    int x, y;
    char c;
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
    for (y = 0; y < 14; y++){
        for (x = 0; x < 16; x++){
            c = closebtn[y][x];
            if      (c == '@'){c = COL8_000000;}
            else if (c == '$'){c = COL8_848484;}
            else if (c == 'Q'){c = COL8_C6C6C6;}
            else              {c = COL8_FFFFFF;}
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

void HariMain(void){
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];
    int mx, my, i;
    struct MOUSE_DECODE mdec;

    unsigned int memtotal, count = 0;
    struct MEMMNGR *memmngr = (struct MEMMNGR *) MEMMNGR_ADDR;

    struct SHTMNGR *shtmngr;
    struct SHEET *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    init_gdtidt();
    init_pic();

    // IDT, PIC の初期化が終わったので CPU の割り込み禁止を解除
    io_sti();

    // fifo バッファの初期化
    // &keyfifo : fifo バッファ用構造体の番地
    // 32       : サイズ
    // keybuf   : バッファの番地
    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    // デバイスに電気信号を送る
    // PICO_IMR : デバイス番号
    // 0xf9     : データ
    io_out8(PIC0_IMR, 0xf9);    // PIC1 とキーボードを許可（11111001）
    io_out8(PIC1_IMR, 0xef);    // マウスを許可（11101111）

    init_keyboard();
    enable_mouse(&mdec);

    memtotal = memtest(0x00400000, 0xbfffffff);
    memmngr_init(memmngr);
    memmngr_free(memmngr, 0x00001000, 0x0009e000);
    memmngr_free(memmngr, 0x00400000, memtotal - 0x00400000);

    // 画面描画
    init_palette();
    shtmngr   = shtmngr_init(memmngr, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back  = sheet_alloc(shtmngr);
    sht_mouse = sheet_alloc(shtmngr);
    sht_win   = sheet_alloc(shtmngr);

    buf_back  = (unsigned char *) memmngr_alloc_4k(memmngr, binfo->scrnx * binfo->scrny);
    buf_win   = (unsigned char *) memmngr_alloc_4k(memmngr, 160 * 52);

    sheet_setbuf(sht_back,  buf_back,  binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    sheet_setbuf(sht_win,   buf_win,   160, 52, -1);

    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 52, "counter");

    // putfonts8_asc(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    // putfonts8_asc(buf_win, 160, 24, 44, COL8_000000, "  Haribote-OS!");

    mx = (binfo->scrnx - 16) / 2;       // 画面中央になるように
    my = (binfo->scrny - 28 - 16) / 2;  // 座標を計算する

    sheet_slide(sht_back,  0,  0 );
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win,   80, 72);

    sheet_updown(sht_back,  0);
    sheet_updown(sht_win,   1);
    sheet_updown(sht_mouse, 2);

    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    sprintf(s, "memory %dMB     free : %dkB", memtotal / (1024 * 1024), memmngr_total(memmngr) / 1024);
    putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);

    for(;;){
        count++;
        sprintf(s, "%010d", count);
        boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
        putfonts8_asc(buf_win, 160, 40, 28, COL8_000000, s);
        sheet_refresh(sht_win, 40, 28, 120, 44);

        // 割り込み禁止
        io_cli();
        if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0){
            // io_stihlt();
            io_sti();
        }
        else{
            if(fifo8_status(&keyfifo) != 0){
                i = fifo8_get(&keyfifo);
                io_sti();

                sprintf(s, "%02X", i);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);

                sheet_refresh(sht_back, 0, 16, 16, 32);
            }
            else if(fifo8_status(&mousefifo) != 0){
                i = fifo8_get(&mousefifo);
                io_sti();

                if(mouse_decode(&mdec, i) != 0){
                    // mdec->phase == 3 のときに mouse_decode() は 1 を返す
                    // つまりデータが 3 バイト揃ったので以下を処理する
                    // sprintf(s, "%02X %02X %02X", mdec.buf[0], mdec.buf[1], mdec.buf[2]);
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if((mdec.btn & 0x01) != 0){
                        s[1] = 'L';
                    }
                    if((mdec.btn & 0x02) != 0){
                        s[3] = 'R';
                    }
                    if((mdec.btn & 0x04) != 0){
                        s[2] = 'C';
                    }
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
                    putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                    sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);

                    // カーソルの移動
                    mx += mdec.x;
                    my += mdec.y;
                    if(mx < 0){
                        mx = 0;
                    }
                    if(my < 0){
                        my = 0;
                    }
                    if(mx > binfo->scrnx - 1){
                        mx = binfo->scrnx - 1;
                    }
                    if(my > binfo->scrny - 1){
                        my = binfo->scrny - 1;
                    }
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);  // 座標を消す
                    putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);  // 座標を書く
                    sheet_refresh(sht_back, 0, 0, 80, 16);
                    sheet_slide(sht_mouse, mx, my);
                }
            }
        }
    }
}
