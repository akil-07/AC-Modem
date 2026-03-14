import os

def build_ino():
    with open('UI_Preview.html', 'r', encoding='utf-8') as f:
        html = f.read()

    with open('manifest.json', 'r', encoding='utf-8') as f:
        manifest = f.read()

    with open('sw.js', 'r', encoding='utf-8') as f:
        sw = f.read()

    with open('icon.svg', 'r', encoding='utf-8') as f:
        icon = f.read()

    ino_content = f"""#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>

// Replace with your Network Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

ESP8266WebServer server(80);

String scheduledTime = "";
bool scheduleTriggered = false;

// Pin Definitions for BC547 Transistor Base connections (via 1k resistor)
const int PIN_POWER = D1;      // Power Button
const int PIN_TEMP_UP = D2;    // Temperature UP Button
const int PIN_TEMP_DOWN = D5;  // Temperature DOWN Button

// Function to simulate a button press
void pressButton(int pin) {{
  digitalWrite(pin, HIGH); // Send signal to Transistor base
  delay(200);              // Hold the button for 200 ms
  digitalWrite(pin, LOW);  // Release button
  Serial.print("Pressed pin: ");
  Serial.println(pin);
}}

// PROGMEM Declarations for PWA Web Assets
const char html_page[] PROGMEM = R"rawliteral({html})rawliteral";
const char manifest_json[] PROGMEM = R"rawliteral({manifest})rawliteral";
const char sw_js[] PROGMEM = R"rawliteral({sw})rawliteral";
const char icon_svg[] PROGMEM = R"rawliteral({icon})rawliteral";

// Route handlers
void handlePower() {{ pressButton(PIN_POWER); server.send(200, "text/plain", "Power Button Pressed"); }}
void handleTempUp() {{ pressButton(PIN_TEMP_UP); server.send(200, "text/plain", "Temp Up Pressed"); }}
void handleTempDown() {{ pressButton(PIN_TEMP_DOWN); server.send(200, "text/plain", "Temp Down Pressed"); }}
void handleSchedule() {{
  if (server.hasArg("time")) {{
    scheduledTime = server.arg("time");
    scheduleTriggered = false;
    server.send(200, "text/plain", "Scheduled for " + scheduledTime);
    Serial.println("Scheduled for " + scheduledTime);
  }} else {{
    server.send(400, "text/plain", "Bad Request");
  }}
}}
void handleNotFound() {{ server.send(404, "text/plain", "Not Found"); }}

void setup() {{
  Serial.begin(115200);
  pinMode(PIN_POWER, OUTPUT); pinMode(PIN_TEMP_UP, OUTPUT); pinMode(PIN_TEMP_DOWN, OUTPUT);
  digitalWrite(PIN_POWER, LOW); digitalWrite(PIN_TEMP_UP, LOW); digitalWrite(PIN_TEMP_DOWN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {{ delay(500); Serial.print("."); }}
  Serial.println(""); Serial.print("Connected to "); Serial.println(ssid);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST Timezone offset: 5.5 * 3600 = 19800

  // Serve static UI assets from PROGMEM
  server.on("/", []() {{ server.send_P(200, "text/html", html_page); }});
  server.on("/UI_Preview.html", []() {{ server.send_P(200, "text/html", html_page); }});
  server.on("/index.html", []() {{ server.send_P(200, "text/html", html_page); }});
  server.on("/manifest.json", []() {{ server.send_P(200, "application/json", manifest_json); }});
  server.on("/sw.js", []() {{ server.send_P(200, "application/javascript", sw_js); }});
  server.on("/icon.svg", []() {{ server.send_P(200, "image/svg+xml", icon_svg); }});

  // Control APIs
  server.on("/power", handlePower);
  server.on("/temp_up", handleTempUp);
  server.on("/temp_down", handleTempDown);
  server.on("/schedule", handleSchedule);
  
  // Dummy handlers for other un-implemented endpoints to avoid 404s in console
  server.on("/timer_on", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/timer_off", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/clock", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/swing", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/energy_save", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/turbo", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/sleep", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/light", [](){{ server.send(200, "text/plain", "OK"); }});
  server.on("/temp_btn", [](){{ server.send(200, "text/plain", "OK"); }});

  server.onNotFound(handleNotFound);

  server.begin(); Serial.println("HTTP server started");
}}

void loop() {{ 
  server.handleClient(); 
  
  if (scheduledTime != "" && !scheduleTriggered) {{
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Check if time is properly synced (year > 1970)
    if (timeinfo->tm_year > 70) {{
      char timeStr[6];
      sprintf(timeStr, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
      
      if (String(timeStr) == scheduledTime) {{
        Serial.println("Schedule Triggered! Turning ON AC.");
        pressButton(PIN_POWER);
        scheduleTriggered = true; // Prevent running multiple times in same minute
      }}
    }}
  }}
}}
"""

    with open('AC_Controller/AC_Controller.ino', 'w', encoding='utf-8') as f:
        f.write(ino_content)

    print("Updated AC_Controller.ino with PWA assets!")

if __name__ == "__main__":
    build_ino()
