#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char* ssid = "jio 4G";
const char* password = "876543210";

// ESP32-CAM stream URL
const char* ESP32_CAM_IP = "http://192.168.4.1:81/stream";

WebServer server(80);
Servo myServo;

// ===== Motor Pins (L298N) =====
int m1 = 12;
int m2 = 13;
int m3 = 32;
int m4 = 33;

// ===== LED & SERVO =====
int LED_PIN   = 22;   // ACTIVE LOW
int SERVO_PIN = 23;

// ===== Web Page =====
String getHTML() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{background:#222;color:white;text-align:center;font-family:sans-serif}";
  html += ".btn{width:80px;height:60px;margin:5px;font-size:16px;border-radius:10px;border:none}";
  html += ".g{background:#4CAF50;color:white}.r{background:#f44336;color:white}.b{background:#2196F3;color:white}</style></head><body>";

  html += "<h2>ESP32 ROVER CONTROL</h2>";

  html += "<img src='" + String(ESP32_CAM_IP) + "' width='100%' style='max-width:400px'><br><br>";

  html += "<button class='btn g' onclick=\"fetch('/f')\">FWD</button><br>";
  html += "<button class='btn g' onclick=\"fetch('/l')\">LEFT</button>";
  html += "<button class='btn r' onclick=\"fetch('/s')\">STOP</button>";
  html += "<button class='btn g' onclick=\"fetch('/r')\">RIGHT</button><br>";
  html += "<button class='btn g' onclick=\"fetch('/b')\">BACK</button><br><br>";

  html += "<button class='btn b' onclick=\"fetch('/lon')\">LIGHT ON</button>";
  html += "<button class='btn b' onclick=\"fetch('/loff')\">LIGHT OFF</button><br><br>";

  html += "<button class='btn g' onclick=\"fetch('/sl')\">SERVO LEFT</button>";
  html += "<button class='btn g' onclick=\"fetch('/sc')\">SERVO CENTER</button>";
  html += "<button class='btn g' onclick=\"fetch('/sr')\">SERVO RIGHT</button>";

  html += "</body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);

  pinMode(m1, OUTPUT); pinMode(m2, OUTPUT);
  pinMode(m3, OUTPUT); pinMode(m4, OUTPUT);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED OFF

  myServo.attach(SERVO_PIN);
  myServo.write(90);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected IP: " + WiFi.localIP().toString());

  server.on("/", []() { server.send(200, "text/html", getHTML()); });

  server.on("/f", [](){ move(1,0,1,0); server.send(200); });
  server.on("/b", [](){ move(0,1,0,1); server.send(200); });
  server.on("/l", [](){ move(0,0,1,0); server.send(200); });
  server.on("/r", [](){ move(1,0,0,0); server.send(200); });
  server.on("/s", [](){ move(0,0,0,0); server.send(200); });

  server.on("/lon", [](){ digitalWrite(LED_PIN, LOW); server.send(200); });
  server.on("/loff", [](){ digitalWrite(LED_PIN, HIGH); server.send(200); });

  server.on("/sl", [](){ myServo.write(0); server.send(200); });
  server.on("/sc", [](){ myServo.write(90); server.send(200); });
  server.on("/sr", [](){ myServo.write(180); server.send(200); });

  server.begin();
}

void move(int a, int b, int c, int d) {
  digitalWrite(m1, a);
  digitalWrite(m2, b);
  digitalWrite(m3, c);
  digitalWrite(m4, d);
}

void loop() {
  server.handleClient();
}
