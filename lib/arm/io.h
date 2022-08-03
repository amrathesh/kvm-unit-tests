/*
 * Prototypes for io.c
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#ifndef _ARM_IO_H_
#define _ARM_IO_H_

#include <asm/io.h>

extern void io_init(void);
extern void __iomem *uart_early_base(void);

#endif
