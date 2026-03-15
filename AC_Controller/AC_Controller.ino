#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Access Point credentials (ESP creates its own WiFi)
const char* apSSID     = "Akil_AC_Control";
const char* apPassword = "12345678";

ESP8266WebServer server(80);

String scheduledTime = "";
bool scheduleTriggered = false;

// Pin Definitions for BC547 Transistor Base connections (via 1k resistor)
const int PIN_POWER = D1;      // Power Button
const int PIN_TEMP_UP = D2;    // Temperature UP Button
const int PIN_TEMP_DOWN = D5;  // Temperature DOWN Button

// Function to simulate a button press
void pressButton(int pin) {
  digitalWrite(pin, HIGH); // Send signal to Transistor base
  delay(200);              // Hold the button for 200 ms
  digitalWrite(pin, LOW);  // Release button
  Serial.print("Pressed pin: ");
  Serial.println(pin);
}

// Web assets are stored in web_data.h to prevent Arduino IDE preprocessor
// from misreading JavaScript keywords as C++ function declarations.
#include "web_data.h"

// Route handlers
void handlePower()    { pressButton(PIN_POWER);     server.send(200, "text/plain", "Power Button Pressed"); }
void handleTempUp()   { pressButton(PIN_TEMP_UP);   server.send(200, "text/plain", "Temp Up Pressed"); }
void handleTempDown() { pressButton(PIN_TEMP_DOWN); server.send(200, "text/plain", "Temp Down Pressed"); }
void handleSchedule() {
  if (server.hasArg("time")) {
    scheduledTime = server.arg("time");
    scheduleTriggered = false;
    server.send(200, "text/plain", "Scheduled for " + scheduledTime);
    Serial.println("Scheduled for " + scheduledTime);
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}
void handleNotFound() { server.send(404, "text/plain", "Not Found"); }

void setup() {
  Serial.begin(115200);
  pinMode(PIN_POWER, OUTPUT); pinMode(PIN_TEMP_UP, OUTPUT); pinMode(PIN_TEMP_DOWN, OUTPUT);
  digitalWrite(PIN_POWER, LOW); digitalWrite(PIN_TEMP_UP, LOW); digitalWrite(PIN_TEMP_DOWN, LOW);

  // Start in AP mode — ESP creates its own WiFi hotspot
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started");
  Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP()); // default: 192.168.4.1

  // Serve UI assets from PROGMEM
  server.on("/",            []() { server.send_P(200, "text/html",        html_page);    });
  server.on("/index.html",  []() { server.send_P(200, "text/html",        html_page);    });
  server.on("/manifest.json",[]() { server.send_P(200, "application/json", manifest_json); });
  server.on("/sw.js",       []() { server.send_P(200, "application/javascript", sw_js);  });
  server.on("/icon.svg",    []() { server.send_P(200, "image/svg+xml",    icon_svg);     });

  // Control APIs — match index.html exactly
  server.on("/power",    handlePower);
  server.on("/tempup",   handleTempUp);    // matches index.html /tempup
  server.on("/tempdown", handleTempDown);  // matches index.html /tempdown
  server.on("/schedule", handleSchedule);

  // Stub handlers for extra buttons (avoid 404s)
  auto ok = [](){ server.send(200, "text/plain", "OK"); };
  server.on("/timer_on",   ok); server.on("/timer_off",  ok);
  server.on("/clock",      ok); server.on("/swing",       ok);
  server.on("/energy_save",ok); server.on("/turbo",       ok);
  server.on("/sleep",      ok); server.on("/light",       ok);
  server.on("/temp_btn",   ok);

  server.onNotFound(handleNotFound);
  server.begin(); Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  // Note: Schedule feature requires NTP (internet). In AP mode the ESP has
  // no internet, so schedule is accepted but won't auto-trigger by time.
  // To enable scheduling, switch to WIFI_STA mode and restore configTime().
}
