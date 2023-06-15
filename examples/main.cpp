#include <Arduino.h>
#include <BoardInfo.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define XPOWERS_CHIP_AXP2101
#include <XPowersLib.h>

#define DUMP_AT_COMMANDS
#define TINY_GSM_RX_BUFFER 1024
#define TINY_GSM_MODEM_SIM7080
#include <TinyGsmClient.h>

// BLE Service and Characteristic UUIDs
#define SERVICE_UUID "0000FFFF-0000-1000-8000-00805F9B34FB"
#define COMMAND_UUID "0000FF01-0000-1000-8000-00805F9B34FB"
#define SYSTEM_HEALTH_UUID "0000FF02-0000-1000-8000-00805F9B34FB"

BLEServer *bleServer;
BLEService *service;
BLECharacteristic *commandCharacteristic;
BLECharacteristic *systemHealthCharacteristic;

XPowersPMU PMU;
TinyGsm modem(Serial);

bool deviceLocked = false;

class CommandCallback : public BLECharacteristicCallbacks
{
	void onWrite(BLECharacteristic *characteristic)
	{
		std::string value = characteristic->getValue();

		if (value.length() > 0)
		{
			// Handle command received from the mobile app
			if (value[0] == 'L')
			{
				deviceLocked = true;
				// Perform lock operation
			}
			else if (value[0] == 'U')
			{
				deviceLocked = false;
				// Perform unlock operation
			}
		}
	}
};

void setup()
{
	// Initialize BLE
	BLEDevice::init("ESP32S3_Device");

	// Create BLE server
	bleServer = BLEDevice::createServer();

	// Create BLE service
	service = bleServer->createService(SERVICE_UUID);

	// Create command characteristic
	commandCharacteristic = service->createCharacteristic(
			COMMAND_UUID,
			BLECharacteristic::PROPERTY_WRITE);
	commandCharacteristic->setCallbacks(new CommandCallback());

	// Create system health characteristic
	systemHealthCharacteristic = service->createCharacteristic(
			SYSTEM_HEALTH_UUID,
			BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

	// Set initial system health value
	systemHealthCharacteristic->setValue("System Health Information");

	// Start service
	service->start();

	// Start advertising
	bleServer->getAdvertising()->start();

	// Initialize power management
	if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL))
	{
		Serial.println("Failed to initialize power.....");
		while (1)
		{
			delay(5000);
		}
	}
	// Set the working voltage of the modem, please do not modify the parameters
	PMU.setDC3Voltage(3000); // SIM7080 Modem main power channel 2700~ 3400V
	PMU.enableDC3();

	// Modem GPS Power channel
	PMU.setBLDO2Voltage(3300);
	PMU.enableBLDO2(); // The antenna power must be turned on to use the GPS
	// function

	// TS Pin detection must be disable, otherwise it cannot be charged
	PMU.disableTSPinMeasure();

	Serial1.begin(115200, SERIAL_8N1, BOARD_MODEM_RXD_PIN, BOARD_MODEM_TXD_PIN);

	pinMode(BOARD_MODEM_PWR_PIN, OUTPUT);
	pinMode(BOARD_MODEM_DTR_PIN, OUTPUT);
	pinMode(BOARD_MODEM_RI_PIN, INPUT);

	int retry = 0;
	while (!modem.testAT(1000))
	{
		Serial.print(".");
		if (retry++ > 6)
		{
			// Pull down PWRKEY for more than 1 second according to manual
			// requirements
			digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
			delay(100);
			digitalWrite(BOARD_MODEM_PWR_PIN, HIGH);
			delay(1000);
			digitalWrite(BOARD_MODEM_PWR_PIN, LOW);
			retry = 0;
			Serial.println("Retry start modem .");
		}
	}
	Serial.println();
	Serial.print("Modem started!");
}

void loop() {}
