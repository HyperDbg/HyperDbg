/**
 * @file MetaMacros.h
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief Helper macros
 * @details
 * @version 0.1
 * @date 2022-05-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#define MetaScopedExpr(Pre_, Post_, ScopedExpr_) \
    do                                           \
    {                                            \
        Pre_;                                    \
        ScopedExpr_;                             \
        Post_;                                   \
    } while (FALSE);

//
// Linked-List
//

//
// borrowed from
// https://github.com/hvmi/hvmi/blob/master/introcore/include/hlist.h
// https://github.com/hvmi/hvmi/blob/master/introcore/src/introcore.c
//

#define LIST_FOR_EACH(_head, _struct_type, _var)               _LIST_FOR_EACH(_head, _struct_type, Link, _var)
#define LIST_FOR_NEXT(_start, _head, _type, _var)              _LIST_FOR_NEXT(_start, _head, _type, Link, _var)
#define LIST_FOR_EACH_LINK(_head, _struct_type, _member, _var) _LIST_FOR_EACH(_head, _struct_type, _member, _var)

//
// Conversion
//
#define HANDLE_TO_UINT32(_var) (UINT32)((UINT64)_var & 0xffffffff)
#define PVOID_TO_BOOLEAN(_var) (BOOLEAN)((UINT64)_var & 0xff)

//
// RAW versions, use only if really needed
//
#define _NEXT(_var, _member)              _var->_member.Flink
#define _NEXT_ENTRY(_var, _member, _type) CONTAINING_RECORD(_NEXT(_var, _member), _type, _member)

#define PREPROC_CONCAT(a, b)     PREPROC_CONCAT_1(a, b)
#define PREPROC_CONCAT_1(a, b)   PREPROC_CONCAT_2(~, a##b)
#define PREPROC_CONCAT_2(p, res) res

#define UNIQUE_NAME(base) PREPROC_CONCAT(base, __LINE__)

#define _LIST_FOR_EACH(_head, _type, _member, _var)                                                                           \
    for (_type * _var = CONTAINING_RECORD(_head.Flink, _type, _member), *UNIQUE_NAME(_n) = _NEXT_ENTRY(_var, _member, _type); \
         &_var->_member != &_head;                                                                                            \
         _var = UNIQUE_NAME(_n), UNIQUE_NAME(_n) = _NEXT_ENTRY(_var, _member, _type))

#define _LIST_FOR_NEXT(_start, _head, _type, _member, _var)                                                        \
    for (_type * _var = _NEXT_ENTRY(_start, _member, _type), *UNIQUE_NAME(_n) = _NEXT_ENTRY(_var, _member, _type); \
         &_var->_member != &_head;                                                                                 \
         _var = UNIQUE_NAME(_n), UNIQUE_NAME(_n) = _NEXT_ENTRY(_var, _member, _type))
