/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*${header:start}*/
#include "fsl_device_registers.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_inputmux.h"
/*${header:end}*/

/*${function:start}*/
void BOARD_InitHardware(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    INPUTMUX_Init(INPUTMUX0);
    INPUTMUX_AttachSignal(INPUTMUX0, 0U, kINPUTMUX_TrigIn5ToFlexPwm0Fault);
    INPUTMUX_AttachSignal(INPUTMUX0, 1U, kINPUTMUX_TrigIn8ToFlexPwm0Fault);
    INPUTMUX_AttachSignal(INPUTMUX0, 2U, kINPUTMUX_TrigIn9ToFlexPwm0Fault);
    INPUTMUX_AttachSignal(INPUTMUX0, 3U, kINPUTMUX_TrigIn10ToFlexPwm0Fault);
}
/*${function:end}*/
