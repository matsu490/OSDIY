; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; オブジェクトファイルを作るモード
[INSTRSET "i486p"]      ; 486 の命令まで使いたいという記述
[BITS 32]               ; 32 ビットモード用の機械語を作らせる
[FILE "naskfunc.nas"]   ; ソースファイル名情報

    GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt
    GLOBAL _io_in8, _io_in16, _io_in32
    GLOBAL _io_out8, _io_out16, _io_out32
    GLOBAL _io_load_eflags, _io_store_eflags
    GLOBAL _load_gdtr, _load_idtr
    GLOBAL _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
    GLOBAL _load_cr0, _store_cr0
    GLOBAL _memtest_sub
    EXTERN _inthandler21, _inthandler27, _inthandler2c

[SECTION .text]         ; オブジェクトファイルではこれを書いてからプログラムを書く

_io_hlt:     ; void io_hlt(void);
    HLT      ; CPU を待機状態にする
    RET      ; return と同じ

; 割り込みフラグ
; 0：CPU に割り込み要求信号が来たときに無視をする
; 1：CPU に割り込み要求信号が来たときに割り込み応答回路が動作する

; clear interrupt flag, 割り込みフラグを 0 にする
_io_cli:
    CLI
    RET

; set interrupt flag, 割り込みフラグを 1 にする
_io_sti:
    STI
    RET

_io_stihlt:
    STI
    HLT
    RET

; IN 命令
; デバイスから電気信号を受け取る
_io_in8:                    ; int io_in8(int port);
    MOV     EDX,[ESP+4]     ; port, デバイス番号
    MOV     EAX,0
    IN      AL,DX
    RET

_io_in16:
    MOV     EDX,[ESP+4]
    MOV     EAX,0
    IN      AX,DX
    RET

_io_in32
    MOV     EDX,[ESP+4]
    IN      EAX,DX
    RET

; OUT 命令
; デバイスに電気信号を送る
_io_out8:                   ; int io_out8(int port, int data);
    MOV     EDX,[ESP+4]     ; port, デバイス番号
    MOV     AL,[ESP+8]      ; data, データ
    OUT     DX,AL
    RET

_io_out16:
    MOV     EDX,[ESP+4]
    MOV     EAX,[ESP+8]
    OUT     DX,AX
    RET

_io_out32
    MOV     EDX,[ESP+4]
    MOV     EAX,[ESP+8]
    OUT     DX,EAX
    RET

_io_load_eflags:    ; int io_load_eflags(void);
    PUSHFD          ; push flags double-word, PUSH EFLAGS と同じ
    POP     EAX
    RET

_io_store_eflags:   ; void io_store_eflags(int eflags);
    MOV     EAX,[ESP+4]
    PUSH    EAX
    POPFD           ; POP EFLAGS と同じ
    RET

; 関数
; 第 1 引数：[ESP+4]
; 第 2 引数：[ESP+8]
; 第 3 引数：[ESP+12]
; 第 4 引数：[ESP+16]
; 戻り値：RET の際に EAX に入っていた値が戻り値になる

; PUSH 命令
; バッファ（スタック構造）に変数を退避させておく

; POP 命令
; バッファから退避されていたものを回収し、変数に代入する

; EFLAGS
; FLAGS という 16 ビットのレジスタが拡張された 32 ビットレジスタ
; キャリーフラグや割り込みフラグ等が詰まったレジスタ
; 特に割り込みフラグはこのレジスタを読み込んでチュックされる

; GDT を読む
; 例：limit = 0xffff, addr = 0x270000
; DWORD [ESP+4] に limit (0x0000ffff) が、DWORD [ESP+8] に番地（0x00270000）が入る
;     <-メモリの頭        メモリの尻尾->
;      [ESP+4]     [ESP+8]
;         |__________ |__________
;         ff ff 00 00 00 00 27 00
; ※メモリには値の後ろから書き込まれる
_load_gdtr:                 ; void load_gdtr(int limit, int addr);
    ; AX に 0x0000ffff をバッファする
    ; [ESP+6] に 0x0000ffff を代入する
    MOV     AX,[ESP+4]      ; limit
    MOV     [ESP+6],AX
    ; <-メモリの頭        メモリの尻尾->
    ;  [ESP+4]     [ESP+8]
    ;     |__________ |__________
    ;     ff ff 00 00 00 00 27 00
    ;     ff ff ff ff 00 00 27 00
    ;           |----------
    ;        [ESP+6]
    ; GDTR (global segment descriptor table redister) に代入する
    ; LGDT を実行するために ff ff 00 00 27 00 という並びが欲しかったので
    ; 上記のような処理を行った
    LGDT    [ESP+6]
    RET

_load_idtr:                 ; void load_idtr(int limit, int addr);
    MOV     AX,[ESP+4]      ; limit
    MOV     [ESP+6],AX
    LIDT    [ESP+6]
    RET

_asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX,ESP
    PUSH    EAX
    MOV     AX,SS
    MOV     DS,AX
    MOV     ES,AX
    CALL    _inthandler21
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_load_cr0:          ; int load_cr0(void);
        MOV EAX,CR0
        RET

_store_cr0:         ; void store_cr0(int cr0);
        MOV EAX,[ESP+4]
        MOV CR0,EAX
        RET

_memtest_sub:       ; unsigned int memtest_sub(unsigned int start, unsigned int end);
        ; EDI, ESI, EBX をスタックに退避しておく
        PUSH    EDI                     ; EBX, ESI, EDI も使いたいので
        PUSH    ESI
        PUSH    EBX
        ; メモリテストに使用する各変数の初期化
        MOV     ESI,0xaa55aa55          ; pattern0 = 0xaa55aa55
        MOV     EDI,0x55aa55aa          ; pattern1 = 0x55aa55aa
        MOV     EAX,[ESP+12+4]          ; i = start;
memtest_loop:
        MOV     EBX,EAX
        ADD     EBX,0xffc               ; p = i + 0xffc;
        MOV     EDX,[EBX]               ; old = *p;
        MOV     [EBX],ESI               ; *p = pattern0;
        XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff;
        CMP     EDI,[EBX]               ; if(*p != pattern1)
        JNE     memtest_finish          ;       goto fin;
        XOR     DWORD [EBX],0xffffffff  ; *p ^= 0xffffffff;
        CMP     ESI,[EBX]               ; if(*p != pattern0)
        JNE     memtest_finish          ;       goto fin;
        MOV     [EBX],EDX               ; *p = old;
        ADD     EAX,0x1000              ; i += 0x1000;
        CMP     EAX,[ESP+12+8]          ; if(i <= end)
        JBE     memtest_loop            ;       goto memtest_loop;
        POP     EBX
        POP     ESI
        POP     EDI
        RET
memtest_finish:
        MOV     [EBX],EDX               ; *p = old;
        POP     EBX
        POP     ESI
        POP     EDI
        RET
