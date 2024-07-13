/* Keystone Assembler Engine */
/* By Nguyen Anh Quynh, 2016 */
/* Added by Mark Juvan, 2023*/
#ifndef KEYSTONE_RISCV_H
#define KEYSTONE_RISCV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "keystone.h"

typedef enum ks_err_asm_riscv {
    KS_ERR_ASM_RISCV_INVALIDOPERAND = KS_ERR_ASM_ARCH,
    KS_ERR_ASM_RISCV_MISSINGFEATURE,
    KS_ERR_ASM_RISCV_MNEMONICFAIL,
} ks_err_asm_riscv;


#ifdef __cplusplus
}
#endif

#endif
