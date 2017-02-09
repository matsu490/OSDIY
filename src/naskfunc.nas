; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; �I�u�W�F�N�g�t�@�C������郂�[�h
[INSTRSET "i486p"]      ; 486 �̖��߂܂Ŏg�������Ƃ����L�q
[BITS 32]               ; 32 �r�b�g���[�h�p�̋@�B�����点��
[FILE "naskfunc.nas"]   ; �\�[�X�t�@�C�������

    GLOBAL _io_hlt, _io_cli, _io_sti, _io_stihlt
    GLOBAL _io_in8, _io_in16, _io_in32
    GLOBAL _io_out8, _io_out16, _io_out32
    GLOBAL _io_load_eflags, _io_store_eflags
    GLOBAL _load_gdtr, _load_idtr
    GLOBAL _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
    GLOBAL _load_cr0, _store_cr0
    GLOBAL _memtest_sub
    EXTERN _inthandler21, _inthandler27, _inthandler2c

[SECTION .text]         ; �I�u�W�F�N�g�t�@�C���ł͂���������Ă���v���O����������

_io_hlt:     ; void io_hlt(void);
    HLT      ; CPU ��ҋ@��Ԃɂ���
    RET      ; return �Ɠ���

; ���荞�݃t���O
; 0�FCPU �Ɋ��荞�ݗv���M���������Ƃ��ɖ���������
; 1�FCPU �Ɋ��荞�ݗv���M���������Ƃ��Ɋ��荞�݉�����H�����삷��

; clear interrupt flag, ���荞�݃t���O�� 0 �ɂ���
_io_cli:
    CLI
    RET

; set interrupt flag, ���荞�݃t���O�� 1 �ɂ���
_io_sti:
    STI
    RET

_io_stihlt:
    STI
    HLT
    RET

; IN ����
; �f�o�C�X����d�C�M�����󂯎��
_io_in8:                    ; int io_in8(int port);
    MOV     EDX,[ESP+4]     ; port, �f�o�C�X�ԍ�
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

; OUT ����
; �f�o�C�X�ɓd�C�M���𑗂�
_io_out8:                   ; int io_out8(int port, int data);
    MOV     EDX,[ESP+4]     ; port, �f�o�C�X�ԍ�
    MOV     AL,[ESP+8]      ; data, �f�[�^
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
    PUSHFD          ; push flags double-word, PUSH EFLAGS �Ɠ���
    POP     EAX
    RET

_io_store_eflags:   ; void io_store_eflags(int eflags);
    MOV     EAX,[ESP+4]
    PUSH    EAX
    POPFD           ; POP EFLAGS �Ɠ���
    RET

; �֐�
; �� 1 �����F[ESP+4]
; �� 2 �����F[ESP+8]
; �� 3 �����F[ESP+12]
; �� 4 �����F[ESP+16]
; �߂�l�FRET �̍ۂ� EAX �ɓ����Ă����l���߂�l�ɂȂ�

; PUSH ����
; �o�b�t�@�i�X�^�b�N�\���j�ɕϐ���ޔ������Ă���

; POP ����
; �o�b�t�@����ޔ�����Ă������̂�������A�ϐ��ɑ������

; EFLAGS
; FLAGS �Ƃ��� 16 �r�b�g�̃��W�X�^���g�����ꂽ 32 �r�b�g���W�X�^
; �L�����[�t���O�⊄�荞�݃t���O�����l�܂������W�X�^
; ���Ɋ��荞�݃t���O�͂��̃��W�X�^��ǂݍ���Ń`���b�N�����

; GDT ��ǂ�
; ��Flimit = 0xffff, addr = 0x270000
; DWORD [ESP+4] �� limit (0x0000ffff) ���ADWORD [ESP+8] �ɔԒn�i0x00270000�j������
;     <-�������̓�        �������̐K��->
;      [ESP+4]     [ESP+8]
;         |__________ |__________
;         ff ff 00 00 00 00 27 00
; ���������ɂ͒l�̌�납�珑�����܂��
_load_gdtr:                 ; void load_gdtr(int limit, int addr);
    ; AX �� 0x0000ffff ���o�b�t�@����
    ; [ESP+6] �� 0x0000ffff ��������
    MOV     AX,[ESP+4]      ; limit
    MOV     [ESP+6],AX
    ; <-�������̓�        �������̐K��->
    ;  [ESP+4]     [ESP+8]
    ;     |__________ |__________
    ;     ff ff 00 00 00 00 27 00
    ;     ff ff ff ff 00 00 27 00
    ;           |----------
    ;        [ESP+6]
    ; GDTR (global segment descriptor table redister) �ɑ������
    ; LGDT �����s���邽�߂� ff ff 00 00 27 00 �Ƃ������т��~���������̂�
    ; ��L�̂悤�ȏ������s����
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
        ; EDI, ESI, EBX ���X�^�b�N�ɑޔ����Ă���
        PUSH    EDI                     ; EBX, ESI, EDI ���g�������̂�
        PUSH    ESI
        PUSH    EBX
        ; �������e�X�g�Ɏg�p����e�ϐ��̏�����
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
