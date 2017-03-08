#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "brzo_i2c.h"

#ifndef SDA_PIN
#define SDA_PIN 5
#endif

#ifndef SCL_PIN
#define SCL_PIN 4
#endif

#ifndef BH1750_ADR
#define  BH1750_ADR 0x23
#endif

const char* ssid = "ra";
const char* password = "xxxxxxxx";

uint8_t buffer[2];

uint8_t ICACHE_RAM_ATTR get_BH1750_value(uint8_t *sens_val) {
	uint32_t ADC_code = 0;
	uint8_t	bcode = 0;

	brzo_i2c_start_transaction(BH1750_ADR, 400);

  // Receives sensor value
	brzo_i2c_read(buffer, 2, false);

	bcode = brzo_i2c_end_transaction();

	uint8_t raw_val = ((buffer[0] << 8) | buffer[1]);

  *sens_val=raw_val;

	return bcode;
}

MDNSResponder mdns;

ESP8266WebServer server(80);

uint8_t error = 0;
uint8_t sens_val = 0;

void handleRoot() {
  Serial.println("Request recieved");

  error=get_BH1750_value(&sens_val);

  if (error == 0) {
		Serial.print("Lux = ");
		Serial.println(sens_val);

		String response="{'lux':";
		response=response+sens_val+"}";

		server.send(200, "application/json", response);
	}
	else {
		Serial.print("Brzo error : ");
		Serial.println(error);

		String response="{'error':";
		response=response+error+"}";
    server.send(200, "application/json", "{'error':}");
	}
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}



void setup() {
  delay(1000);
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Starting...");

  // Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  brzo_i2c_setup(SDA_PIN, SCL_PIN, 2000);

  // Send "Continuously H-resolution mode " instruction
  brzo_i2c_start_transaction(BH1750_ADR, 400);

  buffer[0] = 0x10;
  brzo_i2c_write(buffer, 1, false);

  brzo_i2c_end_transaction();
}

void loop() {
	server.handleClient();
}
