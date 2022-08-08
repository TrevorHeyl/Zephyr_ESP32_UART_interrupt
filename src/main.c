/*
 *
 */

#include <string.h>
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>

/**
 * @brief
 * Get All node identifiers
 */
#define LEDGR_NODE DT_ALIAS(ledgp1)	   // Get the node identifier for the ledgp1 from is alias
#define UART1_NODE DT_NODELABEL(uart1) // Get the node identifier for uart1 from its node label

static const struct gpio_dt_spec ledgr = GPIO_DT_SPEC_GET(LEDGR_NODE, gpios);
const struct device *uart_1 = DEVICE_DT_GET(UART1_NODE);

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
#define MSG_SIZE 32
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* UART data buffer */
static char rx_buf[MSG_SIZE];
static char tx_buf[MSG_SIZE];
static char rx_msg_buf[MSG_SIZE];
static int rx_buf_pos=0;
static int tx_buf_pos=0;
static int tx_buf_size=0;


 

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
 * Print a null-terminated string character by character to the UART interface in a polled manner
 *  */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	tx_buf_pos=0;
	tx_buf_size=0;

	//for (int i = 0; i < msg_len && i < MSG_SIZE; i++)
	for (int i = 0; i < msg_len ; i++)
	{
		tx_buf[tx_buf_size++] = *buf;
		buf++;
	}
	uart_irq_tx_enable(uart_1);
}



/**
 * @brief  the serial callback
 * Interrupts here for each received character. Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c = 0;

	if (uart_irq_update(dev) && uart_irq_is_pending(dev) )
	{
		if (uart_irq_tx_ready(dev)  && (tx_buf_size > 0)  )
		{
			//if (uart_fifo_fill(dev, (uint8_t *)&tx_buf[tx_buf_pos++],1 ) > 0 )
			if (uart_fifo_fill(dev, (uint8_t *)(tx_buf+tx_buf_pos),1 ) > 0 )
			{
				tx_buf_size--;
				tx_buf_pos++;
			}
			if (tx_buf_size == 0) 
			{
				uart_irq_tx_disable(dev);
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

	/* Configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart_1, serial_cb, NULL);
	uart_irq_rx_enable(uart_1);
	//uart_irq_tx_enable(uart_1);


	while (1)
	{
		print_uart("Waiting for message from UART, type a message and end with <CR>\n");
		while (k_msgq_get(&uart_msgq, &rx_msg_buf, K_FOREVER) == 0)
		{
			print_uart("Message:");
			k_msleep(100);
			print_uart(rx_msg_buf);
			k_msleep(100);
			print_uart("\r\n");
			k_msleep(100);
			gpio_pin_toggle_dt(&ledgr);
		}
	}
}
