
This demo shows how to use UART transmit and receive interrupts by implementing an echo bot.
################
Overview
********
This demo shows how to use UART transmit and receive interrupts.
The UART data is clocked in and out using ring buffers provided by Zephyr
A timer is used to trigger a work queue to parse the rceived data in the ring buffer
The demo is written for ESP32 and on UART1 but should work with most boards with a few mods to the device tree (for LED and UART1)
A TX polled mode is available by using #define UART_POLLED_TX_MODE
Pin 19 RXD (in to ESP32) 3V
Pin 18 TXD (out from ESP32) 3V
 
Building and Running
********************
See prj.cong, these Kconfigs are required:
 * CONFIG_SERIAL=y
 * CONFIG_GPIO=y
 * CONFIG_UART_INTERRUPT_DRIVEN=y
 * CONFIG_RING_BUFFER=y

Additionally a UART1 is required, please see the device tree

west build -p -b esp_custom


Sample Output
=============
Connect a terminal emulator @115200 bps to UART 1. And whatever you type will be echoed
