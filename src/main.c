/**
 * @file main.c
 * @author Trevor heyl  https://github.com/TrevorHeyl/
 * @brief 
 * 	This demo shows howe to use UART transmit and receive interrupts.
 * 	The UART data is clocked in and out using ring buffers provided by Zephyr
 * 	A timer is used to trigger a work queue to parse the rceived data in the ring buffer
 * 	The demo is written for ESP32 and on UART1 but should work with most boards with a few mods
 * 		to the device tree (for LED and UART1)
 * 	A TX polle dmde is available by using #define UART_POLLED_TX_MODE
 * 
 * 	The following Kconfig is required:
 * 	CONFIG_SERIAL=y
 *	CONFIG_GPIO=y
 *	CONFIG_UART_INTERRUPT_DRIVEN=y
 *	CONFIG_RING_BUFFER=y
 * @version 0.1
 * @date 2022-08-09
 * 
 * @copyright M.I.T
 * 
 */

#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <sys/ring_buffer.h>



/* Get All node identifiers */
//#define LEDGR_NODE DT_ALIAS(ledgp1)	   // Get the node identifier for the ledgp1 from is alias

//static const struct gpio_dt_spec ledgr = GPIO_DT_SPEC_GET(LEDGR_NODE, gpios);

/* Queue to store up to 10 messages (aligned to 4-byte boundary) */
//#define MSG_SIZE 32
//K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);



void main(void)
{
	int ret;

	// if (!device_is_ready(ledgr.port))
	// {
	// 	// do some failed thing
	// }

	// ret = gpio_pin_configure_dt(&ledgr, GPIO_OUTPUT_ACTIVE);
	// if (ret < 0)
	// {
	// 	// do some failed thing
	// }


	// gpio_pin_set_dt(&ledgr,0);


	/* Do nothing, everyting is handled in interrupts and timers */
	while (1)
	{
		//k_msleep(1000);
	}
}
