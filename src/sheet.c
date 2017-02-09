#include "bootpack.h"

struct SHTMNGR *shtmngr_init(struct MEMMNGR *memmngr, unsigned char *vram, int scrn_x, int scrn_y){
    struct SHTMNGR *mngr;
    int i;
    mngr = (struct SHTMNGR *) memmngr_alloc_4k(memmngr, sizeof(struct SHTMNGR));
    if (mngr == 0){
        goto err;
    }
    mngr->vram = vram;
    mngr->scrn_x = scrn_x;
    mngr->scrn_y = scrn_y;
    mngr->top = -1;      // 下敷きは 1 枚もない
    for (i = 0; i < MAX_SHEETS; i++){
        mngr->sheets0[i].flags = 0;     // 未使用マーク
        mngr->sheets0[i].mngr = mngr;
    }
err:
    return mngr;
}

// ==============================================
// === 新規に未使用の下敷きをもらってくる関数 ===
// ==============================================
struct SHEET *sheet_alloc(struct SHTMNGR *mngr){
    struct SHEET *sht;
    int i;
    // 一番最初に見つかった未使用下敷きを使用中にして返す
    for (i = 0; i < MAX_SHEETS; i++){
        if (mngr->sheets0[i].flags == 0){
            sht = &(mngr->sheets0[i]);
            sht->flags = SHEET_USE;     // 使用中のマーク
            sht->height = -1;           // 非表示中
            return sht;
        }
    }
    return 0;       // すべての下敷きが使用中だった
}

// ==============================================
// === 下敷きのバッファや大きさ、透明色を設定する
// ==============================================
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int pxl_x, int pxl_y, int col_inv){
    sht->buf = buf;
    sht->pxl_x = pxl_x;
    sht->pxl_y = pxl_y;
    sht->col_inv = col_inv;
    return;
}

// ==============================================
// === 下敷きの高さを設定する ===
// ==============================================
void sheet_updown(struct SHEET *sht, int height){
    struct SHTMNGR *mngr = sht->mngr;
    int h, old = sht->height;       // 設定前の高さをバッファしとく

    if (height > mngr->top + 1){    // 指定が低すぎや高すぎだったら修正する
        height = mngr->top + 1;
    }
    if (height < -1){
        height = -1;
    }
    sht->height = height;   // 高さを設定する

    if (old > height){
        if (height >= 0){
            for (h = old; h > height; h--){
                mngr->sheets[h] = mngr->sheets[h - 1];
                mngr->sheets[h]->height = h;
            }
            mngr->sheets[height] = sht;
            sheet_refresh_sub(mngr, sht->vx0, sht->vy0, sht->vx0 + sht->pxl_x, sht->vy0 + sht->pxl_y, height + 1);
        }
        else{
            if (mngr->top > old){
                for (h = old; h < mngr->top; h++){
                    mngr->sheets[h] = mngr->sheets[h + 1];
                    mngr->sheets[h]->height = h;
                }
            }
            mngr->top--;    // 表示中の下敷きが 1 つ減るので一番上の高さが減る
            sheet_refresh_sub(mngr, sht->vx0, sht->vy0, sht->vx0 + sht->pxl_x, sht->vy0 + sht->pxl_y, 0);    // 新しい下敷き情報に従い画面を描き直す
        }
    }
    else if (old < height){    // 以前よりも高くなる
        if (old >= 0){
            // 間のものを押し上げる
            for (h = old; h < height; h++){
                mngr->sheets[h] = mngr->sheets[h + 1];
                mngr->sheets[h]->height = h;
            }
            mngr->sheets[height] = sht;
        }
        else{      // 非表示状態から表示状態へ
            // 上になるものを持ち上げる
            for (h = mngr->top; h >= height; h--){
                mngr->sheets[h + 1] = mngr->sheets[h];
                mngr->sheets[h + 1]->height = h + 1;
            }
            mngr->sheets[height] = sht;
            mngr->top++;    // 表示中の下敷きが 1 つ増えるので一番上の高さが増える
        }
        sheet_refresh_sub(mngr, sht->vx0, sht->vy0, sht->vx0 + sht->pxl_x, sht->vy0 + sht->pxl_y, height);
    }
    return;
}

// 高さの低い順に描画していく
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1){
    if (sht->height >= 0){
        sheet_refresh_sub(sht->mngr, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height);
    }
    return;
}

// 高さの低い順に描画していく
// ただしスクリーン上の描画範囲を指定する
//         vx0 : 描画範囲の始点
//         vy0 : 描画範囲の始点
//         vx1 : 描画範囲の終点
//         vy1 : 描画範囲の終点
// height_thre : この高さよりも上の下敷きのみ再描画する
void sheet_refresh_sub(struct SHTMNGR *mngr, int vx0, int vy0, int vx1, int vy1, int height_thre){
    int h;
    int bx, by;     // 下敷きの横、縦の絶対値
    int bx0, by0;
    int bx1, by1;
    int vx, vy;     // 下敷きの横、縦のスクリーン上の相対値
    unsigned char *buf;
    unsigned char c;
    unsigned char *vram = mngr->vram;
    struct SHEET *sht;

    if (vx0 < 0){
        vx0 = 0;
    }
    if (vy0 < 0){
        vy0 = 0;
    }
    if (vx1 > mngr->scrn_x){
        vx1 = mngr->scrn_x;
    }
    if (vy1 > mngr->scrn_y){
        vy1 = mngr->scrn_y;
    }
    for (h = height_thre; h <= mngr->top; h++){
        sht = mngr->sheets[h];
        buf = sht->buf;

        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;

        if (bx0 < 0){
            bx0 = 0;
        }
        if (by0 < 0){
            by0 = 0;
        }
        if (bx1 > sht->pxl_x){
            bx1 = sht->pxl_x;
        }
        if (by1 > sht->pxl_y){
            by1 = sht->pxl_y;
        }

        for (by = 0; by < sht->pxl_y; by++){
            vy = sht->vy0 + by;

            for (bx = 0; bx < sht->pxl_x; bx++){
                vx = sht->vx0 + bx;

                if (vx0 <= vx && vx < vx1 && vy0 <= vy && vy < vy1){
                    c = buf[by * sht->pxl_x + bx];
                    if (c != sht->col_inv){
                        vram[vy * mngr->scrn_x + vx] = c;
                    }
                }
            }
        }
    }
    return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0){
    int old_vx0 = sht->vx0;
    int old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0){  // もし表示中なら新しい下敷きの情報に沿って画面を書き直す
        sheet_refresh_sub(sht->mngr, old_vx0, old_vy0, old_vx0 + sht->pxl_x, old_vy0 + sht->pxl_y, 0);
        sheet_refresh_sub(sht->mngr, vx0, vy0, vx0 + sht->pxl_x, vy0 + sht->pxl_y, sht->height);
    }
    return;
}

void sheet_free(struct SHEET *sht){
    // 表示中ならまず非表示にする
    if (sht->height >= 0){
        sheet_updown(sht, -1);
    }
    sht->flags = 0;     // 未使用マーク
    return;
}
