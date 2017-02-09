#include "bootpack.h"
// GDT や IDT などの descriptor table 関係
//
// セグメンテーション
// 各アプリが利用するメモリ領域に競合が起きないようにメモリを分割して、各分割領域の始めの番地を 0 番地として扱う
// 1 つのセグメントを表すには以下の 3 つの情報が必要である
// ・セグメントの大きさ
// ・セグメントが始まる番地
// ・管理用属性（書込み禁止、実行禁止、システム専用など）
// CPU 上ではこれらの情報を 64 ビット（8 バイト）で表している
// しかしセグメントレジスタは 16 ビットしかない
// この内 13 ビット（0 から 8,191 番の合計 8,192 個のセグメントが定義できる）（下位 3 ビットは仕様上使えない）にセグメント番号を記録し、この番号と各セグメントを対応付ける
// つまり全 8,192 個のセグメントを設定するには 8,192 * 8 バイト = 65,536 バイト（64 KB）が必要になる
// CPU には当然そんな容量はないのでメモリに記録する
// この 64 KB のメモリ領域を GDT と呼ぶ
//
// GDT : Global segment descriptor table
// 大域セグメント記述子表
// 全 8,192 個分のセグメント情報を保存する 64 KB のメモリ領域
// この先頭番地と有効設定個数を CPU の GDTR (global segment descriptor table register) に設定する
//
// IDT : Interrupt descriptor table
// 割り込み記述子表
// 0 から 255 の割り込み番号とそれぞれに対応した関数を記述している
// 例えば割り込み番号 123 が発生したら、それと対応する f123() 関数を呼び出すといった時に IDT が参照される
// IDT の設定の前にセグメントの設定がされている必要があるので、GDT の設定が必要である
//

void init_gdtidt(void){
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
    int i;

    // LIMIT_GDT = 0xffff = 65,536
    // gdt は定義上 8 バイトなので、番地をインクリメントすると 8 番地ずつ増加していく
    // つまり全 8,192 (= LIMIT_GDT / 8) 個分のセグメント情報を 0 埋めしている
    for(i = 0; i <= LIMIT_GDT / 8; i++){
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 1 番と 2 番のセグメントを設定する
    set_segmdesc(gdt + 1, 0xffffffff  , 0x00000000, AR_DATA32_RW);
    // bootpack.hrb のためのもの
    // このセグメントを使うことで bootpack.hrb を実行することができる
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    // アセンブラで GDT を読む
    load_gdtr(LIMIT_GDT, ADR_GDT);

    // IDT の初期化
    for(i = 0; i <= LIMIT_IDT / 8; i++){
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(LIMIT_IDT, ADR_IDT);

    // IDT の設定
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
    return;
}

// 1 セグメント分の情報を設定する
// limit (20 bit) : 
// セグメントの大きさ（セグメントバイト数 - 1）
// 最大で 4 GB のセグメントを表せる必要があるため、32 bit の数値が必要
// セグメント属性に 12 bit 使うので 20 bit になっている
// セグメント属性の G ビット（granularity, 粒度）を 1 にすると、リミットの単位をバイト単位からページ単位（1 ページが 4 KB）となり 4 GB を表すことができる
// low (16 bit), high (4 bit), セグメント属性の続き用（4 bit）に分けられる
// base  (32 bit) :
// セグメントがどの番地から始まるか（ベース番地と一般には呼ばれる）
// 80286 との互換性のため low (16 bit), mid (8 bit), high (8 bit) の3つに分けられる
// 2 ** 32 個の番地（メモリ 4 GB 分）を一意に表す必要があるため 32 bit
// ar    (12 bit) : 
// セグメント属性、セグメントのアクセス権
// 上位 4 bit は拡張アクセス権（GD00）
// G : G ビット
// D : セグメントモード、1 = 32 ビットモード、0 = 16 ビットモード
// 合計 64 bit = 8 byte = 1 セグメントのサイズ
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
    if(limit > 0xfffff){
        ar |= 0x8000;
        limit /= 0x1000;
    }
    sd->limit_low       = limit & 0xffff;
    sd->limit_high      = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_low        = base  & 0xffff;
    sd->base_mid        = (base >> 16) & 0xff;
    sd->base_high       = (base >> 24) & 0xff;
    sd->access_right    = ar    & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
    gd->offset_low      = offset & 0xffff;
	gd->offset_high     = (offset >> 16) & 0xffff;
	gd->selector        = selector;
	gd->dw_count        = (ar >> 8) & 0xff;
	gd->access_right    = ar & 0xff;
	return;
}
