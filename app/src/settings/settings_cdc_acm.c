/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);


RING_BUF_DECLARE(uart_rx_buf, CONFIG_ZMK_SETTINGS_CDC_ACM_RING_BUF_SIZE);
RING_BUF_DECLARE(uart_tx_buf, CONFIG_ZMK_SETTINGS_CDC_ACM_RING_BUF_SIZE);

const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
/* Work item, used to signal to settings subsystem data is ready to be read */
extern struct k_work zmk_settings_work;

void interrupt_handler(const struct device *dev, void *user_data) {
	int read;
	size_t wrote, len;
	uint8_t buf[64];

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			read = uart_fifo_read(dev, buf, sizeof(buf));
			if (read < 0) {
				LOG_ERR("Failed to read uart FIFO");
				read = 0;
			}

			wrote = ring_buf_put(&uart_rx_buf, buf, read);
			if (wrote < read) {
				LOG_ERR("Drop %zu bytes", (read - wrote));
			}
			k_work_submit(&zmk_settings_work);
		}

		if (uart_irq_tx_ready(dev)) {
			len = ring_buf_get(&uart_tx_buf, buf, sizeof(buf));
			if (!len) {
				uart_irq_tx_disable(dev);
			} else {
				wrote = uart_fifo_fill(dev, buf, len);
			}
		}
	}
}

static void uart_line_set(const struct device *dev)
{
	uint32_t baudrate;
	int ret;

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	if (ret) {
		LOG_DBG("Failed to set DCD, ret code %d", ret);
	}

	ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
	if (ret) {
		LOG_DBG("Failed to set DSR, ret code %d", ret);
	}

	/* Wait 1 sec for the host to do all settings */
	k_sleep(K_MSEC(1000));

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_DBG("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_DBG("Baudrate detected: %d", baudrate);
	}
}

/*
 * Setup UART as a backend to communicate with the host PC
 */
void settings_transport_init(void) {
	uint32_t dtr = 0U;

	LOG_INF("ZMK settings uart transport started");

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART CDC device not ready");
		return;
	}

	LOG_INF("Wait for DTR");
	while (1) {
		uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}

		k_sleep(K_MSEC(100));
	}

	LOG_INF("DTR set");

	uart_irq_callback_set(uart_dev, interrupt_handler);
	uart_line_set(uart_dev);
	uart_irq_rx_enable(uart_dev);
}


/*
 * Writes a buffer payload to the host PC
 * @param buf: buffer of data to write
 * @param len: length of buffer in bytes
 * @retval number of bytes written, or negative value on error.
 */
int settings_transport_write(const uint8_t *buf, size_t len) {
	int written;

	written = ring_buf_put(&uart_tx_buf, buf, len);
	if (written == ring_buf_size_get(&uart_tx_buf)) {
		/* TX irq is stopped. Enable it */
		uart_irq_tx_enable(uart_dev);
	}
	return written;
}

/*
 * Reads a buffer payload from the host PC
 * @param buf: buffer to read into
 * @param len: length of buffer in bytes
 * @retval number of bytes read, or negative value on error.
 */
int settings_transport_read(uint8_t *buf, size_t len) {
	return ring_buf_get(&uart_rx_buf, buf, len);
}
