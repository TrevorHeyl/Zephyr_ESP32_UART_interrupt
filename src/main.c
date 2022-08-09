/*
 *
 */

#include <string.h>
#include <stdio.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <sys/ring_buffer.h>

/**
 * @brief
 * Get All node identifiers
 */
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


/* UART data buffer */
static char rx_buf[MSG_SIZE];
static char rx_msg_buf[MSG_SIZE];
static int rx_buf_pos=0;
 

// // * @brief
//  //* Print a null-terminated string character by character to the UART interface in a polled manner
//  //*  */
// void print_uart(char *buf)
// {
// 	int msg_len = strlen(buf);
// 	for (int i = 0; i < msg_len; i++)
// 	{
// 		uart_poll_out(uart_1, buf[i]);
// 	}
// }


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



/**
 * @brief  the serial callback
 * Interrupts here for each received character. Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c ;
	uint8_t tx_char[1];
	uint32_t ret;

	if (uart_irq_update(dev) && uart_irq_is_pending(dev) )
	{
		if (uart_irq_tx_ready(dev) )
		{
			
			ret = ring_buf_get(&uart_tx_buffer,tx_char,1);
			if ( ret != 1) 
			{
				uart_irq_tx_disable(dev);
			}
			else 
			{
				uart_fifo_fill(dev,tx_char,1);
			}
		}

		while (uart_irq_rx_ready(dev))
		{

			uart_fifo_read(dev, &c, 1);

			if ((c == '\n' || c == '\r') && rx_buf_pos > 0)
			{
				/* terminate string */
				rx_buf[rx_buf_pos] = '\0';

				/* if queue is full, message is silently dropped */
				k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

				/* reset the buffer (it was copied to the msgq) */
				rx_buf_pos = 0;
			}
			else if (rx_buf_pos < (sizeof(rx_buf) - 1))
			{
				rx_buf[rx_buf_pos++] = c;
			}
			/* else: characters beyond buffer size are dropped */
		}
		




	}


}

void main(void)
{
	int ret;

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
	ring_buf_reset(&uart_tx_buffer);
	ring_buf_init(&uart_rx_buffer, UART_RX_RING_BUFFER_SIZE , uart_rx_buffer.buffer  );
	ring_buf_reset(&uart_rx_buffer);

	/* Configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart_1, serial_cb, NULL);
	uart_irq_rx_enable(uart_1);
	uart_irq_tx_disable(uart_1);



	//gpio_pin_set_dt(&ledgr,1);
	//k_msleep(1500);
	//gpio_pin_set_dt(&ledgr,0);



	while (1)
	{
		print_uart("Waiting for message from UART, type a message and end with <CR>\n");
		while (k_msgq_get(&uart_msgq, &rx_msg_buf, K_FOREVER) == 0)
		{
			print_uart("Message:");
			print_uart(rx_msg_buf);
			print_uart("\r\n");
			
		}
	}
}
