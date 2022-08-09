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


#define UART_POLLED_TX_MODE  //Enable to use polled Tx mode instead of interrupts

/* Get All node identifiers */
#define LEDGR_NODE DT_ALIAS(ledgp1)	   // Get the node identifier for the ledgp1 from is alias
#define UART1_NODE DT_NODELABEL(uart1) // Get the node identifier for uart1 from its node label

static const struct gpio_dt_spec ledgr = GPIO_DT_SPEC_GET(LEDGR_NODE, gpios);
const struct device *uart_1 = DEVICE_DT_GET(UART1_NODE);

/* Queue to store up to 10 messages (aligned to 4-byte boundary) */
#define MSG_SIZE 32
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* Ring buffers for Tx and RX uart handling */
#define UART_TX_RING_BUFFER_SIZE			100
#define UART_RX_RING_BUFFER_SIZE			100
RING_BUF_DECLARE(uart_tx_buffer,UART_TX_RING_BUFFER_SIZE);   /* Can also declare more efficient buffers of modulo n bytes  RING_BUGGER_DECLARE_POW2 */
RING_BUF_DECLARE(uart_rx_buffer,UART_RX_RING_BUFFER_SIZE);   

/* Function prototypes */
void print_uart(char *buf);


/* Timers and work queues */
void uart_rx_timer_handler(struct k_timer *dummy);
K_TIMER_DEFINE(uart_rx_timer, uart_rx_timer_handler,NULL); 
void uart_work_handler(struct k_work *work);
K_WORK_DEFINE(uart_work, uart_work_handler);


/**
 * @brief uart_rx_timer_handler
 * 	 Timer interrupt to kick the uart work handler.
 * 		Since this is an interrupt we dont want to block here so we
 * 		pass off any processing to a work thread/queue
 * @param dummy 
 */
void uart_rx_timer_handler(struct k_timer *dummy)
{
	    k_work_submit(&uart_work);
}


/**
 * @brief uart_work_handler
 *   Work queue handler to process the uart ring buffer on a periodic basis
 * @param work 
 */
void uart_work_handler(struct k_work *work)
{
	uint8_t  rx_char[2];

	while( ring_buf_get(&uart_rx_buffer,rx_char,1) == 1 )
	{
		gpio_pin_toggle_dt(&ledgr);
		rx_char[1] = 0;
		print_uart(rx_char);
	}

}

#ifdef UART_POLLED_TX_MODE
/** @brief
* Print a null-terminated string character by character to the UART interface in a polled manner
*/
void print_uart(char *buf)
{
	int msg_len = strlen(buf);
	for (int i = 0; i < msg_len; i++)
	{
		uart_poll_out(uart_1, buf[i]);
	}
}

#else
/**
 * @brief
 * Print a null-terminated string character by character to the UART interface with interrupt mode
 *  */
void print_uart(char *buf)
{
	uint32_t ret;
	uint32_t length = (uint32_t)strlen(buf);

	ret = ring_buf_put(&uart_tx_buffer,(uint8_t *)buf,(uint32_t)length);
	if( ret != length)
	{
		// not enough space in buffer, handle partial print
	} else
	{
		uart_irq_tx_enable(uart_1);
	}
}

#endif

/**
 * @brief  the serial callback
 * Interrupts here for each received character. Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t rxtx_char[1];
	uint32_t ret;

	if (uart_irq_update(dev) && uart_irq_is_pending(dev) )
	{
#ifndef UART_POLLED_TX_MODE  //Enable to use polled Tx mode instead of interrupts
		if (uart_irq_tx_ready(dev) )
		{		
			ret = ring_buf_get(&uart_tx_buffer,rxtx_char,1);
			if ( ret != 1) 
			{
				uart_irq_tx_disable(dev);
			}
			else 
			{
				uart_fifo_fill(dev,rxtx_char,1);
			}
		}
#endif		

		while (uart_irq_rx_ready(dev))
		{
			ret = uart_fifo_read(dev, rxtx_char, 1);
			if (ret == 1)
			{
				ret = ring_buf_put(&uart_rx_buffer,(uint8_t *)rxtx_char,1);
				if( ret != 1)
				{
					// not enough space in buffer, handle partial print
				} else
				{
					// ok
				}
			}
		}
	}
}



void main(void)
{
	int ret;
	char general_message_buffer[100];

	if (!device_is_ready(ledgr.port))
	{
		// do some failed thing
	}

	if (!device_is_ready(uart_1))
	{
		// do some failed thing
	}

	ret = gpio_pin_configure_dt(&ledgr, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		// do some failed thing
	}

	/* Init ring buffers */
	ring_buf_init(&uart_tx_buffer, UART_TX_RING_BUFFER_SIZE , uart_tx_buffer.buffer  );
	ring_buf_reset(&uart_tx_buffer); /* not required initially, but demonstrating how to flush a ring buffer */
	ring_buf_init(&uart_rx_buffer, UART_RX_RING_BUFFER_SIZE , uart_rx_buffer.buffer  );
	ring_buf_reset(&uart_rx_buffer);

	/* Configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart_1, serial_cb, NULL);
	uart_irq_rx_enable(uart_1);
	uart_irq_tx_disable(uart_1);

	gpio_pin_set_dt(&ledgr,0);


	/* Start the uart rx handler timer */
	k_timer_start(&uart_rx_timer,K_MSEC(0),K_MSEC(100));

	print_uart("Waiting to echo data from UART\n");
	/* Do nothing, everyting is handled in interrupts and timers */
	while (1)
	{
		k_msleep(1000);
	}
}
