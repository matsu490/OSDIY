#include "bootpack.h"
#include <stdio.h>

struct FIFO8 keyfifo;

// �L�[�{�[�h�R���g���[�����f�[�^���M�\�ɂȂ�̂�҂�
void wait_KBC_sendready(void){
    for(;;){
        if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
            break;
    }
    return;
}

// �L�[�{�[�h�R���g���[���̏�����
void init_keyboard(void){
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

void inthandler21(int *esp){
    unsigned char data;

    // IRQ-01 ��t������ PIC0 �ɒʒm����
    io_out8(PIC0_OCW2, 0x61);

    data = io_in8(PORT_KEYDAT);
    fifo8_put(&keyfifo, data);
    return;
}
