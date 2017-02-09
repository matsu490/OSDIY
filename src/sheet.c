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
    mngr->top = -1;      // ���~���� 1 �����Ȃ�
    for (i = 0; i < MAX_SHEETS; i++){
        mngr->sheets0[i].flags = 0;     // ���g�p�}�[�N
        mngr->sheets0[i].mngr = mngr;
    }
err:
    return mngr;
}

// ==============================================
// === �V�K�ɖ��g�p�̉��~����������Ă���֐� ===
// ==============================================
struct SHEET *sheet_alloc(struct SHTMNGR *mngr){
    struct SHEET *sht;
    int i;
    // ��ԍŏ��Ɍ����������g�p���~�����g�p���ɂ��ĕԂ�
    for (i = 0; i < MAX_SHEETS; i++){
        if (mngr->sheets0[i].flags == 0){
            sht = &(mngr->sheets0[i]);
            sht->flags = SHEET_USE;     // �g�p���̃}�[�N
            sht->height = -1;           // ��\����
            return sht;
        }
    }
    return 0;       // ���ׂẲ��~�����g�p��������
}

// ==============================================
// === ���~���̃o�b�t�@��傫���A�����F��ݒ肷��
// ==============================================
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int pxl_x, int pxl_y, int col_inv){
    sht->buf = buf;
    sht->pxl_x = pxl_x;
    sht->pxl_y = pxl_y;
    sht->col_inv = col_inv;
    return;
}

// ==============================================
// === ���~���̍�����ݒ肷�� ===
// ==============================================
void sheet_updown(struct SHEET *sht, int height){
    struct SHTMNGR *mngr = sht->mngr;
    int h, old = sht->height;       // �ݒ�O�̍������o�b�t�@���Ƃ�

    if (height > mngr->top + 1){    // �w�肪�Ⴗ���⍂������������C������
        height = mngr->top + 1;
    }
    if (height < -1){
        height = -1;
    }
    sht->height = height;   // ������ݒ肷��

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
            mngr->top--;    // �\�����̉��~���� 1 ����̂ň�ԏ�̍���������
            sheet_refresh_sub(mngr, sht->vx0, sht->vy0, sht->vx0 + sht->pxl_x, sht->vy0 + sht->pxl_y, 0);    // �V�������~�����ɏ]����ʂ�`������
        }
    }
    else if (old < height){    // �ȑO���������Ȃ�
        if (old >= 0){
            // �Ԃ̂��̂������グ��
            for (h = old; h < height; h++){
                mngr->sheets[h] = mngr->sheets[h + 1];
                mngr->sheets[h]->height = h;
            }
            mngr->sheets[height] = sht;
        }
        else{      // ��\����Ԃ���\����Ԃ�
            // ��ɂȂ���̂������グ��
            for (h = mngr->top; h >= height; h--){
                mngr->sheets[h + 1] = mngr->sheets[h];
                mngr->sheets[h + 1]->height = h + 1;
            }
            mngr->sheets[height] = sht;
            mngr->top++;    // �\�����̉��~���� 1 ������̂ň�ԏ�̍�����������
        }
        sheet_refresh_sub(mngr, sht->vx0, sht->vy0, sht->vx0 + sht->pxl_x, sht->vy0 + sht->pxl_y, height);
    }
    return;
}

// �����̒Ⴂ���ɕ`�悵�Ă���
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1){
    if (sht->height >= 0){
        sheet_refresh_sub(sht->mngr, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height);
    }
    return;
}

// �����̒Ⴂ���ɕ`�悵�Ă���
// �������X�N���[����̕`��͈͂��w�肷��
//         vx0 : �`��͈͂̎n�_
//         vy0 : �`��͈͂̎n�_
//         vx1 : �`��͈͂̏I�_
//         vy1 : �`��͈͂̏I�_
// height_thre : ���̍���������̉��~���̂ݍĕ`�悷��
void sheet_refresh_sub(struct SHTMNGR *mngr, int vx0, int vy0, int vx1, int vy1, int height_thre){
    int h;
    int bx, by;     // ���~���̉��A�c�̐�Βl
    int bx0, by0;
    int bx1, by1;
    int vx, vy;     // ���~���̉��A�c�̃X�N���[����̑��Βl
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
    if (sht->height >= 0){  // �����\�����Ȃ�V�������~���̏��ɉ����ĉ�ʂ���������
        sheet_refresh_sub(sht->mngr, old_vx0, old_vy0, old_vx0 + sht->pxl_x, old_vy0 + sht->pxl_y, 0);
        sheet_refresh_sub(sht->mngr, vx0, vy0, vx0 + sht->pxl_x, vy0 + sht->pxl_y, sht->height);
    }
    return;
}

void sheet_free(struct SHEET *sht){
    // �\�����Ȃ�܂���\���ɂ���
    if (sht->height >= 0){
        sheet_updown(sht, -1);
    }
    sht->flags = 0;     // ���g�p�}�[�N
    return;
}
