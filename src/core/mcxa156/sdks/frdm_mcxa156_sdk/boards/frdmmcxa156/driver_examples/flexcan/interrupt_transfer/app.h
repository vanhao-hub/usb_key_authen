/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _APP_H_
#define _APP_H_

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*${macro:start}*/
#define EXAMPLE_CAN                CAN0
#define EXAMPLE_FLEXCAN_IRQn       CAN0_IRQn
#define EXAMPLE_FLEXCAN_IRQHandler CAN0_IRQHandler
#define RX_MESSAGE_BUFFER_NUM      (0)
#define TX_MESSAGE_BUFFER_NUM      (1)

#define EXAMPLE_CAN_CLK_FREQ       CLOCK_GetFlexcanClkFreq()
#define USE_IMPROVED_TIMING_CONFIG (1)
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
