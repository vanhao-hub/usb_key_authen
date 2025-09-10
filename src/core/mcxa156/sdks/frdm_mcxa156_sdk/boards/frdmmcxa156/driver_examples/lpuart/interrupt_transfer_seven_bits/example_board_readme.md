Hardware requirements
=====================
- Type-C USB cable
- FRDM-MCXA156 board
- Personal Computer

Board settings
============
Connect the USB to TTL converter to FRDM-MCXA156 board.
- TX of USB2COM to J5-3(P3_20)
- RX of USB2COM to J5-4(P3_21)
- GND of the USB2COM to J5-8(GND)

Prepare the Demo
===============
1.  Connect a USB Type-C cable between the host PC and the MCU-Link USB port on the target board.
2.  Open a serial terminal with the following settings:
    - 115200 baud rate
    - 7 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.

Running the demo
===============
When the demo runs successfully, the log would be seen on the MCU-Link terminal like:
~~~~~~~~~~~~~~~~~~~~~
Lpuart interrupt example with seven data bits
Board receives 8 characters then sends them out
Now please input:
~~~~~~~~~~~~~~~~~~~~~
