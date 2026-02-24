#include <WiFi.h>
#include <WebServer.h>

// ตั้งค่า Wi-Fi ของคุณ
const char* ssid = "Jay";
const char* password = "jay96778932";

// Web server บน port 80
WebServer server(80);

// UART1 ส่งไป Arduino
HardwareSerial mySerial(1);

// ฟังก์ชันส่งคำสั่ง
void sendCommandToArduino(String cmd) {
  mySerial.print(cmd); // ส่ง %F# %B# ...
}

// หน้าเว็บแบบ AJAX
String htmlPage() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-CAM Car</title>";
  html += "<style>button{width:80px;height:50px;margin:5px;font-size:18px;}</style>";
  html += "<script>";
  html += "function send(cmd){";
  html += "  var xhttp = new XMLHttpRequest();";
  html += "  xhttp.open('GET','/'+cmd,true);";
  html += "  xhttp.send();";
  html += "}";
  html += "</script></head><body>";
  html += "<h2>Control Car</h2>";
  html += "<button onclick=\"send('F')\">Forward</button>";
  html += "<button onclick=\"send('B')\">Backward</button>";
  html += "<button onclick=\"send('L')\">Left</button>";
  html += "<button onclick=\"send('R')\">Right</button>";
  html += "<button onclick=\"send('S')\">Stop</button>";
  html += "</body></html>";
  return html;
}

// Handler คำสั่ง
void handleCommand() {
  String cmd = server.uri();
  cmd = cmd.substring(1);
  String bleCmd = "%" + cmd + "#";
  sendCommandToArduino(bleCmd);
  server.send(200, "text/plain", "OK"); // ส่งตอบสั้น ๆ
}

// หน้า index
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(115200, SERIAL_8N1, -1, 13); // TX = GPIO13

  // เชื่อม Wi-Fi มือถือ
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // ตั้งค่า Web server
  server.on("/", handleRoot);
  server.on("/F", handleCommand);
  server.on("/B", handleCommand);
  server.on("/L", handleCommand);
  server.on("/R", handleCommand);
  server.on("/S", handleCommand);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); // ตรวจ request อย่างต่อเนื่อง
}