#include <ESP8266WiFi.h>
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
void pressButton(int pin) {
  digitalWrite(pin, HIGH); // Send signal to Transistor base
  delay(200);              // Hold the button for 200 ms
  digitalWrite(pin, LOW);  // Release button
  Serial.print("Pressed pin: ");
  Serial.println(pin);
}

// PROGMEM Declarations for PWA Web Assets
const char html_page[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  
  <!-- PWA Meta Tags -->
  <meta name="theme-color" content="#ffffff">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="default">
  <meta name="apple-mobile-web-app-title" content="AC Control">
  <link rel="apple-touch-icon" href="./icon.svg">
  <link rel="manifest" href="./manifest.json">
  
  <title>Smart AC Controller</title>
  <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: 'Inter', sans-serif;
      background-color: #f4f5f7;
      display: flex; justify-content: center; align-items: flex-start;
      min-height: 100vh; color: #111827; padding-top: 20px;
    }
    .app-container {
      background-color: #ffffff;
      width: 100%; max-width: 400px;
      min-height: 800px;
      padding: 40px 24px;
      border-radius: 40px; box-shadow: 0 30px 60px rgba(0,0,0,0.08);
      position: relative;
    }
    header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; }
    header h2 { font-size: 16px; font-weight: 700; }
    .icon-btn {
      width: 44px; height: 44px; border-radius: 50%; border: 1px solid #f3f4f6;
      background: #ffffff; display: flex; justify-content: center; align-items: center;
      cursor: pointer; color: #374151; box-shadow: 0 2px 10px rgba(0,0,0,0.02);
    }
    .title-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px; }
    h1 { font-size: 26px; font-weight: 800; }

    /* Power Button */
    .power-btn {
      width: 48px; height: 48px; border-radius: 50%; border: none;
      background: #ef4444; color: white; display: flex; justify-content: center; align-items: center;
      cursor: pointer; box-shadow: 0 4px 15px rgba(239, 68, 68, 0.4); transition: all 0.2s;
    }
    .power-btn.on { background: #10b981; box-shadow: 0 4px 15px rgba(16, 185, 129, 0.4); }
    .power-btn:active { transform: scale(0.92); }
    .power-btn svg { width: 22px; height: 22px; }

    /* Modes */
    .modes { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; margin-bottom: 40px; }
    .mode-btn {
      display: flex; flex-direction: column; align-items: center; justify-content: center;
      padding: 16px 0; border-radius: 20px; border: none; background-color: #ffffff;
      box-shadow: 0 4px 15px rgba(0,0,0,0.04), inset 0 0 0 1px #f3f4f6; cursor: pointer; color: #9ca3af; transition: all 0.2s;
    }
    .mode-btn svg { width: 24px; height: 24px; margin-bottom: 8px; }
    .mode-btn span { font-size: 13px; font-weight: 600; }
    .mode-btn.active { background-color: #3b82f6; color: white; box-shadow: 0 8px 20px rgba(59, 130, 246, 0.3); }

    /* Dial */
    .dial-container { position: relative; width: 260px; height: 260px; margin: 0 auto 50px auto; display: flex; justify-content: center; align-items: center; }
    .progress-ring { position: absolute; top: 0; left: 0; transform: rotate(-225deg); }
    .progress-ring__circle { transition: stroke-dashoffset 0.3s ease-out; transform: rotate(-90deg); transform-origin: 50% 50%; }
    .dial-center {
      width: 170px; height: 170px; background: white; border-radius: 50%;
      display: flex; justify-content: center; align-items: center; box-shadow: 0 10px 40px rgba(0,0,0,0.08); z-index: 2;
    }
    .dial-center h2 { font-size: 48px; font-weight: 800; letter-spacing: -2px; position: relative; }
    .dial-center h2 span { font-size: 24px; position: absolute; top: 5px; right: -24px; }
    .markers {
      position: absolute; width: 100%; height: 100%; top: 0; left: 0;
      /* Background markers - css trick or just use svg lines for dots */
    }
    .dial-controls {
      position: absolute; width: 280px; display: flex; justify-content: space-between; bottom: 0px; left: -10px; z-index: 10;
    }
    .temp-btn {
      width: 48px; height: 48px; border-radius: 50%; background: white; border: 1px solid #f3f4f6; color: #374151; font-size: 24px; font-weight: 300; display: flex; justify-content: center; align-items: center; cursor: pointer; box-shadow: 0 4px 10px rgba(0,0,0,0.05);
    }
    .temp-btn:active { transform: scale(0.95); }

    /* Extra Controls */
    .controls-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
    .controls-header h2 { font-size: 20px; font-weight: 800; }
    .controls-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; margin-bottom: 20px; }
    .ctrl-btn {
      background-color: #ffffff; border: 1px solid #e5e7eb; border-radius: 16px; padding: 14px 0;
      font-size: 12px; font-weight: 700; color: #4b5563; cursor: pointer;
      box-shadow: 0 4px 10px rgba(0,0,0,0.03); transition: all 0.2s; text-align: center;
    }
    .ctrl-btn:hover { background-color: #f9fafb; border-color: #d1d5db; box-shadow: 0 6px 15px rgba(0,0,0,0.05); }
    .ctrl-btn:active { transform: scale(0.96); }

    /* Schedule */
    .schedule-card {
      background: #f9fafb; border: 1px solid #e5e7eb; border-radius: 20px; padding: 16px; margin-bottom: 30px;
      display: flex; justify-content: space-between; align-items: center; box-shadow: 0 4px 10px rgba(0,0,0,0.02);
    }
    .schedule-info { display: flex; flex-direction: column; }
    .schedule-info label { font-size: 14px; font-weight: 700; color: #374151; margin-bottom: 4px; }
    .schedule-info input[type="time"] { 
      font-family: 'Inter', sans-serif; font-size: 18px; font-weight: 700; color: #3b82f6; 
      border: none; background: transparent; outline: none; cursor: pointer;
    }
    .schedule-btn {
      background: #3b82f6; color: white; border: none; padding: 12px 20px; border-radius: 14px;
      font-size: 13px; font-weight: 600; cursor: pointer; box-shadow: 0 4px 15px rgba(59, 130, 246, 0.3); transition: all 0.2s;
    }
    .schedule-btn:active { transform: scale(0.95); }

    /* Toast Notification */
    #toast {
      visibility: hidden; min-width: 200px; background-color: #111827; color: #fff; text-align: center; border-radius: 50px; padding: 12px 24px; position: fixed; z-index: 100; left: 50%; transform: translateX(-50%); bottom: 40px; font-size: 14px; font-weight: 500; opacity: 0; transition: opacity 0.3s, bottom 0.3s; box-shadow: 0 10px 25px rgba(0,0,0,0.2);
    }
    #toast.show { visibility: visible; opacity: 1; bottom: 60px; }
  </style>
</head>
<body>

<div class="app-container">
  
  <header>
    <button class="icon-btn">
      <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M15 19l-7-7 7-7"></path></svg>
    </button>
    <h2>Living Room</h2>
    <button class="icon-btn">
      <svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M15 17h5l-1.405-1.405A2.032 2.032 0 0118 14.158V11a6.002 6.002 0 00-4-5.659V5a2 2 0 10-4 0v.341C7.67 6.165 6 8.388 6 11v3.159c0 .538-.214 1.055-.595 1.436L4 17h5m6 0v1a3 3 0 11-6 0v-1m6 0H9"></path></svg>
    </button>
  </header>

  <div class="title-row">
    <h1>Air Condition</h1>
    <button id="power-btn" class="power-btn on" onclick="togglePower()">
      <svg fill="none" stroke="currentColor" stroke-width="2.5" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round">
        <path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path>
        <line x1="12" y1="2" x2="12" y2="12"></line>
      </svg>
    </button>
  </div>

  <div class="modes">
    <button class="mode-btn active" onclick="setMode('heat')">
      <svg fill="currentColor" viewBox="0 0 24 24"><path d="M12 2v2m0 16v2M4.93 4.93l1.41 1.41m11.32 11.32l1.41 1.41M2 12h2m16 0h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41M12 7a5 5 0 100 10 5 5 0 000-10z" stroke="currentColor" stroke-width="2" fill="none" stroke-linecap="round" stroke-linejoin="round"/></svg>
      <span>Heat</span>
    </button>
    <button class="mode-btn" onclick="setMode('cool')">
      <svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2v20M17 5l-5 5-5-5M17 19l-5-5-5 5M2 12h20M5 7l5 5-5 5M19 7l-5 5 5 5"/></svg>
      <span>Cool</span>
    </button>
    <button class="mode-btn" onclick="setMode('dry')">
      <svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2.69l5.66 5.66a8 8 0 11-11.31 0z"/></svg>
      <span>Dry</span>
    </button>
    <button class="mode-btn" onclick="setMode('fan')">
      <svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M12 12a2 2 0 100-4 2 2 0 000 4zm0 0a2 2 0 110 4 2 2 0 010-4zm0 0a2 2 0 10-4 0 2 2 0 004 0zm0 0a2 2 0 114 0 2 2 0 01-4 0zM12 2v6M12 16v6M2 12h6M16 12h6"/></svg>
      <span>Fan</span>
    </button>
  </div>

  <div class="dial-container">
    <svg class="progress-ring" width="260" height="260">
      <!-- Markers -->
      <g stroke="#9ca3af" stroke-width="1.5" stroke-linecap="round" opacity="0.6" transform="rotate(-45 130 130)">
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(270 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(300 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(330 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(0 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(30 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(60 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(90 130 130)" />
      </g>
      
      <!-- Arcs -->
      <circle class="progress-ring__circle-bg" stroke="#e0f2fe" stroke-width="14" fill="transparent" r="95" cx="130" cy="130" stroke-dasharray="597" stroke-dashoffset="150" stroke-linecap="round"/>
      <circle class="progress-ring__circle" stroke="#3b82f6" stroke-width="14" fill="transparent" r="95" cx="130" cy="130" stroke-dasharray="597" stroke-dashoffset="400" stroke-linecap="round"/>
      <circle id="dial-thumb" cx="130" cy="35" r="7" fill="#3b82f6" stroke="#fff" stroke-width="3" />
    </svg>
    
    <div class="dial-center">
      <h2 id="temp-display">24<span>°C</span></h2>
    </div>
    
    <div class="dial-controls">
      <button class="temp-btn" onclick="changeTemp(-1)">-</button>
      <button class="temp-btn" onclick="changeTemp(1)">+</button>
    </div>
  </div>

  <div class="controls-header">
    <h2>Smart Schedule</h2>
  </div>
  
  <div class="schedule-card">
    <div class="schedule-info">
      <label>Auto Turn-ON Time</label>
      <input type="time" id="auto-on-time" value="08:00">
    </div>
    <button class="schedule-btn" onclick="setSchedule()">Set Timer</button>
  </div>

  <div class="controls-header">
    <h2>Additional Controls</h2>
  </div>

  <div class="controls-grid">
    <button class="ctrl-btn" onclick="simulateCommand('/timer_on')">Timer On</button>
    <button class="ctrl-btn" onclick="simulateCommand('/timer_off')">Timer Off</button>
    <button class="ctrl-btn" onclick="simulateCommand('/clock')">Clock</button>
    
    <button class="ctrl-btn" onclick="simulateCommand('/swing')">Swing</button>
    <button class="ctrl-btn" onclick="simulateCommand('/energy_save')">Energy Save</button>
    <button class="ctrl-btn" onclick="simulateCommand('/turbo')">Turbo</button>
    
    <button class="ctrl-btn" onclick="simulateCommand('/sleep')">Sleep</button>
    <button class="ctrl-btn" onclick="simulateCommand('/light')">Light</button>
    <button class="ctrl-btn" onclick="simulateCommand('/temp_btn')">Temp</button>
  </div>
</div>

<div id="toast">Command Sent Successfully!</div>

<script>
  let temp = 24;
  const minTemp = 16;
  const maxTemp = 30;
  
  const circle = document.querySelector('.progress-ring__circle');
  const radius = circle.r.baseVal.value;
  const circumference = radius * 2 * Math.PI;
  const arcLength = circumference * 0.75; // 270 degrees
  
  circle.style.strokeDasharray = `${circumference} ${circumference}`;
  
  function updateDial() {
    const percent = (temp - minTemp) / (maxTemp - minTemp);
    
    // Calculate SVG stroke offset for the filled arc
    const offset = circumference - (percent * arcLength);
    circle.style.strokeDashoffset = offset + (circumference * 0.25);
    
    // Position thumb on the arc
    // Circle is rotated -90deg then its parent -225deg = -315deg
    // The visual start of the arc is at -135deg on standard 0-360 plane.
    // Let's do pure math for the thumb position relative to SVG center (130, 130)
    const startAngle = 135; // degrees from top, clockwise
    const currentAngle = startAngle + (percent * 270);
    const rad = (currentAngle - 90) * (Math.PI / 180);
    
    const cx = 130 + radius * Math.cos(rad);
    const cy = 130 + radius * Math.sin(rad);
    
    const thumb = document.getElementById('dial-thumb');
    thumb.setAttribute('cx', cx);
    thumb.setAttribute('cy', cy);
  }

  function changeTemp(delta) {
    const newTemp = temp + delta;
    if (newTemp >= minTemp && newTemp <= maxTemp) {
      temp = newTemp;
      document.getElementById('temp-display').innerHTML = `${temp}<span>°C</span>`;
      updateDial();
      
      const endpoint = delta > 0 ? '/temp_up' : '/temp_down';
      simulateCommand(endpoint);
    }
  }

  let powerState = true;
  function togglePower() {
    powerState = !powerState;
    const btn = document.getElementById('power-btn');
    if(powerState) {
      btn.classList.add('on');
    } else {
      btn.classList.remove('on');
    }
    simulateCommand('/power');
  }

  function setMode(mode) {
    document.querySelectorAll('.mode-btn').forEach(btn => btn.classList.remove('active'));
    event.currentTarget.classList.add('active');
    showToast(`Mode: ${mode.toUpperCase()}`);
  }

  function setSchedule() {
    const timeVal = document.getElementById('auto-on-time').value;
    if(!timeVal) return;
    
    showToast(`AC Scheduled for ${timeVal}`);
    
    // Convert time to a format the ESP8266 can parse easily (like HH:MM)
    fetch(`/schedule?time=${timeVal}`).catch(err => {
      // Fallback for UI testing
      !window.location.protocol.includes('http') && console.log(`Triggered: /schedule?time=${timeVal}`);
    });
  }

  function showToast(msg) {
    const toast = document.getElementById("toast");
    toast.innerText = msg;
    toast.className = "show";
    setTimeout(function(){ toast.className = toast.className.replace("show", ""); }, 2000);
  }

  async function simulateCommand(endpoint) {
    // Determine command mapping for standard HTTP endpoint
    return fetch(endpoint).then(res => {
      showToast("Sent Action!");
    }).catch(err => {
      // Ignore network errors in preview mode
      showToast("Triggered: " + endpoint.replace('/', '').toUpperCase());
    });
  }

  updateDial();

  // Register PWA Service Worker
  if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
      navigator.serviceWorker.register('./sw.js').then(reg => {
        console.log('ServiceWorker registration successful');
      }).catch(err => {
        console.log('ServiceWorker registration failed: ', err);
      });
    });
  }
</script>

</body>
</html>
)rawliteral";
const char manifest_json[] PROGMEM = R"rawliteral({
  "name": "Smart AC Controller",
  "short_name": "AC Control",
  "start_url": "./UI_Preview.html",
  "display": "standalone",
  "background_color": "#f4f5f7",
  "theme_color": "#ffffff",
  "icons": [
    {
      "src": "./icon.svg",
      "sizes": "any",
      "type": "image/svg+xml"
    },
    {
      "src": "./icon.svg",
      "sizes": "192x192",
      "type": "image/svg+xml"
    },
    {
      "src": "./icon.svg",
      "sizes": "512x512",
      "type": "image/svg+xml"
    }
  ]
}
)rawliteral";
const char sw_js[] PROGMEM = R"rawliteral(const CACHE_NAME = 'ac-controller-v1';
const ASSETS_TO_CACHE = [
  './UI_Preview.html',
  './manifest.json',
  './icon.svg',
  'https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap'
];

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => cache.addAll(ASSETS_TO_CACHE))
      .then(() => self.skipWaiting())
  );
});

self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(cacheNames => {
      return Promise.all(
        cacheNames.map(cacheName => {
          if (cacheName !== CACHE_NAME) {
            return caches.delete(cacheName);
          }
        })
      );
    }).then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', event => {
  event.respondWith(
    caches.match(event.request)
      .then(response => {
        if (response) {
          return response;
        }
        return fetch(event.request);
      })
  );
});
)rawliteral";
const char icon_svg[] PROGMEM = R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512" width="512" height="512">
  <!-- Background -->
  <rect width="512" height="512" rx="120" ry="120" fill="#3b82f6" />
  
  <!-- Outer Box (AC Unit) -->
  <rect x="96" y="160" width="320" height="140" rx="30" ry="30" fill="#ffffff" />
  
  <!-- Vents / Details -->
  <rect x="136" y="240" width="240" height="20" rx="10" ry="10" fill="#e0f2fe" />
  <rect x="340" y="190" width="40" height="12" rx="6" ry="6" fill="#bfdbfe" />
  
  <!-- Air Flow Lines -->
  <path d="M 160 340 q 20 40 -10 80" stroke="#ffffff" stroke-width="20" stroke-linecap="round" fill="none" />
  <path d="M 256 340 q 20 50 0 90" stroke="#ffffff" stroke-width="20" stroke-linecap="round" fill="none" />
  <path d="M 352 340 q -20 40 10 80" stroke="#ffffff" stroke-width="20" stroke-linecap="round" fill="none" />
</svg>
)rawliteral";

// Route handlers
void handlePower() { pressButton(PIN_POWER); server.send(200, "text/plain", "Power Button Pressed"); }
void handleTempUp() { pressButton(PIN_TEMP_UP); server.send(200, "text/plain", "Temp Up Pressed"); }
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

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(""); Serial.print("Connected to "); Serial.println(ssid);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());

  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST Timezone offset: 5.5 * 3600 = 19800

  // Serve static UI assets from PROGMEM
  server.on("/", []() { server.send_P(200, "text/html", html_page); });
  server.on("/UI_Preview.html", []() { server.send_P(200, "text/html", html_page); });
  server.on("/index.html", []() { server.send_P(200, "text/html", html_page); });
  server.on("/manifest.json", []() { server.send_P(200, "application/json", manifest_json); });
  server.on("/sw.js", []() { server.send_P(200, "application/javascript", sw_js); });
  server.on("/icon.svg", []() { server.send_P(200, "image/svg+xml", icon_svg); });

  // Control APIs
  server.on("/power", handlePower);
  server.on("/temp_up", handleTempUp);
  server.on("/temp_down", handleTempDown);
  server.on("/schedule", handleSchedule);
  
  // Dummy handlers for other un-implemented endpoints to avoid 404s in console
  server.on("/timer_on", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/timer_off", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/clock", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/swing", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/energy_save", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/turbo", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/sleep", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/light", [](){ server.send(200, "text/plain", "OK"); });
  server.on("/temp_btn", [](){ server.send(200, "text/plain", "OK"); });

  server.onNotFound(handleNotFound);

  server.begin(); Serial.println("HTTP server started");
}

void loop() { 
  server.handleClient(); 
  
  if (scheduledTime != "" && !scheduleTriggered) {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Check if time is properly synced (year > 1970)
    if (timeinfo->tm_year > 70) {
      char timeStr[6];
      sprintf(timeStr, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
      
      if (String(timeStr) == scheduledTime) {
        Serial.println("Schedule Triggered! Turning ON AC.");
        pressButton(PIN_POWER);
        scheduleTriggered = true; // Prevent running multiple times in same minute
      }
    }
  }
}
