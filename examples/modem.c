#include "esp_log.h"
#include "esp_modem_api.h"

#define MODEM_BAUD_RATE 9600
#define UART_PORT_NUM UART_NUM_1
#define UART_TX_PIN 5		// connected to SIM7080g RXD
#define UART_RX_PIN 4		// connected to SIM7080g TXD
#define UART_RTS_PIN 42 // connected to SIM7080g DTR
#define UART_CTS_PIN 3	// connected to SIM7080g RI

// Replace these with your APN details
#define APN "your_apn"
#define APN_USER "apn_username"
#define APN_PASS "apn_password"

static const char *TAG = "modem_example";

esp_modem_dce_t *modem_init(void)
{
	/* Create an instance of esp_modem_dte_config_t and set its parameters */
	esp_modem_dte_config_t config = ESP_MODEM_DTE_DEFAULT_CONFIG();
	/* setup UART pins */
	config.uart_config.tx_io_num = UART_TX_PIN;
	config.uart_config.rx_io_num = UART_RX_PIN;
	config.uart_config.rts_io_num = UART_RTS_PIN;
	config.uart_config.cts_io_num = UART_CTS_PIN;
	config.uart_config.rx_buffer_size = 1024;
	/* setup UART driver */
	esp_modem_dce_device
			esp_modem_dte_t *dte = esp_modem_dte_init(&config);

	/* Configure a DCE module (SIM7080 in this case) */
	esp_modem_dce_t *dce = sim7080_init(dte);
	esp_modem_dce_set_flow_ctrl(dce, MODEM_FLOW_CONTROL_NONE);
	esp_modem_dce_set_parity(dce, MODEM_PARITY_DISABLE);
	esp_modem_dce_set_stop_bits(dce, MODEM_STOP_BITS_1);

	/* Initialize the modem */
	if (esp_modem_dce_init(dce) != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to init modem DCE");
		free(dce);
		return NULL;
	}

	return dce;
}

esp_err_t check_sim_card(esp_modem_dce_t *dce)
{
	char response[64];
	esp_err_t err = esp_modem_dce_at_command(dce, "AT+CPIN?");
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to send command");
		return err;
	}

	err = esp_modem_dce_read(dce, response, sizeof(response));
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to read response");
		return err;
	}

	if (strstr(response, "+CPIN: READY"))
	{
		ESP_LOGI(TAG, "SIM card inserted and ready");
		return ESP_OK;
	}
	else
	{
		ESP_LOGE(TAG, "SIM card not ready");
		return ESP_FAIL;
	}
}

esp_err_t register_to_network(esp_modem_dce_t *dce)
{
	esp_err_t err = esp_modem_dce_at_command(dce, "AT+CREG=1");
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to send command AT+CREG=1");
		return err;
	}

	ESP_LOGI(TAG, "Network registration requested");

	char response[64];
	int attempts = 20; // Adjust this to change the maximum number of attempts
	while (attempts-- > 0)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay between checks

		err = esp_modem_dce_at_command(dce, "AT+CREG?");
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "Failed to send command AT+CREG?");
			return err;
		}

		err = esp_modem_dce_read(dce, response, sizeof(response));
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "Failed to read response");
			return err;
		}

		if (strstr(response, "+CREG: 0,1") || strstr(response, "+CREG: 0,5"))
		{
			ESP_LOGI(TAG, "Network registration succeeded");
			return ESP_OK;
		}
	}

	ESP_LOGE(TAG, "Network registration not successful");
	return ESP_FAIL;
}

esp_err_t establish_ppp_connection(esp_modem_dce_t *dce, const char *apn, const char *username, const char *password)
{
	char cmd[64];

	/* Configure APN */
	snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
	esp_err_t err = esp_modem_dce_at_command(dce, cmd);
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set APN");
		return err;
	}

	/* Configure username */
	if (username)
	{
		snprintf(cmd, sizeof(cmd), "AT+CGAUTH=1,1,\"%s\"", username);
		err = esp_modem_dce_at_command(dce, cmd);
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "Failed to set username");
			return err;
		}
	}

	/* Configure password */
	if (password)
	{
		snprintf(cmd, sizeof(cmd), "AT+CGAUTH=1,1,\"%s\"", password);
		err = esp_modem_dce_at_command(dce, cmd);
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "Failed to set password");
			return err;
		}
	}

	/* Start dial-up */
	err = esp_modem_dce_at_command(dce, "ATD*99#");
	if (err != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to start PPP dial-up");
		return err;
	}

	ESP_LOGI(TAG, "PPP connection established");

	return ESP_OK;
}
