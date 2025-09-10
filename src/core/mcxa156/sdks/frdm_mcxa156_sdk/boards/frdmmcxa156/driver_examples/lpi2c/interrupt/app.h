/*
 * Copyright 2024 NXP
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
#define EXAMPLE_LPI2C_MASTER_BASEADDR LPI2C0
#define EXAMPLE_LPI2C_SLAVE_BASEADDR  LPI2C3

/* Get frequency of lpi2c clock */
#define LPI2C_CLOCK_FREQUENCY CLOCK_GetLpi2cClkFreq(0u)

#define LPI2C_MASTER_CLOCK_FREQUENCY LPI2C_CLOCK_FREQUENCY
#define LPI2C_SLAVE_CLOCK_FREQUENCY  LPI2C_CLOCK_FREQUENCY

#define LPI2C_MASTER_IRQ LPI2C0_IRQn
#define LPI2C_SLAVE_IRQ  LPI2C3_IRQn

#define LPI2C_MASTER_IRQHandler LPI2C0_IRQHandler
#define LPI2C_SLAVE_IRQHandler  LPI2C3_IRQHandler
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
