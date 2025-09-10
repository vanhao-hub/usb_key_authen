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
#define BOARD_PWM_BASEADDR     (FLEXPWM0)
#define PWM_SRC_CLK_FREQ       (CLOCK_GetFreq(kCLOCK_MainClk))
#define DEMO_PWM_CLOCK_DEVIDER (kPWM_Prescale_Divide_4)
#define DEMO_PWM_FAULT_LEVEL   true

#define BOARD_LED_GPIO     BOARD_LED_RED_GPIO
#define BOARD_LED_GPIO_PIN BOARD_LED_RED_GPIO_PIN
/*${macro:end}*/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*${prototype:start}*/
void BOARD_InitHardware(void);
/*${prototype:end}*/

#endif /* _APP_H_ */
