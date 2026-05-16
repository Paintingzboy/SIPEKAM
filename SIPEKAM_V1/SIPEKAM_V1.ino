#include <WiFi.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ================= KONFIGURASI =================
const char* ssid = "Nanadia";
const char* password = "12345678";

IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

#define DHTPIN 4       // GPIO 4 (D4)
#define DHTTYPE DHT22  // Sesuai sensor baru kamu
#define LAMP_PIN 18    // GPIO 18 (D18)
#define FAN_PIN 19     // GPIO 19 (D19)
#define BTN_LAMP 13    // GPIO 13 (D13)
#define BTN_FAN 14     // GPIO 14 (D14)

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WiFiServer server(80);

bool lampState = false;
bool fanState = false;
float suhu = 0, hum = 0;

int scrollX = 128; // Mulai teks dari pinggir kanan layar
unsigned long lastScrollTime = 0;
const int scrollSpeed = 50; // Semakin kecil angka, semakin cepat jalannya

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 1. Inisialisasi Hardware
  Wire.begin(21, 22);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Gagal!");
  }
  
  dht.begin();
  pinMode(LAMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BTN_LAMP, INPUT_PULLUP);
  pinMode(BTN_FAN, INPUT_PULLUP);

  // 2. Splash Screen Awal
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(15, 25);
  display.println("SIPEKAM");
  display.display();

  // 3. Proses Koneksi WiFi
  WiFi.begin(ssid, password);
  unsigned long startAttempt = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) { 
    delay(500); 
    Serial.print("."); 
  }
  
  // 4. Update Tampilan OLED Berdasarkan Status Koneksi
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Sistem Siap!");
  display.drawFastHLine(0, 12, 128, WHITE);
  display.setCursor(0, 25);

  if(WiFi.status() == WL_CONNECTED) {
    // Jika Berhasil Konek
    Serial.println("\nWiFi Connected!");
    server.begin();
    MDNS.begin("esp32");

    display.println("Status: ONLINE");
    display.setCursor(0, 40);
    display.println("IP Address:");
    display.setTextSize(1);
    display.setCursor(0, 52);
    display.println(WiFi.localIP()); // Tampilkan IP di OLED
  } else {
    // Jika Gagal Konek
    Serial.println("\nWiFi Offline Mode");
    display.setTextSize(2);
    display.setCursor(0, 30);
    display.println("OFFLINE");
    display.setTextSize(1);
    display.setCursor(0, 50);
    display.println("Tombol Fisik Aktif");
  }

  display.display();
  delay(3000); // Tampilkan status selama 3 detik sebelum masuk ke menu sensor
}

void loop() {
  // --- BACA SENSOR ---
  for (int i = 0; i < 5; i++) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { suhu = t; hum = h; break; }
    delay(100);
  }

  // --- MANUAL PUSH BUTTON ---
  if (digitalRead(BTN_LAMP) == LOW) {
    delay(200); lampState = !lampState;
    digitalWrite(LAMP_PIN, lampState ? LOW : HIGH); // LOW untuk nyala, HIGH untuk mati
  }
  if (digitalRead(BTN_FAN) == LOW) {
    delay(200); fanState = !fanState;
    digitalWrite(FAN_PIN, fanState ? LOW : HIGH); // LOW untuk nyala, HIGH untuk mati
  }

 // --- OLED ---
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // --- OLED UPDATE DENGAN TEKS BERJALAN ---
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  // Logika Slider / Teks Berjalan untuk Header
  String statusText = "";
  if(WiFi.status() == WL_CONNECTED) {
    statusText = "SIPEKAM | IP: " + WiFi.localIP().toString() + "   "; 
  } else {
    statusText = "SIPEKAM | STATUS: OFFLINE   ";
  }

  // Update posisi scroll setiap X milidetik
  if (millis() - lastScrollTime > scrollSpeed) {
    scrollX--; // Geser ke kiri
    if (scrollX < -150) scrollX = 128; // Reset ke kanan jika sudah lewat layar
    lastScrollTime = millis();
  }

// --- OLED UPDATE (VERSI BERSIH & FIX SLIDER) ---
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setTextWrap(false); // Supaya teks IP tidak turun ke baris bawah

  String displayStatus = ""; 
  if(WiFi.status() == WL_CONNECTED) {
    displayStatus = "SIPEKAM | IP: " + WiFi.localIP().toString() + "   "; 
  } else {
    displayStatus = "SIPEKAM | STATUS: OFFLINE   ";
  }

  // Logika pergerakan teks (Slider)
  if (millis() - lastScrollTime > scrollSpeed) {
    scrollX--; 
    if (scrollX < -220) scrollX = 128; // Batas reset diperlebar agar IP tidak terpotong
    lastScrollTime = millis();
  }

  display.setCursor(scrollX, 0); 
  display.print(displayStatus);
  
  display.setTextWrap(true); 
  display.drawFastHLine(0, 10, 128, WHITE);

  // --- DATA SENSOR (TAMPILAN UTAMA) ---
  display.setCursor(0, 18); 
  display.println("SUHU");
  display.setTextSize(2); 
  display.setCursor(0, 30);
  display.print(suhu, 1); 
  display.setTextSize(1); display.print(" C");

  display.setCursor(70, 18); 
  display.println("LEMBAP");
  display.setTextSize(2);
  display.setCursor(70, 30);
  display.print((int)hum); 
  display.setTextSize(1); display.print(" %");

  // Status Relay di Bagian Bawah
  display.drawFastHLine(0, 50, 128, WHITE);
  display.setCursor(0, 56); 
  display.print("LMP:"); display.print(lampState ? "ON" : "OFF");
  display.setCursor(70, 56); 
  display.print("FAN:"); display.print(fanState ? "ON" : "OFF");

  display.display();

  // --- WEB SERVER ---
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    
    // Kontrol relay via AJAX (Tanpa Refresh)
    if (request.indexOf("/L_ON") != -1) lampState = true;
    if (request.indexOf("/L_OFF") != -1) lampState = false;
    if (request.indexOf("/F_ON") != -1) fanState = true;
    if (request.indexOf("/F_OFF") != -1) fanState = false;
    
    // Ganti bagian ini agar relay sinkron dengan web
    digitalWrite(LAMP_PIN, lampState ? LOW : HIGH);
    digitalWrite(FAN_PIN, fanState ? LOW : HIGH);

    // Endpoint API untuk update data
    if (request.indexOf("/update") != -1) {
      String json = "{\"t\":" + String(suhu,1) + ",\"h\":" + String(hum,1) + ",\"l\":" + String(lampState) + ",\"f\":" + String(fanState) + "}";
      client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json);
      client.stop();
      return;
    }

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<link href='https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600&display=swap' rel='stylesheet'>";
    html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
    html += "<style>";
    html += "body{background:#070b13; color:#fff; font-family:\"Inter\", sans-serif; margin:0; display:flex; justify-content:center; align-items:center; min-height:100vh;}";
    
    // Splash Screen
    html += "#splash{position:fixed; top:0; left:0; width:100%; height:100%; background:#070b13; z-index:999; display:flex; flex-direction:column; align-items:center; justify-content:center;}";
    html += ".splash-text{font-size:28px; letter-spacing:8px; font-weight:300; margin-bottom:10px;}";
    
    // Main Background Box (Kotak di background)
    html += ".main-box{width:360px; background:#0c121e; border-radius:40px; padding:30px; border:1px solid #1a2335; box-shadow: 0 20px 50px rgba(0,0,0,0.5);}";
    html += ".sec-label{font-size:11px; color:#3a4a6e; letter-spacing:2px; font-weight:600; margin:20px 0 12px 0; display:block; text-transform:uppercase;}";
    
    // Chart Design
    html += ".chart-container{background:#151d2e; height:140px; border-radius:20px; padding:15px; border:1px solid #1e293d; margin-bottom:10px;}";
    
    // Grid Align
    html += ".grid{display:grid; grid-template-columns:1fr 1fr; gap:15px; align-items:stretch;}";
    html += ".card{background:#151d2e; border-radius:25px; padding:18px; border:1px solid #1e293d; display:flex; flex-direction:column; justify-content:space-between; height:150px; box-sizing:border-box;}";
    html += ".val{font-size:28px; font-weight:600;} .label{font-size:10px; color:#6b7a99;}";
    
    // Toggle Switch
    html += ".switch-group{display:flex; justify-content:space-between; align-items:center;}";
    html += ".switch{position:relative; width:60px; height:32px;} .switch input{opacity:0; width:0; height:0;}";
    html += ".slider{position:absolute; cursor:pointer; top:0; left:0; right:0; bottom:0; background:#303f5f; transition:.4s; border-radius:30px;}";
    html += ".slider:before{position:absolute; content:\"\"; height:24px; width:24px; left:4px; bottom:4px; background:white; transition:.4s; border-radius:50%;}";
    html += "input:checked + .slider{background:#00d084;} input:checked + .slider:before{transform:translateX(28px);}";
    
    html += ".detail{background:#151d2e; border-radius:20px; padding:15px; font-size:10px; color:#a1acc2; line-height:1.6; border:1px solid #1e293d;}";
    html += "</style></head><body>";
    
    html += "<div id='splash'><div class='splash-text'>SIPEKAM</div><div class='splash-sub'>(Smart Rice Dryer and Quality Monitoring)</div></div>";

    // Start Main Box
    html += "<div class='main-box'>";
    html += "<span class='sec-label' style='margin-top:0;'>Grafik</span>";
    html += "<div class='chart-container'><canvas id='tC'></canvas></div>";
    html += "<div class='chart-container'><canvas id='hC'></canvas></div>";
    
    html += "<div class='grid'>";
    html += "<div><span class='sec-label'>Status</span><div class='card'>";
    html += "<div><div class='label'>Temp</div><div class='val' id='vt'>--</div></div>";
    html += "<div><div class='label'>Hum</div><div class='val' id='vh'>--</div></div></div></div>";
    
    html += "<div><span class='sec-label'>Control</span><div class='card'>";
    html += "<div><div class='label'>Lamps</div><div class='switch-group'><label class='switch'><input type='checkbox' id='sl' onchange='tg(\"L\")'><span class='slider'></span></label></div></div>";
    html += "<div><div class='label'>Fan</div><div class='switch-group'><label class='switch'><input type='checkbox' id='sf' onchange='tg(\"F\")'><span class='slider'></span></label></div></div></div></div>";
    html += "</div>";
    
    html += "<span class='sec-label'>About</span>";
    html += "<div class='detail'><b>SIPEKAM V1.0</b><br>SIPEKAM (Smart Rice Dryer and Quality Monitoring) adalah sebuah inovasi solusi cerdas untuk mengoptimalkan proses pasca panen bagi petani padi</div>";
    html += "</div>"; // End Main Box

    html += "<script>";
    html += "setTimeout(function(){ document.getElementById(\"splash\").style.display=\"none\"; }, 2000);";
    
    html += "function tg(d){ var c=document.getElementById(\"s\"+d.toLowerCase()).checked; fetch(\"/\"+d+(c?\"_ON\":\"_OFF\")); }";

    html += "function mC(id,col,unit){ return new Chart(document.getElementById(id),{type:\"line\",data:{labels:[\" \",\" \",\" \",\" \",\" \",\" \",\" \",\" \",\" \",\" \"],datasets:[{data:[0,0,0,0,0,0,0,0,0,0],borderColor:col,borderWidth:2,tension:0.4,pointRadius:0}]},options:{responsive:true,maintainAspectRatio:false,plugins:{legend:{display:false}},scales:{y:{grid:{color:\"#1e293d\"},ticks:{color:\"#6b7a99\",font:{size:8}}},x:{display:false}}}}); }";
    
    html += "var tC=mC(\"tC\",\"#00d084\"); var hC=mC(\"hC\",\"#3a4a6e\");";

    html += "setInterval(function(){ fetch(\"/update\").then(function(r){return r.json()}).then(function(d){";
    html += "document.getElementById(\"vt\").innerHTML=d.t+\"&deg;\";";
    html += "document.getElementById(\"vh\").innerHTML=d.h+\"%\";";
    html += "document.getElementById(\"sl\").checked=d.l; document.getElementById(\"sf\").checked=d.f;";
    // Update Grafik (Push data baru, shift data lama)
    html += "tC.data.datasets[0].data.push(d.t); tC.data.datasets[0].data.shift(); tC.update(\"none\");";
    html += "hC.data.datasets[0].data.push(d.h); hC.data.datasets[0].data.shift(); hC.update(\"none\");";
    html += "}); }, 2000);";
    html += "</script></body></html>";

    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + html);
    client.stop();
  }
}