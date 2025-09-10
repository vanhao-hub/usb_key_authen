Hardware requirements
=====================
- Type-C USB cable
- FRDM-MCXA156 Board
- Personal Computer

Board settings
==============
No special settings are required.

Prepare the Demo
================
1. Connect a USB cable between the PC host and the MCU-Link USB on the board.
2. Open a serial terminal on PC for MCU-Link serial device with these settings:
   - 115200 baud rate
   - 8 data bits
   - No parity
   - One stop bit
   - No flow control
3. Download the program to the target board.
4. Either press the reset button on your board or launch the debugger in your IDE to begin running
   the example.

Running the demo
================
When the example runs successfully, following information can be seen on the MCU-Link terminal:

~~~~~~~~~~~~~~~~~~~~~~


==FlexCAN loopback functional example -- Start.==

Send message from MB1 to MB0
tx word0 = 0x11223344
tx word1 = 0x55667788

Received message from MB0
rx word0 = 0x11223344
rx word1 = 0x55667788

==FlexCAN loopback functional example -- Finish.==
~~~~~~~~~~~~~~~~~~~~~~
