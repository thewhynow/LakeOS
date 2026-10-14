/**
 * @file idt.h
 * @brief interrupt descriptor table
 */

#ifndef _IDT_H
#define _IDT_H

#include "../../libc/include/types.h"


/**
 * flags for the gate type + privilege level of an IDT entry,
 *  or them together (a gate type, a ring, and PRESENT)
 */
typedef enum {
    IDT_FLAG_GATE_TASK       = 0x5,
    IDT_FLAG_GATE_16BIT_INT  = 0x6,
    IDT_FLAG_GATE_16BIT_TRAP = 0x7,
    /** interrupt gate, interrupts are disabled while the handler runs */
    IDT_FLAG_GATE_32BIT_INT  = 0xE,
    IDT_FLAG_GATE_32BIT_TRAP = 0xF,

    /** ring that is allowed to trigger the gate with the int instruction */
    IDT_FLAG_RING0           = (0 << 5),
    IDT_FLAG_RING1           = (1 << 5),
    IDT_FLAG_RING2           = (2 << 5),
    IDT_FLAG_RING3           = (3 << 5),

    /** gate is in use, the cpu faults if it's not set */
    IDT_FLAG_PRESENT         = 0x80,
} IDT_FLAGS;

/**
 * one gate of the IDT
 */
typedef struct {
    /** handler address, bits 0-15 */
    uint16_t base_low;
    /** code segment the handler runs in */
    uint16_t segment_select;
    uint8_t reserved;
    /** gate type | ring | present, see IDT_FLAGS */
    uint8_t flags;
    /** handler address, bits 16-31 */
    uint16_t base_high;
} __attribute__((__packed__)) IDT_entry_t;

/**
 * what lidt wants
 */
typedef struct {
    /** size of the table in bytes - 1 */
    uint16_t limit;
    IDT_entry_t* ptr;
} __attribute__((__packed__)) IDT_descriptor_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief loads the IDT into the cpu
 */
void IDT_init();

/**
 * @brief fills in a gate
 * @param int_num the vector, 0 - 255
 * @param _base the handler
 * @param segment_descriptor code segment for the handler, GDT_CODE_SEGMENT
 * @param flags see IDT_FLAGS, it won't fire until PRESENT is set
 *  (here or with IDT_enablegate)
 */
void IDT_setgate(int int_num, void(*_base)(), uint16_t segment_descriptor, uint8_t flags);

/**
 * the descriptor for the one IDT
 */
extern IDT_descriptor_t IDT_descriptor;

/**
 * @brief sets the present flag of a gate
 * @param int_num the vector
 */
void IDT_enablegate(int int_num);

/**
 * @brief clears the present flag of a gate
 * @param int_num the vector
 */
void IDT_disablegate(int int_num);
#ifdef __cplusplus
}
#endif

#endif
