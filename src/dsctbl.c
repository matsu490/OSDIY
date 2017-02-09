#include "bootpack.h"
// GDT �� IDT �Ȃǂ� descriptor table �֌W
//
// �Z�O�����e�[�V����
// �e�A�v�������p���郁�����̈�ɋ������N���Ȃ��悤�Ƀ������𕪊����āA�e�����̈�̎n�߂̔Ԓn�� 0 �Ԓn�Ƃ��Ĉ���
// 1 �̃Z�O�����g��\���ɂ͈ȉ��� 3 �̏�񂪕K�v�ł���
// �E�Z�O�����g�̑傫��
// �E�Z�O�����g���n�܂�Ԓn
// �E�Ǘ��p�����i�����݋֎~�A���s�֎~�A�V�X�e����p�Ȃǁj
// CPU ��ł͂����̏��� 64 �r�b�g�i8 �o�C�g�j�ŕ\���Ă���
// �������Z�O�����g���W�X�^�� 16 �r�b�g�����Ȃ�
// ���̓� 13 �r�b�g�i0 ���� 8,191 �Ԃ̍��v 8,192 �̃Z�O�����g����`�ł���j�i���� 3 �r�b�g�͎d�l��g���Ȃ��j�ɃZ�O�����g�ԍ����L�^���A���̔ԍ��Ɗe�Z�O�����g��Ή��t����
// �܂�S 8,192 �̃Z�O�����g��ݒ肷��ɂ� 8,192 * 8 �o�C�g = 65,536 �o�C�g�i64 KB�j���K�v�ɂȂ�
// CPU �ɂ͓��R����ȗe�ʂ͂Ȃ��̂Ń������ɋL�^����
// ���� 64 KB �̃������̈�� GDT �ƌĂ�
//
// GDT : Global segment descriptor table
// ���Z�O�����g�L�q�q�\
// �S 8,192 ���̃Z�O�����g����ۑ����� 64 KB �̃������̈�
// ���̐擪�Ԓn�ƗL���ݒ���� CPU �� GDTR (global segment descriptor table register) �ɐݒ肷��
//
// IDT : Interrupt descriptor table
// ���荞�݋L�q�q�\
// 0 ���� 255 �̊��荞�ݔԍ��Ƃ��ꂼ��ɑΉ������֐����L�q���Ă���
// �Ⴆ�Ί��荞�ݔԍ� 123 ������������A����ƑΉ����� f123() �֐����Ăяo���Ƃ��������� IDT ���Q�Ƃ����
// IDT �̐ݒ�̑O�ɃZ�O�����g�̐ݒ肪����Ă���K�v������̂ŁAGDT �̐ݒ肪�K�v�ł���
//

void init_gdtidt(void){
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
    int i;

    // LIMIT_GDT = 0xffff = 65,536
    // gdt �͒�`�� 8 �o�C�g�Ȃ̂ŁA�Ԓn���C���N�������g����� 8 �Ԓn���������Ă���
    // �܂�S 8,192 (= LIMIT_GDT / 8) ���̃Z�O�����g���� 0 ���߂��Ă���
    for(i = 0; i <= LIMIT_GDT / 8; i++){
        set_segmdesc(gdt + i, 0, 0, 0);
    }
    // 1 �Ԃ� 2 �Ԃ̃Z�O�����g��ݒ肷��
    set_segmdesc(gdt + 1, 0xffffffff  , 0x00000000, AR_DATA32_RW);
    // bootpack.hrb �̂��߂̂���
    // ���̃Z�O�����g���g�����Ƃ� bootpack.hrb �����s���邱�Ƃ��ł���
    set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
    // �A�Z���u���� GDT ��ǂ�
    load_gdtr(LIMIT_GDT, ADR_GDT);

    // IDT �̏�����
    for(i = 0; i <= LIMIT_IDT / 8; i++){
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(LIMIT_IDT, ADR_IDT);

    // IDT �̐ݒ�
    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
    set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
    return;
}

// 1 �Z�O�����g���̏���ݒ肷��
// limit (20 bit) : 
// �Z�O�����g�̑傫���i�Z�O�����g�o�C�g�� - 1�j
// �ő�� 4 GB �̃Z�O�����g��\����K�v�����邽�߁A32 bit �̐��l���K�v
// �Z�O�����g������ 12 bit �g���̂� 20 bit �ɂȂ��Ă���
// �Z�O�����g������ G �r�b�g�igranularity, ���x�j�� 1 �ɂ���ƁA���~�b�g�̒P�ʂ��o�C�g�P�ʂ���y�[�W�P�ʁi1 �y�[�W�� 4 KB�j�ƂȂ� 4 GB ��\�����Ƃ��ł���
// low (16 bit), high (4 bit), �Z�O�����g�����̑����p�i4 bit�j�ɕ�������
// base  (32 bit) :
// �Z�O�����g���ǂ̔Ԓn����n�܂邩�i�x�[�X�Ԓn�ƈ�ʂɂ͌Ă΂��j
// 80286 �Ƃ̌݊����̂��� low (16 bit), mid (8 bit), high (8 bit) ��3�ɕ�������
// 2 ** 32 �̔Ԓn�i������ 4 GB ���j����ӂɕ\���K�v�����邽�� 32 bit
// ar    (12 bit) : 
// �Z�O�����g�����A�Z�O�����g�̃A�N�Z�X��
// ��� 4 bit �͊g���A�N�Z�X���iGD00�j
// G : G �r�b�g
// D : �Z�O�����g���[�h�A1 = 32 �r�b�g���[�h�A0 = 16 �r�b�g���[�h
// ���v 64 bit = 8 byte = 1 �Z�O�����g�̃T�C�Y
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
