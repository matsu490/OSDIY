/* FIFO ���C�u���� */
#include "bootpack.h"
#define FLAGS_OVERRUN   0x0001

/* FIFO �o�b�t�@�̏����� */
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf){
    fifo->size = size;
    fifo->buf = buf;    /* �o�b�t�@�̔Ԓn */
    fifo->free = size;  /* ���݉��o�C�g�󂢂Ă��邩 */
    fifo->flags = 0;    /* ���ӂꂽ���Ƃ�m�点��t���O */
    fifo->p = 0;        /* �������݈ʒu */
    fifo->q = 0;        /* �ǂݍ��݈ʒu */
    return;
}

/* FIFO �o�b�t�@�� 1 �o�C�g�̃f�[�^�L�^���� */
int fifo8_put(struct FIFO8 *fifo, unsigned char data){
    /* �󂫂��Ȃ��Ă��ӂꂽ */
    if(fifo->free == 0){
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if(fifo->p == fifo->size){
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

/* FIFO ���� 1 �o�C�g�̃f�[�^��������Ă��� */
int fifo8_get(struct FIFO8 *fifo){
    int data;

    /* �o�b�t�@����̎��� -1 ��Ԃ� */
    if(fifo->free == fifo->size){
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if(fifo->q == fifo->size){
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

/* �ǂ̂��炢�f�[�^�����܂��Ă��邩��Ԃ� */
int fifo8_status(struct FIFO8 *fifo){
    return fifo->size - fifo->free;
}
