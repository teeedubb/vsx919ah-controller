// ESP32 IR sender using IRremoteESP8266, OTA, and enhanced Web UI with raw support
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "commands.h"
#include "credentials.h"

const uint8_t kIrGpio = 25;
WebServer server(80);
IRsend irsend(kIrGpio);

// Predefined raw IR commands
struct RawCommand {
  const char* name;
  uint16_t* data;
  uint16_t length;
  const char* category;
};

// Array of all raw commands with categories
RawCommand rawCommands[] = {
// Inputs
  {"HDMI 1", rawInputHDMI1, sizeof(rawInputHDMI1) / sizeof(rawInputHDMI1[0]), "input"},
  {"HDMI 2", rawInputHDMI2, sizeof(rawInputHDMI2) / sizeof(rawInputHDMI2[0]), "input"},
  {"HDMI BD", rawInputBD, sizeof(rawInputBD) / sizeof(rawInputBD[0]), "input"},
  {"DVD (PC)", rawInputDVD, sizeof(rawInputDVD) / sizeof(rawInputDVD[0]), "input"},
  {"DVR", rawInputDVR, sizeof(rawInputDVR) / sizeof(rawInputDVR[0]), "input"},
  {"TV/SAT", rawInputTVSAT, sizeof(rawInputTVSAT) / sizeof(rawInputTVSAT[0]), "input"},
  {"AUX", rawInputAUX, sizeof(rawInputAUX) / sizeof(rawInputAUX[0]), "input"},
  {"CD", rawInputCD, sizeof(rawInputCD) / sizeof(rawInputCD[0]), "input"},
  {"CDR", rawInputCDR, sizeof(rawInputCDR) / sizeof(rawInputCDR[0]), "input"},
  {"Multi CH", rawInputMultiCH, sizeof(rawInputMultiCH) / sizeof(rawInputMultiCH[0]), "input"},
//  {"Tuner", rawInputTuner, sizeof(rawInputTuner) / sizeof(rawInputTuner[0]), "input"},
//  {"Video", rawInputVideo, sizeof(rawInputVideo) / sizeof(rawInputVideo[0]), "input"},
  {"Next", rawInputNext, sizeof(rawInputNext) / sizeof(rawInputNext[0]), "input"},
  {"Prev", rawInputPrev, sizeof(rawInputPrev) / sizeof(rawInputPrev[0]), "input"},
//Control
  {"Vol Up", rawVolumeUp, sizeof(rawVolumeUp) / sizeof(rawVolumeUp[0]), "control"},
  {"Vol Down", rawVolumeDown, sizeof(rawVolumeDown) / sizeof(rawVolumeDown[0]), "control"},
  {"Mute", rawMute, sizeof(rawMute) / sizeof(rawMute[0]), "control"},
//  {"Speakers", rawSpeaker, sizeof(rawSpeaker) / sizeof(rawSpeaker[0]), "control"},
  {"Power", rawPower, sizeof(rawPower) / sizeof(rawPower[0]), "control"},
  {"Status", rawStatus, sizeof(rawStatus) / sizeof(rawStatus[0]), "control"},
  {"Dimmer", rawDimmer, sizeof(rawDimmer) / sizeof(rawDimmer[0]), "control"},
//Settings
  {"Direct", rawDirect, sizeof(rawDirect) / sizeof(rawDirect[0]), "settings"},
  {"Auto Surround", rawAutoSurround, sizeof(rawAutoSurround) / sizeof(rawAutoSurround[0]), "settings"},
  {"Standard Surround", rawStandardSurround, sizeof(rawStandardSurround) / sizeof(rawStandardSurround[0]), "settings"},
  {"Advanced Surround", rawAdvancedSurround, sizeof(rawAdvancedSurround) / sizeof(rawAdvancedSurround[0]), "settings"},
  {"Stereo", rawStereo, sizeof(rawStereo) / sizeof(rawStereo[0]), "settings"},
  {"Auto Level Control", rawAutoLevelControl, sizeof(rawAutoLevelControl) / sizeof(rawAutoLevelControl[0]), "settings"},
  {"Pure Direct", rawPureDirect, sizeof(rawPureDirect) / sizeof(rawPureDirect[0]), "settings"},
  {"Mode Toggle", rawModeToggle, sizeof(rawModeToggle) / sizeof(rawModeToggle[0]), "settings"}
};

void handleRoot() {
String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>VSX-919AH</title>
    <link rel="icon" type="image/svg+xml" sizes="any" href="data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTI4IiBoZWlnaHQ9IjEyOCIgdmlld0JveD0iMCAwIDIxMCAyOTciIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PGcgaWQ9ImxheWVyMSI+PHBhdGggZD0ibSAxNy45OTE2NjgsMTU4LjQzNTA4IGggMjguMTU2MTQ0IGwgMTcuNzEwNDAzLC00OC41NDY2MSBjIDAsMCA0LjExMTAzOCwtMTYuNTk1MTQxIDMxLjg5MzEzOSwtMTYuNTk1MTQxIDI1LjU0NDcwNiwwIDIzLjUwNjM0Niw5LjMyMzYxMSAyMy41MDYzNDYsMTAuODE2MzUxIDAsMS40OTYxNyAtMS42OTE3Nyw5LjY5NDIyIC0yMS42Mzk1NjQsOS42OTQyMiAtMTkuOTQ0MzY1LDAgLTI4LjkwNDIzLC0zLjM1OTUyIC0yOC45MDQyMywtMy4zNTk1MiBsIDQuMjkyOTExLDIwLjUxNzQ0IGMgMCwwIDE5LjI3MTc3NSwyLjYxNDg3IDI3Ljk3MDg0MywyLjYxNDg3IDcuNjUyNDMsMCA0Ni4yNTQzMSwtNC4zMDMyMSA0Ni4yNTQzMSwtMzEuMzQwNjUgMCwtMTcuMTU3OTIzIC0yMy4xMzU3MywtMjguMTUyNzE3IC01MC45MTc4MzYsLTI4LjE1MjcxNyAtMzIuNDQ5MDU1LDAgLTUwLjkzODQyOCwxMi40MjU3NjUgLTU5LjEwNTU5OCwzMi44MjY1MjcgeiBNIDE1My41NzM1NCwxMTAuMzU4NTkgMTM1LjY2NDEsMTU4LjQzNTA4IFoiIGZpbGw9IiM5MDBkMzUiIHN0eWxlPSJzdHJva2Utd2lkdGg6Ni44NjMxNiIvPjwvZz48L3N2Zz4="/>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #8B0000 0%, #5F021F 50%, #2C1810 100%);
            min-height: 100vh;
            padding: 1rem;
            color: #333;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: rgba(255, 255, 255, 0.95);
            border-radius: 20px;
            padding: 2rem;
            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
            backdrop-filter: blur(10px);
        }
        
        .header {
            text-align: center;
            margin-bottom: 2rem;
        }
        
        .pioneer-logo {
            margin-bottom: -1.5rem;
            display: flex;
            justify-content: center;
        }
        
        .pioneer-logo svg {
            filter: drop-shadow(0 2px 4px rgba(139, 0, 0, 0.3));
        }
        
        .header p {
            color: #666;
            font-size: 1.1rem;
        }
        
        .section {
            margin-bottom: 2rem;
        }
        
        .section-title {
            font-size: 1.3rem;
            font-weight: 600;
            margin-bottom: 1rem;
            color: #5F021F;
            padding-bottom: 0.5rem;
            border-bottom: 2px solid #8B0000;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }
        
        .section-icon {
            font-size: 1.2rem;
        }
        
        .button-grid {
            display: grid;
            gap: 0.75rem;
            margin-bottom: 1rem;
            width: 100%;
        }
        
        /* Dynamic grid columns based on number of buttons */
        .button-grid[data-count="1"] { grid-template-columns: 1fr; }
        .button-grid[data-count="2"] { grid-template-columns: 1fr 1fr; }
        .button-grid[data-count="3"] { grid-template-columns: 1fr 1fr 1fr; }
        .button-grid[data-count="4"] { grid-template-columns: 1fr 1fr; }
        .button-grid[data-count="5"] { grid-template-columns: 1fr 1fr 1fr; }
        .button-grid[data-count="6"] { grid-template-columns: 1fr 1fr 1fr; }
        .button-grid[data-count="7"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        .button-grid[data-count="8"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        .button-grid[data-count="9"] { grid-template-columns: 1fr 1fr 1fr; }
        .button-grid[data-count="10"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        .button-grid[data-count="11"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        .button-grid[data-count="12"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        .button-grid[data-count="13"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        .button-grid[data-count="14"] { grid-template-columns: 1fr 1fr 1fr 1fr; }
        
        /* Default fallback for larger counts */
        .button-grid:not([data-count]) { grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); }
        
        .btn {
            background: linear-gradient(135deg, #8B0000, #5F021F);
            color: white;
            border: none;
            border-radius: 12px;
            padding: 0.75rem 1rem;
            font-size: 0.9rem;
            font-weight: 500;
            text-decoration: none;
            text-align: center;
            transition: all 0.3s ease;
            cursor: pointer;
            position: relative;
            overflow: hidden;
            display: block;
            box-shadow: 0 3px 10px rgba(95, 2, 31, 0.3);
            width: 100%;
        }
        
        .btn::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
            transition: left 0.5s;
        }
        
        .btn:hover::before {
            left: 100%;
        }
        
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 20px rgba(139, 0, 0, 0.4);
            background: linear-gradient(135deg, #A50000, #6F2C4F);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .custom-form {
            background: #f8f9fa;
            border-radius: 15px;
            padding: 1.5rem;
            border: 1px solid #e9ecef;
        }
        
        .form-group {
            margin-bottom: 1rem;
        }
        
        .form-label {
            display: block;
            margin-bottom: 0.5rem;
            font-weight: 500;
            color: #555;
        }
        
        .form-control {
            width: 100%;
            padding: 0.75rem;
            border: 2px solid #e9ecef;
            border-radius: 8px;
            font-size: 1rem;
            transition: border-color 0.3s ease;
            background: white;
        }
        
        .form-control:focus {
            outline: none;
            border-color: #8B0000;
            box-shadow: 0 0 0 3px rgba(139, 0, 0, 0.1);
        }
        
        textarea.form-control {
            resize: vertical;
            min-height: 100px;
            font-family: monospace;
        }
        
        .btn-submit {
            background: linear-gradient(135deg, #8B0000, #5F021F);
            padding: 0.75rem 2rem;
            font-size: 1rem;
            border-radius: 8px;
            border: none;
            color: white;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 3px 10px rgba(95, 2, 31, 0.3);
        }
        
        .btn-submit:hover {
            transform: translateY(-1px);
            box-shadow: 0 6px 20px rgba(139, 0, 0, 0.4);
            background: linear-gradient(135deg, #A50000, #6F2C4F);
        }
        
        .status {
            text-align: center;
            padding: 1rem;
            margin-top: 1rem;
            border-radius: 8px;
            background: linear-gradient(135deg, rgba(139, 0, 0, 0.1), rgba(95, 2, 31, 0.1));
            color: #5F021F;
            border: 1px solid rgba(139, 0, 0, 0.2);
        }
        
        @media (max-width: 768px) {
            .container {
                padding: 1rem;
                margin: 0.5rem;
            }
            
            .pioneer-logo svg {
                width: 280px;
                height: 80px;
            }
            
            .button-grid {
                gap: 0.5rem;
            }
            
            .btn {
                padding: 0.6rem 0.8rem;
                font-size: 0.8rem;
            }
        }
        
        @media (max-width: 480px) {
            .pioneer-logo svg {
                width: 240px;
                height: 68px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="pioneer-logo">
                <svg xmlns="http://www.w3.org/2000/svg" width="350" height="100" viewBox="8 82 178 48">
                    <defs>
                        <linearGradient id="pioneerTextGradient" x1="0%" y1="0%" x2="100%" y2="100%">
                            <stop offset="0%" style="stop-color:#8B0000;stop-opacity:1" />
                            <stop offset="100%" style="stop-color:#5F021F;stop-opacity:1" />
                        </linearGradient>
                    </defs>
                    <g fill-rule="evenodd" clip-rule="evenodd">
                        <path d="M10.085 106.322h8.205l5.161-14.147s1.198-4.836 9.294-4.836c7.444 0 6.85 2.717 6.85 3.152 0 .436-.493 2.825-6.306 2.825-5.812 0-8.423-.979-8.423-.979l1.251 5.979s5.616.762 8.151.762c2.23 0 13.479-1.254 13.479-9.133 0-5-6.742-8.204-14.838-8.204-9.456 0-14.844 3.621-17.224 9.566l-5.6 15.015zM49.595 92.312l-5.219 14.01h7.664l5.215-14.01h-7.66zM84.592 98.967c0 5.629-9.408 8.1-14.838 8.1-2.664 0-12.932-1.469-12.932-7.5 0-6.032 10.224-7.988 14.347-7.988 4.615 0 13.423 1.76 13.423 7.388zm-13.914-2.988c-1.848 0-5.704 1.033-5.704 3.423 0 2.393 3.421 3.209 5.433 3.209s6.034-.982 6.034-3.371-3.643-3.261-5.763-3.261zM84.101 106.322h7.5l3.043-8.115c.208-.674 1.251-2.336 4.512-2.336 3.783 0 3.32 2.301 3.096 2.881l-2.771 7.57h7.393l2.879-7.787s2.391-6.848-9.02-6.848c-11.684 0-13.368 5.977-13.368 5.977l-3.264 8.658zM126.326 95.383c4.4 0 4.566 1.357 4.566 1.357s.816.924-4.131.924-6.9-1.195-6.9-1.195 2.553-1.086 6.465-1.086zm-.76-3.696c-5.221 0-14.129 2.175-14.129 8.313 0 5.219 9.721 7.119 12.658 7.119 7.719 0 11.145-1.193 11.145-1.193l1.576-4.348s-4.455 1.629-10.814 1.629c-9.676 0-8.969-4.076-8.969-4.076s2.666 2.176 9.133 2.176s12.119-1.359 12.119-3.969c0-2.607-5.512-5.651-12.719-5.651zM165.943 106.322h7.447l2.715-7.57s.545-2.121 2.5-2.121h5.816l1.412-4.319h-10.488s-4.238.247-6.086 4.919c-1.847 4.671-3.316 9.091-3.316 9.091zM166.461 97.439c0 2.607-5.65 3.967-12.119 3.967-6.467 0-9.131-2.176-9.131-2.176s-.707 4.076 8.967 4.076c6.359 0 10.816-1.629 10.816-1.629l-1.578 4.348s-3.426 1.193-11.145 1.193c-2.938 0-12.656-1.9-12.656-7.119 0-6.137 8.906-8.312 14.127-8.312 7.207.001 12.719 3.044 12.719 5.652zm-18.422-.871s1.953 1.195 6.9 1.195s4.131-.924 4.131-.924-.168-1.357-4.568-1.357c-3.912.001-6.463 1.086-6.463 1.086z" fill="url(#pioneerTextGradient)"/>
                    </g>
                </svg>
            </div>
            <p>VSX-919AH controller</p>
        </div>
        
        <div class="section">
            <div class="section-title">
                <span class="section-icon">📺</span>
                Input Sources
            </div>
            <div class="button-grid" id="input-grid">
)rawliteral";

  // Count and add input buttons
  int inputCount = 0;
  for (auto& cmd : rawCommands) {
    if (String(cmd.category) == "input") {
      inputCount++;
    }
  }
  html += "<script>document.getElementById('input-grid').setAttribute('data-count', '" + String(inputCount) + "');</script>";
  
  for (auto& cmd : rawCommands) {
    if (String(cmd.category) == "input") {
      html += "<a class='btn' href='/sendraw?name=" + String(cmd.name) + "'>" + String(cmd.name) + "</a>";
    }
  }

  html += R"rawliteral(
            </div>
        </div>
        
        <div class="section">
            <div class="section-title">
                <span class="section-icon">🎛️</span>
                Control
            </div>
            <div class="button-grid" id="control-grid">
)rawliteral";

  // Count and add control buttons
  int controlCount = 0;
  for (auto& cmd : rawCommands) {
    if (String(cmd.category) == "control") {
      controlCount++;
    }
  }
  html += "<script>document.getElementById('control-grid').setAttribute('data-count', '" + String(controlCount) + "');</script>";
  
  for (auto& cmd : rawCommands) {
    if (String(cmd.category) == "control") {
      html += "<a class='btn' href='/sendraw?name=" + String(cmd.name) + "'>" + String(cmd.name) + "</a>";
    }
  }

  html += R"rawliteral(
            </div>
        </div>
        
        <div class="section">
            <div class="section-title">
                <span class="section-icon">⚙️</span>
                Settings
            </div>
            <div class="button-grid" id="settings-grid">
)rawliteral";

  // Count and add settings buttons
  int settingsCount = 0;
  for (auto& cmd : rawCommands) {
    if (String(cmd.category) == "settings") {
      settingsCount++;
    }
  }
  html += "<script>document.getElementById('settings-grid').setAttribute('data-count', '" + String(settingsCount) + "');</script>";
  
  for (auto& cmd : rawCommands) {
    if (String(cmd.category) == "settings") {
      html += "<a class='btn' href='/sendraw?name=" + String(cmd.name) + "'>" + String(cmd.name) + "</a>";
    }
  }

  html += R"rawliteral(
            </div>
        </div>
        
        <div class="section">
            <div class="section-title">
                <span class="section-icon">🔧</span>
                Custom Command
            </div>
            <div class="custom-form">
                <form action='/sendraw' method='GET' id='customForm'>
                    <div class="form-group">
                        <label class="form-label">Raw IR Data (comma separated pulse timings):</label>
                        <textarea name='raw' class="form-control" placeholder="Example: 9000,4500,560,560,560,1690,560,560..."></textarea>
                    </div>
                    <button type='submit' class="btn-submit" id='customSubmit'>Send Custom Command</button>
                </form>
            </div>
        </div>
        
        <div class="status">
            Ready to send IR commands • IP: )rawliteral" + WiFi.localIP().toString() + R"rawliteral(
        </div>
    </div>
    
    <script>
        // Add click feedback
        document.querySelectorAll('.btn, .btn-submit').forEach(btn => {
            btn.addEventListener('click', function() {
                this.style.transform = 'translateY(0)';
                setTimeout(() => {
                    this.style.transform = '';
                }, 100);
            });
        });
        
        // Auto-focus textarea on mobile
        if (window.innerWidth <= 768) {
            document.querySelector('textarea').addEventListener('focus', function() {
                setTimeout(() => {
                    this.scrollIntoView({behavior: 'smooth', block: 'center'});
                }, 300);
            });
        }
    </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

void handleSendRaw() {
  String name = server.arg("name");
  String rawStr = server.arg("raw");
  bool commandSent = false;

  if (rawStr.length() > 0) {
    // Handle custom raw command
    std::vector<uint16_t> pulses;
    int lastIndex = 0;
    while (lastIndex < rawStr.length()) {
      int commaIndex = rawStr.indexOf(',', lastIndex);
      String token = (commaIndex == -1) ? rawStr.substring(lastIndex) : rawStr.substring(lastIndex, commaIndex);
      token.trim(); // Remove whitespace
      if (token.length() > 0) {
        pulses.push_back(token.toInt());
      }
      if (commaIndex == -1) break;
      lastIndex = commaIndex + 1;
    }
    if (!pulses.empty()) {
      irsend.sendRaw(pulses.data(), pulses.size(), 38);
      commandSent = true;
      Serial.println("Sent custom raw command with " + String(pulses.size()) + " pulses");
    }
  } else if (name.length() > 0) {
    // Handle predefined raw command
    for (auto& cmd : rawCommands) {
      if (name == cmd.name) {
        irsend.sendRaw(cmd.data, cmd.length, 38);
        commandSent = true;
        Serial.println("Sent command: " + name);
        break;
      }
    }
  }
  
  if (!commandSent) {
    Serial.println("No valid command found");
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);  // This is the Arduino-ESP32 equivalent
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  irsend.begin();
  ArduinoOTA.setPassword(otaPassword);
  ArduinoOTA.begin();

  server.on("/", handleRoot);
  server.on("/sendraw", handleSendRaw);
  server.begin();
  
  Serial.println("Server started");
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
}