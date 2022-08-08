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
const struct device *uart_0 = DEVICE_DT_GET(UART1_NODE);

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
#define MSG_SIZE 32
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* UART data buffer */
static char rx_buf[MSG_SIZE];
static char tx_buf[MSG_SIZE];
static int rx_buf_pos;

/**
 * @brief
 * Print a null-terminated string character by character to the UART interface in a polled manner
 *  */
void print_uart(char *buf)
{
	int msg_len = strlen(buf);
	for (int i = 0; i < msg_len; i++)
	{
		uart_poll_out(uart_0, buf[i]);
	}
}

/**
 * @brief  the serial callback
 * Interrupts here for each received character. Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c = 0;

	if (!uart_irq_update(uart_0))
	{
		return;
	}

	gpio_pin_toggle_dt(&ledgr);

	while (uart_irq_rx_ready(uart_0))
	{

		uart_fifo_read(uart_0, &c, 1);

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

void main(void)
{
	int ret;

	if (!device_is_ready(ledgr.port))
	{
		// do some failed thing
	}

	if (!device_is_ready(uart_0))
	{
		// do some failed thing
	}

	ret = gpio_pin_configure_dt(&ledgr, GPIO_OUTPUT_ACTIVE);
	if (ret < 0)
	{
		// do some failed thing
	}

	/* Configure interrupt and callback to receive data */
	uart_irq_callback_user_data_set(uart_0, serial_cb, NULL);
	uart_irq_rx_enable(uart_0);

	while (1)
	{
		print_uart("Waiting for message from UART, type a message and end with <CR>\n");
		while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0)
		{
			print_uart("Message:");
			print_uart(tx_buf);
			print_uart("\r\n");
			gpio_pin_toggle_dt(&ledgr);
		}
	}
}
