#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <TinyGPSPlus.h>

// ================= AP MODE =================
const char* ap_ssid = "ESP32_ROVER";
const char* ap_password = "12345678";

// ESP32-CAM stream URL (STA connected to Rover AP)
const char* ESP32_CAM_STREAM = "http://192.168.4.2/stream";

WebServer server(80);

// ================= SERVOS =================
Servo camServo;
Servo gunServo;

#define SERVO_FREQ 50
#define SERVO_MIN  500
#define SERVO_MAX  2400

// ================= GPS =================
TinyGPSPlus gps;
HardwareSerial GPS_Serial(1);

// ================= MOTOR PINS =================
int m1 = 13;
int m2 = 12;
int m3 = 32;
int m4 = 33;

// ================= OTHER PINS =================
int LED_PIN   = 22;
int SERVO1_PIN = 23;   // Camera pan
int SERVO2_PIN = 21;   // Gun tilt

int RELAY1 = 16;   // Always HIGH
int RELAY2 = 17;   // Pump (ACTIVE LOW)

// ================= GPS DATA =================
float lat = 0, lng = 0;

// ================= FIRE SAFETY =================
bool pumpState = false;
unsigned long lastFireTime = 0;
const unsigned long FIRE_GAP = 1000;

// ================= HTML PAGE =================
String getHTML() {
  String html =
  "<!DOCTYPE html><html><head>"
  "<meta name='viewport' content='width=device-width, initial-scale=1'>"
  "<style>"
  "body{margin:0;background:#111;color:#fff;font-family:Arial;text-align:center}"
  ".box{max-width:420px;margin:auto;padding:10px}"
  "img{width:100%;border-radius:12px;border:2px solid #333;background:black}"
  ".section{margin-top:15px}"
  ".grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}"
  ".grid2{display:grid;grid-template-columns:1fr 1fr;gap:10px}"
  ".btn{padding:16px 0;font-size:15px;border:none;border-radius:14px;font-weight:bold}"
  ".green{background:#2ecc71;color:#000}"
  ".red{background:#e74c3c;color:#fff}"
  ".blue{background:#3498db;color:#fff}"
  ".yellow{background:#f1c40f;color:#000}"
  ".stop{background:#ff3b3b;color:#fff}"
  "</style>"
  "<script>"
  "let lock=false;"
  "function fire(){"
  " if(lock) return;"
  " lock=true;"
  " fetch('/fire');"
  " setTimeout(()=>lock=false,700);"
  "}"
  "</script>"
  "</head><body><div class='box'>";

  html += "<h2>ESP32 ROVER</h2>";

  // VIDEO
  html += "<img src='" + String(ESP32_CAM_STREAM) + "'>";

  // DRIVE
  html += "<div class='section'><div class='grid3'>";
  html += "<div></div><button class='btn green' onclick=\"fetch('/f')\">FWD</button><div></div>";
  html += "<button class='btn green' onclick=\"fetch('/l')\">LEFT</button>";
  html += "<button class='btn stop' onclick=\"fetch('/s')\">STOP</button>";
  html += "<button class='btn green' onclick=\"fetch('/r')\">RIGHT</button>";
  html += "<div></div><button class='btn green' onclick=\"fetch('/b')\">BACK</button><div></div>";
  html += "</div></div>";

  // LIGHT
  html += "<div class='section'><div class='grid2'>";
  html += "<button class='btn blue' onclick=\"fetch('/lon')\">LIGHT ON</button>";
  html += "<button class='btn blue' onclick=\"fetch('/loff')\">LIGHT OFF</button>";
  html += "</div></div>";

  // CAMERA PAN
  html += "<div class='section'><div class='grid3'>";
  html += "<button class='btn green' onclick=\"fetch('/sl')\">CAM L</button>";
  html += "<button class='btn green' onclick=\"fetch('/sc')\">CAM C</button>";
  html += "<button class='btn green' onclick=\"fetch('/sr')\">CAM R</button>";
  html += "</div></div>";

  // GUN TILT
  html += "<div class='section'><div class='grid2'>";
  html += "<button class='btn yellow' onclick=\"fetch('/gunup')\">GUN UP</button>";
  html += "<button class='btn yellow' onclick=\"fetch('/gundown')\">GUN DOWN</button>";
  html += "</div></div>";

  // FIRE
  html += "<div class='section'>";
  html += "<button class='btn red' style='width:100%' onclick='fire()'>FIRE</button>";
  html += "</div>";

  // GPS
  if (lat && lng) {
    html += "<div class='section'><a style='color:#00ffd5' target='_blank' href='https://maps.google.com/?q=";
    html += String(lat,6) + "," + String(lng,6) + "'>VIEW LOCATION</a></div>";
  } else {
    html += "<div class='section'><p style='color:#777'>Waiting for GPS...</p></div>";
  }

  html += "</div></body></html>";
  return html;
}

// ================= MOTOR =================
void move(int a,int b,int c,int d){
  digitalWrite(m1,a); digitalWrite(m2,b);
  digitalWrite(m3,c); digitalWrite(m4,d);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(m1,OUTPUT); pinMode(m2,OUTPUT);
  pinMode(m3,OUTPUT); pinMode(m4,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(RELAY1,OUTPUT);
  pinMode(RELAY2,OUTPUT);

  digitalWrite(LED_PIN,HIGH);
  digitalWrite(RELAY1,HIGH);
  digitalWrite(RELAY2,HIGH);

  // SERVOS
  camServo.setPeriodHertz(SERVO_FREQ);
 // gunServo.setPeriodHertz(SERVO_FREQ);
  camServo.attach(SERVO1_PIN, SERVO_MIN, SERVO_MAX);
 // gunServo.attach(SERVO2_PIN, SERVO_MIN, SERVO_MAX);
  camServo.write(90);
  //gunServo.write(90);

  // GPS
  GPS_Serial.begin(9600, SERIAL_8N1, 19, 18);

  // WIFI AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  Serial.print("Rover IP: ");
  Serial.println(WiFi.softAPIP()); // 192.168.4.1

  // ROUTES
  server.on("/", [](){ server.send(200,"text/html",getHTML()); });

  server.on("/f", [](){ move(1,0,1,0); server.send(200); });
  server.on("/b", [](){ move(0,1,0,1); server.send(200); });
  server.on("/l", [](){ move(0,0,1,0); server.send(200); });
  server.on("/r", [](){ move(1,0,0,0); server.send(200); });
  server.on("/s", [](){ move(0,0,0,0); server.send(200); });

  server.on("/lon", [](){ digitalWrite(LED_PIN,LOW); server.send(200); });
  server.on("/loff",[](){ digitalWrite(LED_PIN,HIGH); server.send(200); });

  server.on("/sl", [](){ camServo.write(60); server.send(200); });
  server.on("/sc", [](){ camServo.write(90); server.send(200); });
  server.on("/sr", [](){ camServo.write(120); server.send(200); });

  server.on("/gunup",   [](){ gunServo.write(60); server.send(200); });
  server.on("/gundown", [](){ gunServo.write(120); server.send(200); });

  server.on("/fire", [](){
    if (millis() - lastFireTime < FIRE_GAP) { server.send(200); return; }
    lastFireTime = millis();
    pumpState = !pumpState;
    digitalWrite(RELAY2, pumpState ? LOW : HIGH);
    server.send(200);
  });

  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();

  while (GPS_Serial.available()) {
    gps.encode(GPS_Serial.read());
    if (gps.location.isUpdated()) {
      lat = gps.location.lat();
      lng = gps.location.lng();
    }
  }
}
