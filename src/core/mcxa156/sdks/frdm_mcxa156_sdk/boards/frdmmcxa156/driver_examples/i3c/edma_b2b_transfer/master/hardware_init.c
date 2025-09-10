/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*${header:start}*/
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
/*${header:end}*/

/*${function:start}*/
void BOARD_InitHardware(void)
{
    /* Attach peripheral clock */
    CLOCK_SetClockDiv(kCLOCK_DivI3C0_FCLK, 4U);
    CLOCK_AttachClk(kFRO_HF_DIV_to_I3C0FCLK);

    BOARD_InitPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
}
/*${function:end}*/
