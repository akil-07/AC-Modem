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

// PROGMEM Declarations for PWA Web Assets
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
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
    body { font-family: 'Inter', sans-serif; background-color: #f4f5f7; display: flex; justify-content: center; align-items: flex-start; min-height: 100vh; color: #111827; padding-top: 20px; }
    .app-container { background-color: #ffffff; width: 100%; max-width: 400px; min-height: 800px; padding: 40px 24px; border-radius: 40px; box-shadow: 0 30px 60px rgba(0,0,0,0.08); position: relative; }
    header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; }
    header h2 { font-size: 16px; font-weight: 700; }
    .icon-btn { width: 44px; height: 44px; border-radius: 50%; border: 1px solid #f3f4f6; background: #ffffff; display: flex; justify-content: center; align-items: center; cursor: pointer; color: #374151; }
    .title-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 24px; }
    h1 { font-size: 26px; font-weight: 800; }
    .power-btn { width: 48px; height: 48px; border-radius: 50%; border: none; background: #ef4444; color: white; display: flex; justify-content: center; align-items: center; cursor: pointer; box-shadow: 0 4px 15px rgba(239,68,68,0.4); transition: all 0.2s; }
    .power-btn.on { background: #10b981; box-shadow: 0 4px 15px rgba(16,185,129,0.4); }
    .power-btn:active { transform: scale(0.92); }
    .power-btn svg { width: 22px; height: 22px; }
    .modes { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; margin-bottom: 40px; }
    .mode-btn { display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 16px 0; border-radius: 20px; border: none; background-color: #ffffff; box-shadow: 0 4px 15px rgba(0,0,0,0.04), inset 0 0 0 1px #f3f4f6; cursor: pointer; color: #9ca3af; transition: all 0.2s; }
    .mode-btn svg { width: 24px; height: 24px; margin-bottom: 8px; }
    .mode-btn span { font-size: 13px; font-weight: 600; }
    .mode-btn.active { background-color: #3b82f6; color: white; box-shadow: 0 8px 20px rgba(59,130,246,0.3); }
    .dial-container { position: relative; width: 260px; height: 260px; margin: 0 auto 50px auto; display: flex; justify-content: center; align-items: center; }
    .progress-ring { position: absolute; top: 0; left: 0; }
    .progress-ring__circle { transition: stroke-dashoffset 0.3s ease-out; }
    .progress-ring__circle-bg { transition: stroke-dashoffset 0.3s ease-out; }
    .dial-center { width: 170px; height: 170px; background: white; border-radius: 50%; display: flex; justify-content: center; align-items: center; box-shadow: 0 10px 40px rgba(0,0,0,0.08); z-index: 2; }
    .dial-center h2 { font-size: 48px; font-weight: 800; letter-spacing: -2px; position: relative; }
    .dial-center h2 span { font-size: 24px; position: absolute; top: 5px; right: -24px; }
    .dial-controls { position: absolute; width: 280px; display: flex; justify-content: space-between; bottom: 0px; left: -10px; z-index: 10; }
    .temp-btn { width: 48px; height: 48px; border-radius: 50%; background: white; border: 1px solid #f3f4f6; color: #374151; font-size: 24px; font-weight: 300; display: flex; justify-content: center; align-items: center; cursor: pointer; }
    .temp-btn:active { transform: scale(0.95); }
    .status-badge { display: flex; align-items: center; gap: 6px; font-size: 12px; font-weight: 600; color: #6b7280; }
    .status-dot { width: 8px; height: 8px; border-radius: 50%; background: #d1d5db; transition: background 0.4s; }
    .status-dot.connected { background: #10b981; box-shadow: 0 0 0 3px rgba(16,185,129,0.2); }
    .status-dot.disconnected { background: #ef4444; box-shadow: 0 0 0 3px rgba(239,68,68,0.2); }
    .controls-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
    .controls-header h2 { font-size: 20px; font-weight: 800; }
    .controls-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 12px; margin-bottom: 20px; }
    .ctrl-btn { background-color: #ffffff; border: 1px solid #e5e7eb; border-radius: 16px; padding: 14px 0; font-size: 12px; font-weight: 700; color: #4b5563; cursor: pointer; transition: all 0.2s; text-align: center; }
    .ctrl-btn:active { transform: scale(0.96); }
    .schedule-card { background: #f9fafb; border: 1px solid #e5e7eb; border-radius: 20px; padding: 16px; margin-bottom: 30px; display: flex; justify-content: space-between; align-items: center; }
    .schedule-info { display: flex; flex-direction: column; }
    .schedule-info label { font-size: 14px; font-weight: 700; color: #374151; margin-bottom: 4px; }
    .schedule-info input[type="time"] { font-family: 'Inter', sans-serif; font-size: 18px; font-weight: 700; color: #3b82f6; border: none; background: transparent; outline: none; cursor: pointer; }
    .schedule-btn { background: #3b82f6; color: white; border: none; padding: 12px 20px; border-radius: 14px; font-size: 13px; font-weight: 600; cursor: pointer; transition: all 0.2s; }
    #toast { visibility: hidden; min-width: 200px; background-color: #111827; color: #fff; text-align: center; border-radius: 50px; padding: 12px 24px; position: fixed; z-index: 100; left: 50%; transform: translateX(-50%); bottom: 40px; font-size: 14px; opacity: 0; transition: opacity 0.3s, bottom 0.3s; }
    #toast.show { visibility: visible; opacity: 1; bottom: 60px; }
  </style>
</head>
<body>
<div class="app-container">
  <header>
    <button class="icon-btn"><svg width="20" height="20" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M15 19l-7-7 7-7"></path></svg></button>
    <h2>Living Room</h2>
    <div class="status-badge"><div id="status-dot" class="status-dot"></div><span id="status-text">--</span></div>
  </header>
  <div class="title-row">
    <h1>Air Condition</h1>
    <button id="power-btn" class="power-btn on" onclick="togglePower()">
      <svg fill="none" stroke="currentColor" stroke-width="2.5" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
    </button>
  </div>
  <div class="modes">
    <button class="mode-btn active" onclick="setMode('heat')"><svg fill="currentColor" viewBox="0 0 24 24"><path d="M12 2v2m0 16v2M4.93 4.93l1.41 1.41m11.32 11.32l1.41 1.41M2 12h2m16 0h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41M12 7a5 5 0 100 10 5 5 0 000-10z" stroke="currentColor" stroke-width="2" fill="none" stroke-linecap="round" stroke-linejoin="round"/></svg><span>Heat</span></button>
    <button class="mode-btn" onclick="setMode('cool')"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2v20M17 5l-5 5-5-5M17 19l-5-5-5 5M2 12h20M5 7l5 5-5 5M19 7l-5 5 5 5"/></svg><span>Cool</span></button>
    <button class="mode-btn" onclick="setMode('dry')"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M12 2.69l5.66 5.66a8 8 0 11-11.31 0z"/></svg><span>Dry</span></button>
    <button class="mode-btn" onclick="setMode('fan')"><svg fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" stroke-linecap="round" stroke-linejoin="round"><path d="M12 12a2 2 0 100-4 2 2 0 000 4zm0 0a2 2 0 110 4 2 2 0 010-4zm0 0a2 2 0 10-4 0 2 2 0 004 0zm0 0a2 2 0 114 0 2 2 0 01-4 0zM12 2v6M12 16v6M2 12h6M16 12h6"/></svg><span>Fan</span></button>
  </div>
  <div class="dial-container">
    <svg class="progress-ring" width="260" height="260">
      <g stroke="#9ca3af" stroke-width="1.5" stroke-linecap="round" opacity="0.6">
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(-135 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(-90 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(-45 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(0 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(45 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(90 130 130)" />
        <line x1="130" y1="10" x2="130" y2="20" transform="rotate(135 130 130)" />
      </g>
      <circle class="progress-ring__circle-bg" stroke="#e0f2fe" stroke-width="14" fill="transparent" r="95" cx="130" cy="130" stroke-linecap="round" transform="rotate(135 130 130)"/>
      <circle class="progress-ring__circle" stroke="#3b82f6" stroke-width="14" fill="transparent" r="95" cx="130" cy="130" stroke-linecap="round" transform="rotate(135 130 130)"/>
      <circle id="dial-thumb" cx="130" cy="35" r="7" fill="#3b82f6" stroke="#fff" stroke-width="3" />
    </svg>
    <div class="dial-center"><h2 id="temp-display">24<span>&#176;C</span></h2></div>
    <div class="dial-controls">
      <button class="temp-btn" onclick="changeTemp(-1)">-</button>
      <button class="temp-btn" onclick="changeTemp(1)">+</button>
    </div>
  </div>
  <div class="controls-header"><h2>Smart Schedule</h2></div>
  <div class="schedule-card">
    <div class="schedule-info">
      <label>Auto Turn-ON Time</label>
      <input type="time" id="auto-on-time" value="08:00">
    </div>
    <button class="schedule-btn" onclick="setSchedule()">Set Timer</button>
  </div>
  <div class="controls-header"><h2>Additional Controls</h2></div>
  <div class="controls-grid">
    <button class="ctrl-btn" onclick="sc('/timer_on')">Timer On</button>
    <button class="ctrl-btn" onclick="sc('/timer_off')">Timer Off</button>
    <button class="ctrl-btn" onclick="sc('/clock')">Clock</button>
    <button class="ctrl-btn" onclick="sc('/swing')">Swing</button>
    <button class="ctrl-btn" onclick="sc('/energy_save')">Energy Save</button>
    <button class="ctrl-btn" onclick="sc('/turbo')">Turbo</button>
    <button class="ctrl-btn" onclick="sc('/sleep')">Sleep</button>
    <button class="ctrl-btn" onclick="sc('/light')">Light</button>
    <button class="ctrl-btn" onclick="sc('/temp_btn')">Temp</button>
  </div>
</div>
<div id="toast"></div>
<script>
  var temp=24,minT=16,maxT=30;
  var ring=document.querySelector('.progress-ring__circle');
  var ringBg=document.querySelector('.progress-ring__circle-bg');
  var R=ring.r.baseVal.value,C=R*2*Math.PI,A=C*0.75;
  ring.style.strokeDasharray=ringBg.style.strokeDasharray=C+' '+C;
  ringBg.style.strokeDashoffset=C-A;
  function dial(){var p=(temp-minT)/(maxT-minT);ring.style.strokeDashoffset=C-(p*A);var rad=(135+p*270)*Math.PI/180;document.getElementById('dial-thumb').setAttribute('cx',130+R*Math.cos(rad));document.getElementById('dial-thumb').setAttribute('cy',130+R*Math.sin(rad));}
  function sc(path){fetch(path).then(function(){toast('&#10003; '+path.replace('/','').toUpperCase())}).catch(function(){toast('&#9888; No response');});}
  function changeTemp(d){var n=temp+d;if(n>=minT&&n<=maxT){temp=n;document.getElementById('temp-display').innerHTML=temp+'<span>&#176;C</span>';dial();sc(d>0?'/tempup':'/tempdown');}}
  var pwr=true;
  function togglePower(){pwr=!pwr;var b=document.getElementById('power-btn');pwr?b.classList.add('on'):b.classList.remove('on');sc('/power');}
  function setMode(m){document.querySelectorAll('.mode-btn').forEach(function(b){b.classList.remove('active');});event.currentTarget.classList.add('active');toast('Mode: '+m.toUpperCase());}
  function setSchedule(){var t=document.getElementById('auto-on-time').value;if(!t)return;sc('/schedule?time='+t);}
  function toast(msg){var el=document.getElementById('toast');el.innerHTML=msg;el.className='show';setTimeout(function(){el.className='';},2500);}
  var dot=document.getElementById('status-dot'),stxt=document.getElementById('status-text');
  function ping(){fetch('/power').then(function(){dot.className='status-dot connected';stxt.textContent='Connected';}).catch(function(){dot.className='status-dot disconnected';stxt.textContent='Offline';});}
  dial();ping();setInterval(ping,5000);
</script>
</body>
</html>
)rawliteral";
const char manifest_json[] PROGMEM = R"rawliteral({
  "name": "Smart AC Controller",
  "short_name": "AC Control",
  "start_url": "./index.html",
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
const char sw_js[] PROGMEM = R"rawliteral(const CACHE_NAME = 'ac-controller-v2';
const ASSETS_TO_CACHE = [
  './index.html',
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
