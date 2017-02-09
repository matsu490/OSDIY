#include "bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end){
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // ===========================================
    // === 386 なのか、486 以降なのかチェックする
    // 486 以降の CPU では EFLAGS レジスタの 18 ビット目が AC フラグになっている
    // 386 の場合、このビットに 1 を書き込んでも 0 に戻る
    // EFLAGS の第 18 ビットに 1 を書き込んで、再び読み込んだときに 0 なら 386、1 なら 486 と判断する
    // ===========================================
    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT;
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if((eflg & EFLAGS_AC_BIT) != 0)
        flg486 = 1;
    eflg &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflg);
    // ===========================================

    // ===========================================
    // === キャッシュ禁止処理 ====================
    // ===========================================
    if(flg486 != 0){
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    // ===========================================

    i = memtest_sub(start, end);

    // ===========================================
    // === キャッシュ許可処理 ====================
    // ===========================================
    if(flg486 != 0){
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    // ===========================================
    return i;
}

// ==============================================
// === メモリマネージャの初期化処理 ===
// ==============================================
void memmngr_init(struct MEMMNGR *mngr){
    mngr->frees = 0;        // 空き情報の個数
    mngr->maxfrees = 0;     // 状況観察用：frees の最大値
    mngr->lostsize = 0;     // 解放に失敗した合計サイズ
    mngr->losts = 0;        // 解放に失敗した回数
    return;
}

// ==============================================
// === 空き領域のサイズの合計を返す ===
// ==============================================
unsigned int memmngr_total(struct MEMMNGR *mngr){
    unsigned int i, t = 0;
    for (i = 0; i < mngr->frees; i++)
        t += mngr->free[i].size;
    return t;
}

// ==============================================
// === メモリ領域の確保 ===
// *mngr : メモリマネージャ
// size  : 確保するメモリサイズ
// ==============================================
unsigned int memmngr_alloc(struct MEMMNGR *mngr, unsigned int size){
    unsigned int i, a;
    // 空き領域情報テーブルを始めから最後までスキャンしていく
    for (i = 0; i < mngr->frees; i++){
        // ある空き領域のサイズが要求サイズよりも大きかったら
        if (mngr->free[i].size >= size){
            // その空き領域の先頭番地をバッファしておき
            a = mngr->free[i].addr;
            // その空き領域の先頭番地を要求サイズ分だけシフトする
            mngr->free[i].addr += size;
            // さらにその空き領域のサイズを要求サイズ分だけ減らす
            mngr->free[i].size -= size;
            // 上記処理後、その空き領域サイズが 0 になったら
            if (mngr->free[i].size == 0){
                // マネージャが管理している空き領域の合計個数を 1 減らす
                mngr->frees--;
                // 空き領域情報が 1 つなくなったので、空き領域情報をつめる
                for (; i < mngr->frees; i++)
                    mngr->free[i] = mngr->free[i + 1];
            }
            // 要求サイズのメモリ領域の先頭番地を返す
            return a;
        }
    }
    // スキャンしたけど空き領域が見つからなかった場合
    return 0;
}
// ==============================================
// === 4 kB 単位でメモリ領域の確保を行う ===
// 1 バイト単位で管理すると不連続な小さな
// 空き領域ができてしまうため 4 kB 単位とする
// ==============================================
unsigned int memmngr_alloc_4k(struct MEMMNGR *mngr, unsigned int size){
    unsigned int a;
    // 0x1000 単位にするための切り上げ処理
    size = (size + 0xfff) & 0xfffff000;
    a = memmngr_alloc(mngr, size);
    return a;
}

// ==============================================
// === メモリの解放 ===
// メモリマネージャにメモリ情報を追加する
// ==============================================
int memmngr_free(struct MEMMNGR *mngr, unsigned int addr, unsigned int size){
    int i, j;
    // ----------------------------------------------
    // まとめやすさを考えると free[] が addr 順に並んでるほうがいい
    // だからまずどこに入れるべきかを決める
    // ----------------------------------------------
    for (i = 0; i < mngr->frees; i++){
        if (mngr->free[i].addr > addr){
            break;
        }
    }
    // ----------------------------------------------
    // free[i - 1].addr < addr < free[i].addr
    // 前がある場合
    // ----------------------------------------------
    if (i > 0){
        if (mngr->free[i - 1].addr + mngr->free[i - 1].size == addr){
            mngr->free[i - 1].size += size;
            if (i < mngr->frees){
                if (addr + size == mngr->free[i].addr){
                    mngr->free[i - 1].size += mngr->free[i].size;
                    mngr->frees--;
                    for (; i < mngr->frees; i++){
                        mngr->free[i] = mngr->free[i + 1];
                    }
                }
            }
            return 0;
        }
    }
    if (i < mngr->frees){
        if (addr + size == mngr->free[i].addr){
            mngr->free[i].addr = addr;
            mngr->free[i].size = size;
            return 0;
        }
    }
    if (mngr->frees < MEMMNGR_FREES){
        for (j = mngr->frees; j > i; j--){
            mngr->free[j] = mngr->free[j - 1];
        }
        mngr->frees++;
        if (mngr->maxfrees < mngr->frees){
            mngr->maxfrees = mngr->frees;
        }
        mngr->free[i].addr = addr;
        mngr->free[i].size = size;
        return 0;
    }
    mngr->losts++;
    mngr->lostsize += size;
    return -1;
}
// ==============================================
// === 4 kB 単位でメモリ領域の解放を行う ===
// 1 バイト単位で管理すると不連続な小さな
// 空き領域ができてしまうため 4 kB 単位とする
// ==============================================
int memmngr_free_4k(struct MEMMNGR *mngr, unsigned int addr, unsigned int size){
    int i;
    // 0x1000 単位にするための切り上げ処理
    size = (size + 0xfff) & 0xfffff000;
    i = memmngr_free(mngr, addr, size);
    return i;
}

/* 冗長な処理がコンパイラによって最適化されてしまうので、
 * この処理はアセンブラで書くことにした。
unsigned int memtest_sub(unsigned int start, unsigned int end);
unsigned int memtest(unsigned int start, unsigned int end);

unsigned int memtest_sub(unsigned int start, unsigned int end){
    unsigned int i, *p, old, pattern0 = 0xaa55aa55, pattern1 = 0x55aa55aa;
    // 0x1000 (4 kB) ずつメモリチェックを行う
    // 1 バイトずつやったんじゃ遅くて仕方ない
    // むしろ 1 MB ずつでも問題ないくらい
    for(i = start; i <= end; i += 0x1000){
        p = (unsigned int *) (i + 0xffc);
        old = *p;           // 元々その番地にあった値を退避しておく
        *p = pattern0;      // 試しに適当な値を書き込んでみる
        *p ^= 0xffffffff;   // 反転してみる
        // 反転結果になってなければメモリテストを終了する
        // 機種によっては書き込んだ値がそのまま読めてしまうことがあるため反転してみる
        if(*p != pattern1){
not_memory:
            *p = old;
            break;
        }
        *p ^= 0xffffffff;   // もう一度反転してみる
        // 値が元に戻らなければメモリテストを終了する
        if(*p != pattern0)
            goto not_memory;
        *p = old;           // 元の値に戻す
    }
    return i;
}
*/
