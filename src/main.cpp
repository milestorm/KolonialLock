#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <config.h>
// #include <avdweb_VirtualDelay.h>
// #include <ToneSfx.h>
// #include <sounds.h>
#include <SPI.h>
#include <MFRC522.h>
#include <rootCer.h>

#define LEDC_CHANNEL_0    0
#define LEDC_TIMER_8_BIT  8
#define LEDC_BASE_FREQ    5000

// WiFiClient client;
WiFiClientSecure client; // Use WiFiClientSecure for HTTPS
HTTPClient https;

// ToneSfx toneSfxSound(BEEPER_PIN);
MFRC522 mfrc522(SS_PIN, RST_PIN);

bool ledState = false;
unsigned long previousMillis = 0; // Stores last time LED was updated
const long onInterval = 50;   // LED on duration in milliseconds
const long offInterval = 3000; // LED off duration in milliseconds

uint32_t mfrcLastMillis = 0;

// const char* ssid = "ms-home";
// const char* password = "";

String adminTokenIds[3] = {"04131e4fea7480", "044c1016196c80", "045eb406496c80"}; // Admin Tokens: Dejf, Jarda, Hirek

const char* ssid = "szn-Devices"; // Your network SSID (name)
const char* username = "3b4719"; // Your WPA2 Enterprise username
const char* password = "KjQuKiQa"; // Your WPA2 Enterprise password

String serverPath = "https://milestorm.net/kolonial/php/openDoor.php?cardId=";

// Helper routine to dump a byte array as hex values to Serial
void dump_byte_array(byte *buffer, byte bufferSize) {
	for (byte i = 0; i < bufferSize; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], HEX);
	}
}

String dumpByteArraToString(byte *buffer, byte bufferSize) {
	String output;
	for (byte i = 0; i < bufferSize; i++) {
		if (buffer[i] < 0x10) {
			output += "0";
		}
		output += String(buffer[i], HEX);
	}
	return output;
}

bool isAdminToken(const String& tokenId) {
	for (int i = 0; i < 3; i++) {
		if (tokenId == adminTokenIds[i]) {
			return true; // Token ID is found in the array
		}
	}
	return false; // Token ID is not found in the array
}

void initRC522() {
	SPI.begin(); // Init SPI bus
	// SPI.setClockDivider(SPI_CLOCK_DIV16);
	mfrc522.PCD_Init(); // Init MFRC522
	delay(50);
	// digitalWrite(LED_RED_PIN, LOW);
}

void killRC522() {
	digitalWrite(RST_PIN, LOW);
	// digitalWrite(LED_RED_PIN, HIGH);
}

void allLedsOff() {
	digitalWrite(LED_RED_PIN, LOW);
	digitalWrite(LED_GREEN_PIN, LOW);
	digitalWrite(LED_BLUE_PIN, LOW);
}

void ledWhiteOn() {
	allLedsOff();
	digitalWrite(LED_RED_PIN, HIGH);
	digitalWrite(LED_GREEN_PIN, HIGH);
	digitalWrite(LED_BLUE_PIN, HIGH);
}

void ledCyanOn() {
	allLedsOff();
	digitalWrite(LED_GREEN_PIN, HIGH);
	digitalWrite(LED_BLUE_PIN, HIGH);
}

void ledOrangeOn() {
	allLedsOff();
	digitalWrite(LED_RED_PIN, HIGH);
	digitalWrite(LED_GREEN_PIN, HIGH);
}

void ledVioletOn() {
	allLedsOff();
	digitalWrite(LED_RED_PIN, HIGH);
	digitalWrite(LED_BLUE_PIN, HIGH);
}

void ledRedOn() {
	allLedsOff();
	digitalWrite(LED_RED_PIN, HIGH);
}

void ledGreenOn() {
	allLedsOff();
	digitalWrite(LED_GREEN_PIN, HIGH);
}

void ledBlueOn() {
	allLedsOff();
	digitalWrite(LED_BLUE_PIN, HIGH);
}

void playStartupMelody() {
	tone(BEEPER_PIN, 659, 125);
	delay(125);
	tone(BEEPER_PIN, 784, 125);
	delay(125);
	tone(BEEPER_PIN, 1175, 125);
	delay(125);
	tone(BEEPER_PIN, 988, 375);
	delay(375);
	noTone(BEEPER_PIN);
}

void playOpenMelody() {
	tone(BEEPER_PIN, 1047, 62);
	delay(62);
	tone(BEEPER_PIN, 1397, 250);
	delay(250);
	noTone(BEEPER_PIN);
}

void playErrorMelody() {
	tone(BEEPER_PIN, 262, 62);
	delay(128);
	tone(BEEPER_PIN, 262, 62);
	delay(128);
	tone(BEEPER_PIN, 262, 62);
	delay(128);
	noTone(BEEPER_PIN);
}

void playTimeoutMelody() {
	tone(BEEPER_PIN, 262, 62);
	delay(128);
	tone(BEEPER_PIN, 196, 62);
	delay(190);
	noTone(BEEPER_PIN);
}

void openTheDoor() {
	ledGreenOn();
	playOpenMelody();
	delay(200);

	// opens the lock for a short time
	digitalWrite(LOCK_PIN, HIGH);
	delay(3000);
	digitalWrite(LOCK_PIN, LOW);
}

void processRfidCard() {
	// Show some details of the PICC (that is: the tag/card)
	Serial.print(F("Card UID:"));
	dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
	Serial.println();

	ledBlueOn();

	String scannedTokenId = dumpByteArraToString(mfrc522.uid.uidByte, mfrc522.uid.size);

	if (isAdminToken(scannedTokenId)) {
		Serial.println("Token is an admin token.");
		openTheDoor();
	} else {
		String serverPathWithParam = serverPath + scannedTokenId;

		HTTPClient https;

		if (https.begin(client, serverPathWithParam.c_str())) { // Specify the URL
			int httpCode = https.GET(); // Make the request

			if (httpCode > 0) { // Check for the returning code
				String payload = https.getString();
				Serial.println(httpCode);
				Serial.println(payload);

				if (httpCode == 200) {
					if (payload == "AccesGranted") {
						openTheDoor();
					} else {
						ledRedOn();
						playErrorMelody();
					}
				} else {
					ledRedOn();
					playErrorMelody();
				}
			} else {
				Serial.println("Error on HTTP request");
				Serial.println(httpCode);
				ledRedOn();
				playTimeoutMelody();
			}

			https.end(); // Free the resources
		}
	}

	allLedsOff();

	// Halt PICC
	mfrc522.PICC_HaltA();
	// Stop encryption on PCD
	// mfrc522.PCD_StopCrypto1();
}

void ledBlinkTick() {
	unsigned long currentMillis = millis();
	unsigned long interval = ledState ? onInterval : offInterval;

	if (currentMillis - previousMillis >= interval) {
		// Save the last time you toggled the LED
		previousMillis = currentMillis;

		// If the LED is off turn it on and vice-versa:
		ledState = !ledState;
		digitalWrite(LED_RED_PIN, ledState ? HIGH : LOW);
		digitalWrite(LED_GREEN_PIN, ledState ? HIGH : LOW);
		digitalWrite(LED_BLUE_PIN, ledState ? HIGH : LOW);
	}
}

void connectToWifi() {
	// WPA2 Enterprise connect
	WiFi.disconnect(true);  // Ensure WiFi is disconnected and start fresh
  	WiFi.mode(WIFI_MODE_STA);
	Serial.println("Connecting to WPA2 Enterprise");
	// Use the WiFi class's begin method for WPA2 Enterprise
	// Note: The last two parameters (anonymousIdentity and caCert) are optional and can be omitted if not used/required.
	WiFi.begin(ssid, WPA2_AUTH_PEAP, username, username, password); // For WPA2 Enterprise, the begin method can be overloaded with these parameters

	// WPA2 normal
	// WiFi.mode(WIFI_MODE_STA);
	// WiFi.begin(ssid, password);
	// Serial.println("Connecting to WiFi");

	while(WiFi.status() != WL_CONNECTED) {
		// WL_IDLE_STATUS      = 0,
		// WL_NO_SSID_AVAIL    = 1,
		// WL_SCAN_COMPLETED   = 2,
		// WL_CONNECTED        = 3,
		// WL_CONNECT_FAILED   = 4,
		// WL_CONNECTION_LOST  = 5,
		// WL_DISCONNECTED     = 6
		ledVioletOn();
		delay(350);
		allLedsOff();
		delay(350);
		for (size_t i = 0; i < WiFi.status(); i++) {
			ledCyanOn();
			delay(200);
			allLedsOff();
			delay(200);
		}
		Serial.println(WiFi.status());
		Serial.print(".");

		// TODO
		// kdyz se to zasekne na nejakym statusu.. treba dlouho na 6ce, tak pouzit ESP.restart() k restartovani ESPcka...
	}
	ledGreenOn();
	Serial.println("");
	Serial.print("Connected to WiFi network with IP Address: ");
	Serial.println(WiFi.localIP());

	// For HTTPS: if you don't want to use a certificate, you can bypass verification (not secure):
	// client.setInsecure(); // Bypass certificate verification - only use for testing or trusted environments

	// Optionally, you can set the certificate for the server you're connecting to for secure connections:
	client.setCACert(rootCACertificate);

	delay(1000);
	allLedsOff();
}

void setup() {
	Serial.begin(9600);   // Initialize serial communications with the PC
	delay(1000);

	//Serial.setDebugOutput(true);

	Serial.println("== KOLONIAL LOCK ==");

	ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT);

	pinMode(LED_RED_PIN, OUTPUT);
	pinMode(LED_GREEN_PIN, OUTPUT);
	pinMode(LED_BLUE_PIN, OUTPUT);
	pinMode(LOCK_PIN, OUTPUT);
	pinMode(BEEPER_PIN, OUTPUT);

	connectToWifi();

	playStartupMelody();
}

void loop() {
	if (WiFi.status() != WL_CONNECTED) {
		connectToWifi();
	}

	// https://community.particle.io/t/getting-the-rfid-rc522-to-work-solved/3571/282
	// Initialize RFID reader
	initRC522();

	// Reset last ms timer
	mfrcLastMillis = millis();

	// Wait up to N ms for card reading
	while (millis() - mfrcLastMillis < 5000) {
		// Look for a new NFC card to be present
		if (!mfrc522.PICC_IsNewCardPresent()) {
			continue;
		}
		// Attempt to read the card serial number (uid)
		if (!mfrc522.PICC_ReadCardSerial()) {
			continue;
		}

		processRfidCard();
		break;
	}

	killRC522();

	// ledBlueOn();
	delay(50);
	// allLedsOff();

}
