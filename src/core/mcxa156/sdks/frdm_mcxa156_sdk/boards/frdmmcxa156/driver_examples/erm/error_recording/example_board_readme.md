Hardware requirements
=====================
- Type-C USB cable
- FRDM-MCXA156 board
- Personal Computer

Board settings
============

Prepare the Demo
===============
1.  Connect a Type-C USB cable between the host PC and the MCU-Link USB port on the target board.
2.  Open a serial terminal with the following settings (See Appendix A in Getting started guide for description how to determine serial port number):
    - 115200 baud rate
    - 8 data bits
    - No parity
    - One stop bit
    - No flow control
3.  Download the program to the target board.
4.  Either press the reset button on your board or launch the debugger in your IDE to begin running the demo.

Running the demo
================
The log below shows the output of the hello world demo in the terminal window:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ERM error recording example.

Original ram data correct.

ERM error recording address is 0x20000000.

ERM error recording example finished successfully.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note:
All system bus masters can access the programming model:
1. Only in supervisor mode
2. Using only 32-bit (word) accesses
Any of the following attempted references to the programming model generates an error termination:
1. In user mode
2. Using non-32-bit access sizes
3. To undefined (reserved) addresses
