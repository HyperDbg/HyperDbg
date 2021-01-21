/**
 * @file GdbStub.h
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @brief Remote Serial Protocol (GDB Stub) Headers
 * @details
 * @version 0.1
 * @date 2021-01-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#pragma once

#ifndef _GDBSTUB_H_
#    define _GDBSTUB_H_

/*****************************************************************************
 * Macros
 ****************************************************************************/

#    if DEBUG
#        define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#    else
#        define DEBUG_PRINT(...)
#    endif

#    ifndef EOF
#        define EOF (-1)
#    endif

#    ifndef NULL
#        define NULL ((void *)0)
#    endif

#    ifndef ASSERT
#        if DEBUG
#            define ASSERT(x)                                                                       \
                {                                                                                   \
                    if (!(x))                                                                       \
                    {                                                                               \
                        fprintf(stderr, "ASSERTION FAILED\n");                                      \
                        fprintf(stderr, "  Assertion: %s\n", #x);                                   \
                        fprintf(stderr, "  Location:  %s @ %s:%d\n", __func__, __FILE__, __LINE__); \
                        exit(1);                                                                    \
                    }                                                                               \
                }
#        else
#            define ASSERT(x) \
                do            \
                {             \
                } while (0)
#        endif
#    endif

#    ifndef _GDBSTUB_SYS_H_
#        define _GDBSTUB_SYS_H_

/* Define the size_t type */
#        define DBG_DEFINE_SIZET 1

/* Define required standard integer types (e.g. uint16_t) */
#        define DBG_DEFINE_STDINT 1

/*****************************************************************************
 * Types
 ****************************************************************************/

#        if DBG_DEFINE_STDINT
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
#        endif

typedef unsigned int address;
typedef unsigned int reg;

#        pragma pack(1)
struct dbg_interrupt_state
{
    uint32_t ss;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};
#        pragma pack()

#        pragma pack(1)
struct dbg_idtr
{
    uint16_t len;
    uint32_t offset;
};
#        pragma pack()

#        pragma pack(1)
struct dbg_idt_gate
{
    uint16_t offset_low;
    uint16_t segment;
    uint16_t flags;
    uint16_t offset_high;
};
#        pragma pack()

enum DBG_REGISTER
{
    DBG_CPU_I386_REG_EAX       = 0,
    DBG_CPU_I386_REG_ECX       = 1,
    DBG_CPU_I386_REG_EDX       = 2,
    DBG_CPU_I386_REG_EBX       = 3,
    DBG_CPU_I386_REG_ESP       = 4,
    DBG_CPU_I386_REG_EBP       = 5,
    DBG_CPU_I386_REG_ESI       = 6,
    DBG_CPU_I386_REG_EDI       = 7,
    DBG_CPU_I386_REG_PC        = 8,
    DBG_CPU_I386_REG_PS        = 9,
    DBG_CPU_I386_REG_CS        = 10,
    DBG_CPU_I386_REG_SS        = 11,
    DBG_CPU_I386_REG_DS        = 12,
    DBG_CPU_I386_REG_ES        = 13,
    DBG_CPU_I386_REG_FS        = 14,
    DBG_CPU_I386_REG_GS        = 15,
    DBG_CPU_I386_NUM_REGISTERS = 16
};

struct dbg_state
{
    int signum;
    reg registers[DBG_CPU_I386_NUM_REGISTERS];
};

/*****************************************************************************
 * Const Data
 ****************************************************************************/

void const * const dbg_int_handlers[10000];

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

int
dbg_hook_idt(uint8_t vector, const void * function);
int
dbg_init_gates(void);
int
dbg_init_idt(void);
int
dbg_load_idt(struct dbg_idtr * idtr);
int
dbg_store_idt(struct dbg_idtr * idtr);
uint32_t
dbg_get_cs(void);
void
dbg_int_handler(struct dbg_interrupt_state * istate);
void
dbg_interrupt(struct dbg_interrupt_state * istate);
void
dbg_start(void);
void
dbg_io_write_8(uint16_t port, uint8_t val);
uint8_t
dbg_io_read_8(uint16_t port);
void *
dbg_sys_memset(void * ptr, int data, size_t len);

#    endif

/*****************************************************************************
  * Prototypes
  ****************************************************************************/

int
dbg_main(struct dbg_state * state);

/* System functions, supported by all stubs */
int
dbg_sys_getc(void);
int
dbg_sys_putchar(int ch);
int
dbg_sys_mem_readb(address addr, char * val);
int
dbg_sys_mem_writeb(address addr, char val);
int
dbg_sys_continue();
int
dbg_sys_step();

#endif
