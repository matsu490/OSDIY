#include "bootpack.h"
#include <stdio.h>

struct FIFO8 mousefifo;

// �}�E�X�̗L����
void enable_mouse(struct MOUSE_DECODE *mdec){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // ���܂������� ACK (0xfa) �����M�����
    mdec->phase = 0;
    return;
}

int mouse_decode(struct MOUSE_DECODE *mdec, unsigned char dat){
    // �����i�K
    // �}�E�X���L�������ꂽ�玟�̒i�K�Ɉڍs����
    if(mdec->phase == 0){
        if(dat == 0xfa)         // 0xfa �Ƃ������Ƃ̓}�E�X���L��������Ă���
            mdec->phase = 1;
        return 0;
    }
    // �}�E�X�� 1 �o�C�g�ڂ�҂��Ă���i�K
    if(mdec->phase == 1){
        if((dat & 0xc8) == 0x08){  // ������ 1 �o�C�g�ڂ��`�F�b�N����
            // dat �̏�ʌ��� 0x0 ~ 0x3
            // dat �̉��ʌ��� 0x8 ~ 0xf �ł���K�v������
            // 0xc (1100) �Ƃ� & �� 0x0 (0000) �Ȃ�� dat �̏�ʌ��� 0x0 ~ 0x3 (0000 ~ 0011)
            // 0x8 (1000) �Ƃ� & �� 0x8 (1000) �Ȃ�� dat �̏�ʌ��� 0x8 ~ 0xf (1000 ~ 1111)
            mdec->buf[0] = dat;    // �P��N���b�N�ƃN���b�N�̑g�ݍ��킹�ŕω�����
            mdec->phase = 2;
        }
        return 0;
    }
    // �}�E�X�� 2 �o�C�g�ڂ�҂��Ă���i�K
    if(mdec->phase == 2){
        mdec->buf[1] = dat;    // ���ړ��ŕω�����
        mdec->phase = 3;
        return 0;
    }
    // �}�E�X�� 3 �o�C�g�ڂ�҂��Ă���i�K
    if(mdec->phase == 3){
        mdec->buf[2] = dat;    // �c�ړ��ŕω�����
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;    // buf[0] �̉��� 3 ���ɂ���{�^���̏�Ԃ����o��
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if((mdec->buf[0] & 0x10) != 0)
            mdec->x |= 0xffffff00;
        if((mdec->buf[0] & 0x20) != 0)
            mdec->y |= 0xffffff00;
        mdec->y = -mdec->y;     // �}�E�X�ł� y �����̕�������ʂƔ���
        return 1;
    }
    return -1;      // �����ɂ��邱�Ƃ͂Ȃ��͂�
}

// �}�E�X����̊��荞��
void inthandler2c(int *esp){
    unsigned char data;

    // IRQ-12 ��t������ PIC1 �ɒʒm����
    io_out8(PIC1_OCW2, 0x64);

    // IRQ-02 ��t������ PIC0 �ɒʒm����
    io_out8(PIC0_OCW2, 0x62);

    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}
