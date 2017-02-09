/* asmhead.nas */
struct BOOTINFO{
    char cyls;          /* ブートセクタはどこまでディスクを読んだのか */
    char leds;          /* ブート時のキーボードのLEDの状態 */
    char vmode;         /* ビデオモード　何ビットカラーか */
    char reserve;
    short scrnx, scrny; /* 画面解像度 */
    char *vram;
};
#define ADR_BOOTINFO    0x00000ff0

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
int load_cr0(void);
void store_cr0(int);
unsigned int memtest_sub(unsigned int start, unsigned int end);

/* fifo.c */
struct FIFO8 {
	unsigned char *buf;
	int p, q, size, free, flags;
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char back_color);
void putblock8_8(char *vram, int vxsize, int pxl_x, int pxl_y,
                 int px0, int py0, char *buf, int pxl_x_);
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

/* dsctbl.c */
// gdt (global segment descriptor table) の 8 バイト
// 1 セグメント分の情報（8 バイト）
struct SEGMENT_DESCRIPTOR{
    short limit_low;    // 2 byte
    short base_low;     // 2 byte
    char base_mid;      // 1 byte
    char access_right;  // 1 byte
    char limit_high;    // 1 byte
    char base_high;     // 1 byte -> 合計 8 byte
};

// IDT (interrupt descriptor table) の 8 バイト
struct GATE_DESCRIPTOR{
    short offset_low;   // 2 byte
    short selector;     // 2 byte
    char dw_count;      // 1 byte
    char access_right;  // 1 byte
    short offset_high;  // 2 byte -> 合計 8 byte
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT         0x0026f800
#define ADR_GDT         0x00270000
#define ADR_BOTPAK      0x00280000
#define LIMIT_IDT       0x000007ff
// 全 8,192 個分のセグメントの情報を記述するための領域（8,192 * 8 B = 65,536 B
#define LIMIT_GDT       0x0000ffff  // 65,536 B = 2 ** 16 = 0xffff
#define LIMIT_BOTPAK    0x0007ffff
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409a
#define AR_INTGATE32    0x008e

/* int.c */
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
// PIC のレジスタはすべて 8 ビット
// IRQ : interrupt request 割り込み信号
// ICW : initial control word 初期化制御データ
// ICW1 から ICW4 まであり、合計 4 バイトのデータ
// ICW1, ICW4 : PIC が基板上にどう配線されているか、割り込み信号の電気的特性に関すること
// IC2 : IRQ をどの割り込み番号として CPU に伝えるかを決めたもの
// IC3 : マスタースレーブ接続に関する設定
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
// IMR : interrupt mask register
// 各ビットがそれぞれ IRQ 信号の 8 個に対応する
// 1 になっている IRQ 信号は無視される
// 0x0021 = 0000 0000 0010 0001
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
#define PORT_KEYDAT             0x0060
#define PORT_KEYCMD             0x0064
#define PORT_KEYSTA             0x0064
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60
#define KBC_MODE                0x47
extern struct FIFO8 keyfifo;
void wait_KBC_sendready(void);
void init_keyboard(void);
void inthandler21(int *esp);

/* mouse.c */
#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4
struct MOUSE_DECODE{
    unsigned char buf[3], phase;
    int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct MOUSE_DECODE * mdec);
int mouse_decode(struct MOUSE_DECODE * mdec, unsigned char dat);
extern struct FIFO8 mousefifo;

/* memory.c */
#define EFLAGS_AC_BIT      0x00040000
#define CR0_CACHE_DISABLE  0x60000000
// メモリ空き領域の管理用領域サイズ（およそ 32 kB）
#define MEMMNGR_FREES      4090
// メモリマネージャ用に 0x003c0000 番地から 32 kB を使うことにした
// 0x00300000 以降は今後使うことになるが、0x003c0000 までは使わないので
#define MEMMNGR_ADDR       0x003c0000
// ==============================================
// === メモリの空き情報 ===
// addr : 空き領域の始めの番地
// size : 空き領域のサイズ
// ==============================================
struct FREEINFO{
    unsigned int addr, size;
};
// ==============================================
// === メモリ管理用マネージャ ===
// ==============================================
struct MEMMNGR{
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMNGR_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memmngr_init(struct MEMMNGR *mngr);
unsigned int memmngr_total(struct MEMMNGR *mngr);
unsigned int mammngr_alloc(struct MEMMNGR *mngr, unsigned int size);
int memmngr_free(struct MEMMNGR *mngr, unsigned int addr, unsigned int size);
unsigned int memmngr_alloc_4k(struct MEMMNGR *man, unsigned int size);
int memmngr_free_4k(struct MEMMNGR *man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS      256 // 管理する下敷きの最大数
#define SHEET_USE       1   // 下敷き使用中フラグ
// ==============================================
// === 下敷き用の構造体 ===
// ==============================================
// *buf    : 下敷きに描いてある内容を記憶している番地
// pxl_x   : 下敷きの横の長さ
// pxl_y   : 下敷きの縦の長さ
// vx0     : 下敷きの x 座標（v は VRAM の v）
// vy0     : 下敷きの y 座標（v は VRAM の v）
// col_inv : 透明色番号（color invisible）
// height  : 下敷きの高さ
// flags   : 下敷きの設定
struct SHEET{
    unsigned char *buf;
    int pxl_x, pxl_y;
    int vx0, vy0;
    int col_inv, height, flags;
    struct SHTMNGR *mngr;
};
// ==============================================
// ==============================================
// === 下敷き管理用構造体 ===
// ==============================================
// *vram   : VRAM の番地
// scrn_x  : 画面の横のサイズ
// scrn_y  : 画面の縦のサイズ
// top     : 一番上の下敷きの高さ
// *sheets : 高さの低い順に並べた 256 枚の下敷きの番地
// sheets0 : 256 枚の下敷き本体
struct SHTMNGR{
    unsigned char *vram;
    int scrn_x, scrn_y, top;
    struct SHEET *sheets[MAX_SHEETS];
    struct SHEET sheets0[MAX_SHEETS];
};
// ==============================================
struct SHTMNGR *shtmngr_init(struct MEMMNGR *memmngr, unsigned char *vram, int scrn_x, int scrn_y);
struct SHEET *sheet_alloc(struct SHTMNGR *mngr);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int pxl_x, int pxl_y, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refresh_sub(struct SHTMNGR *mngr, int vx0, int vy0, int vx1, int vy1, int height_thre);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
