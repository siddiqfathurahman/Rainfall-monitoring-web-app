#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Inisialisasi LCD (alamat I2C dan ukuran LCD 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin sensor hujan, buzzer, dan button
const int rainSensorPin = D6;
const int buzzerPin = D5;
const int buttonPin = D7;

// Variabel WiFi dan server web
const char* ssid = "POCO F4";
const char* password = "sayaadalahkamu";
ESP8266WebServer server(80);

int rainOutput = 0;
bool ipDisplayed = false;

// String HTML untuk halaman web
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Monitoring Sensor Hujan</title>
    <link href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css" rel="stylesheet">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; font-family: Arial, sans-serif; }
        body { background: #f0f2f5; padding: 20px; min-height: 100vh; transition: background-color 0.5s; position: relative; overflow: hidden; }
        .dark-mode { background: #1a1a1a; color: #fff; }
        .container { max-width: 800px; margin: 0 auto; position: relative; }
        .header { background: #fff; padding: 20px; border-radius: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); margin-bottom: 20px; }
        .dark-mode .header { background: #2d2d2d; }
        .title { color: #1a73e8; margin-bottom: 10px; font-size: 24px; text-align: center; }
        .status-card { background: #fff; padding: 30px; border-radius: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); text-align: center; }
        .dark-mode .status-card { background: #2d2d2d; }
        .weather-icon { font-size: 80px; margin: 20px 0; }
        .status-text { font-size: 2.5em; font-weight: bold; margin: 20px 0; text-transform: uppercase; }
        .raindrop { position: absolute; width: 2px; height: 10px; background: rgba(255, 255, 255, 0.8); animation: fall linear infinite; }
        @keyframes fall {
            0% { top: -10px; }
            100% { top: 100%; }
        }
        .lightning { position: absolute; width: 4px; height: 60px; background: yellow; display: none; }
        @keyframes flash {
            0%, 100% { opacity: 0; }
            50% { opacity: 1; }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1 class="title">Dashboard Monitoring Sensor Hujan</h1>
            <p class="last-update">Update terakhir: <span id="lastUpdate">-</span></p>
        </div>
        <div class="status-card">
            <div id="weatherIcon" class="weather-icon">
                <i class="fas fa-sun"></i>
            </div>
            <div class="status-text" id="statusText">Mengecek...</div>
        </div>
    </div>
    <script>
        function createRaindrops() {
            const container = document.body;
            for (let i = 0; i < 50; i++) {
                const drop = document.createElement('div');
                drop.className = 'raindrop';
                drop.style.left = Math.random() * 100 + 'vw';
                drop.style.animationDuration = Math.random() * 1 + 0.5 + 's'; // Random fall speed
                container.appendChild(drop);
            }
        }

        function createLightning() {
            const container = document.body;
            const lightning = document.createElement('div');
            lightning.className = 'lightning';
            lightning.style.left = Math.random() * 100 + 'vw';
            lightning.style.top = Math.random() * 50 + 'vh';
            lightning.style.display = 'block';
            container.appendChild(lightning);
            lightning.style.animation = 'flash 0.2s forwards';

            setTimeout(() => {
                lightning.remove();
            }, 200); // Remove lightning after it flashes
        }

        function updateStatus() {
            fetch('/status')
                .then(response => response.text())
                .then(data => {
                    const weatherIcon = document.getElementById('weatherIcon');
                    const statusText = document.getElementById('statusText');
                    const body = document.body;

                    if (data === '1') {
                        weatherIcon.innerHTML = '<i class="fas fa-cloud-rain"></i>';
                        statusText.textContent = 'HUJAN';
                        body.classList.add('dark-mode'); 
                        createRaindrops();
                        createLightning();
                        setInterval(createLightning, 2000); // Flash lightning every 2 seconds
                    } else {
                        weatherIcon.innerHTML = '<i class="fas fa-sun"></i>';
                        statusText.textContent = 'CERAH';
                        body.classList.remove('dark-mode');
                        const drops = document.querySelectorAll('.raindrop');
                        drops.forEach(drop => drop.remove()); // Remove raindrops
                    }
                    const now = new Date();
                    document.getElementById('lastUpdate').textContent = now.toLocaleTimeString('id-ID');
                })
                .catch(error => {
                    console.error('Error:', error);
                    document.getElementById('statusText').textContent = 'Error koneksi';
                });
        }

        // Update status setiap 1 detik
        setInterval(updateStatus, 1000);
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

// Function prototypes
void connectToWiFi();
void handleRoot();
void handleStatus();
void displayIPAddress();

void setup() {
    lcd.init();
    lcd.backlight();
    Serial.begin(115200);
    pinMode(rainSensorPin, INPUT);
    pinMode(buzzerPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);
    
    lcd.setCursor(0, 0);
    lcd.print("Status Cuaca:");
    lcd.setCursor(0, 1);
    lcd.print("Cerah");
    
    connectToWiFi();
    
    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.begin();
}

void loop() {
    int rainValue = digitalRead(rainSensorPin);
    if (rainValue == LOW) {
        if (rainOutput == 0) {
            rainOutput = 1;
            lcd.setCursor(0, 1);
            lcd.print("Hujan ");
            digitalWrite(buzzerPin, HIGH);
        }
    } else {
        if (rainOutput == 1) {
            rainOutput = 0;
            lcd.setCursor(0, 1);
            lcd.print("Cerah ");
            digitalWrite(buzzerPin, LOW);
        }
    }
    
    Serial.println(rainOutput);
    
    if (digitalRead(buttonPin) == LOW) {
        displayIPAddress();
    }
    
    server.handleClient();
}

void handleRoot() {
    server.send(200, "text/html", html);
}

void handleStatus() {
    server.send(200, "text/plain", String(rainOutput));
}

void displayIPAddress() {
    if (!ipDisplayed) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("IP Address:");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP());
        ipDisplayed = true;
        delay(5000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Status Cuaca:");
        ipDisplayed = false;
    }
}

void connectToWiFi() {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi Terhubung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}
