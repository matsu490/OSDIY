#include "bootpack.h"

unsigned int memtest(unsigned int start, unsigned int end){
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // ===========================================
    // === 386 �Ȃ̂��A486 �ȍ~�Ȃ̂��`�F�b�N����
    // 486 �ȍ~�� CPU �ł� EFLAGS ���W�X�^�� 18 �r�b�g�ڂ� AC �t���O�ɂȂ��Ă���
    // 386 �̏ꍇ�A���̃r�b�g�� 1 ����������ł� 0 �ɖ߂�
    // EFLAGS �̑� 18 �r�b�g�� 1 ����������ŁA�Ăѓǂݍ��񂾂Ƃ��� 0 �Ȃ� 386�A1 �Ȃ� 486 �Ɣ��f����
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
    // === �L���b�V���֎~���� ====================
    // ===========================================
    if(flg486 != 0){
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }
    // ===========================================

    i = memtest_sub(start, end);

    // ===========================================
    // === �L���b�V�������� ====================
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
// === �������}�l�[�W���̏��������� ===
// ==============================================
void memmngr_init(struct MEMMNGR *mngr){
    mngr->frees = 0;        // �󂫏��̌�
    mngr->maxfrees = 0;     // �󋵊ώ@�p�Ffrees �̍ő�l
    mngr->lostsize = 0;     // ����Ɏ��s�������v�T�C�Y
    mngr->losts = 0;        // ����Ɏ��s������
    return;
}

// ==============================================
// === �󂫗̈�̃T�C�Y�̍��v��Ԃ� ===
// ==============================================
unsigned int memmngr_total(struct MEMMNGR *mngr){
    unsigned int i, t = 0;
    for (i = 0; i < mngr->frees; i++)
        t += mngr->free[i].size;
    return t;
}

// ==============================================
// === �������̈�̊m�� ===
// *mngr : �������}�l�[�W��
// size  : �m�ۂ��郁�����T�C�Y
// ==============================================
unsigned int memmngr_alloc(struct MEMMNGR *mngr, unsigned int size){
    unsigned int i, a;
    // �󂫗̈���e�[�u�����n�߂���Ō�܂ŃX�L�������Ă���
    for (i = 0; i < mngr->frees; i++){
        // ����󂫗̈�̃T�C�Y���v���T�C�Y�����傫��������
        if (mngr->free[i].size >= size){
            // ���̋󂫗̈�̐擪�Ԓn���o�b�t�@���Ă���
            a = mngr->free[i].addr;
            // ���̋󂫗̈�̐擪�Ԓn��v���T�C�Y�������V�t�g����
            mngr->free[i].addr += size;
            // ����ɂ��̋󂫗̈�̃T�C�Y��v���T�C�Y���������炷
            mngr->free[i].size -= size;
            // ��L������A���̋󂫗̈�T�C�Y�� 0 �ɂȂ�����
            if (mngr->free[i].size == 0){
                // �}�l�[�W�����Ǘ����Ă���󂫗̈�̍��v���� 1 ���炷
                mngr->frees--;
                // �󂫗̈��� 1 �Ȃ��Ȃ����̂ŁA�󂫗̈�����߂�
                for (; i < mngr->frees; i++)
                    mngr->free[i] = mngr->free[i + 1];
            }
            // �v���T�C�Y�̃������̈�̐擪�Ԓn��Ԃ�
            return a;
        }
    }
    // �X�L�����������ǋ󂫗̈悪������Ȃ������ꍇ
    return 0;
}
// ==============================================
// === 4 kB �P�ʂŃ������̈�̊m�ۂ��s�� ===
// 1 �o�C�g�P�ʂŊǗ�����ƕs�A���ȏ�����
// �󂫗̈悪�ł��Ă��܂����� 4 kB �P�ʂƂ���
// ==============================================
unsigned int memmngr_alloc_4k(struct MEMMNGR *mngr, unsigned int size){
    unsigned int a;
    // 0x1000 �P�ʂɂ��邽�߂̐؂�グ����
    size = (size + 0xfff) & 0xfffff000;
    a = memmngr_alloc(mngr, size);
    return a;
}

// ==============================================
// === �������̉�� ===
// �������}�l�[�W���Ƀ���������ǉ�����
// ==============================================
int memmngr_free(struct MEMMNGR *mngr, unsigned int addr, unsigned int size){
    int i, j;
    // ----------------------------------------------
    // �܂Ƃ߂₷�����l����� free[] �� addr ���ɕ���ł�ق�������
    // ������܂��ǂ��ɓ����ׂ��������߂�
    // ----------------------------------------------
    for (i = 0; i < mngr->frees; i++){
        if (mngr->free[i].addr > addr){
            break;
        }
    }
    // ----------------------------------------------
    // free[i - 1].addr < addr < free[i].addr
    // �O������ꍇ
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
// === 4 kB �P�ʂŃ������̈�̉�����s�� ===
// 1 �o�C�g�P�ʂŊǗ�����ƕs�A���ȏ�����
// �󂫗̈悪�ł��Ă��܂����� 4 kB �P�ʂƂ���
// ==============================================
int memmngr_free_4k(struct MEMMNGR *mngr, unsigned int addr, unsigned int size){
    int i;
    // 0x1000 �P�ʂɂ��邽�߂̐؂�グ����
    size = (size + 0xfff) & 0xfffff000;
    i = memmngr_free(mngr, addr, size);
    return i;
}

/* �璷�ȏ������R���p�C���ɂ���čœK������Ă��܂��̂ŁA
 * ���̏����̓A�Z���u���ŏ������Ƃɂ����B
unsigned int memtest_sub(unsigned int start, unsigned int end);
unsigned int memtest(unsigned int start, unsigned int end);

unsigned int memtest_sub(unsigned int start, unsigned int end){
    unsigned int i, *p, old, pattern0 = 0xaa55aa55, pattern1 = 0x55aa55aa;
    // 0x1000 (4 kB) ���������`�F�b�N���s��
    // 1 �o�C�g��������񂶂�x���Ďd���Ȃ�
    // �ނ��� 1 MB ���ł����Ȃ����炢
    for(i = start; i <= end; i += 0x1000){
        p = (unsigned int *) (i + 0xffc);
        old = *p;           // ���X���̔Ԓn�ɂ������l��ޔ����Ă���
        *p = pattern0;      // �����ɓK���Ȓl����������ł݂�
        *p ^= 0xffffffff;   // ���]���Ă݂�
        // ���]���ʂɂȂ��ĂȂ���΃������e�X�g���I������
        // �@��ɂ���Ă͏������񂾒l�����̂܂ܓǂ߂Ă��܂����Ƃ����邽�ߔ��]���Ă݂�
        if(*p != pattern1){
not_memory:
            *p = old;
            break;
        }
        *p ^= 0xffffffff;   // ������x���]���Ă݂�
        // �l�����ɖ߂�Ȃ���΃������e�X�g���I������
        if(*p != pattern0)
            goto not_memory;
        *p = old;           // ���̒l�ɖ߂�
    }
    return i;
}
*/
