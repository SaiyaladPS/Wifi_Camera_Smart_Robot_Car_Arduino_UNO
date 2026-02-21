#include "esp_camera.h"
#include <WiFi.h>

// Camera Pin Configuration for AI-Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// WiFi Credentials
const char* ssid = "Jay";
const char* password = "jay96778932";

// Web Server
WiFiServer server(80);

// Serial Communication with Arduino
#define SERIAL_BAUD 115200

// Command buffer
String commandBuffer = "";

void setup() {
  // Initialize Serial for Arduino communication
  Serial.begin(SERIAL_BAUD);
  
  // Initialize camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Frame size configuration
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  // Start web server
  server.begin();
  
  delay(100);
}

void loop() {
  // Handle web client connections
  WiFiClient client = server.available();
  
  if (client) {
    String request = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        
        if (c == '\n') {
          // Parse HTTP request
          if (request.indexOf("GET /") >= 0) {
            
            // Movement commands
            if (request.indexOf("GET /forward") >= 0) {
              sendCommandToArduino("%F#");
              sendHTMLResponse(client, "Forward");
            }
            else if (request.indexOf("GET /backward") >= 0) {
              sendCommandToArduino("%B#");
              sendHTMLResponse(client, "Backward");
            }
            else if (request.indexOf("GET /left") >= 0) {
              sendCommandToArduino("%L#");
              sendHTMLResponse(client, "Left");
            }
            else if (request.indexOf("GET /right") >= 0) {
              sendCommandToArduino("%R#");
              sendHTMLResponse(client, "Right");
            }
            else if (request.indexOf("GET /stop") >= 0) {
              sendCommandToArduino("%S#");
              sendHTMLResponse(client, "Stop");
            }
            
            // Servo commands
            else if (request.indexOf("GET /servo_up") >= 0) {
              sendCommandToArduino("%H#");
              sendHTMLResponse(client, "Servo Up");
            }
            else if (request.indexOf("GET /servo_down") >= 0) {
              sendCommandToArduino("%G#");
              sendHTMLResponse(client, "Servo Down");
            }
            
            // Mode commands
            else if (request.indexOf("GET /trace") >= 0) {
              sendCommandToArduino("%T#");
              sendHTMLResponse(client, "Line Tracing Mode");
            }
            else if (request.indexOf("GET /avoid") >= 0) {
              sendCommandToArduino("%A#");
              sendHTMLResponse(client, "Obstacle Avoidance Mode");
            }
            else if (request.indexOf("GET /follow") >= 0) {
              sendCommandToArduino("%Z#");
              sendHTMLResponse(client, "Follow Mode");
            }
            
            // Camera stream
            else if (request.indexOf("GET /stream") >= 0) {
              streamCamera(client);
            }
            
            // Main page
            else {
              sendMainPage(client);
            }
          }
          break;
        }
      }
    }
    
    client.stop();
  }
  
  delay(10);
}

void sendCommandToArduino(String command) {
  Serial.print(command);
  delay(10);
}

void sendHTMLResponse(WiFiClient &client, String message) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><meta charset='UTF-8'><title>ESP32-CAM Robot</title></head>");
  client.println("<body>");
  client.println("<h1>Command: " + message + "</h1>");
  client.println("<p><a href='/'>Back to Control Panel</a></p>");
  client.println("</body></html>");
}

void sendMainPage(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head>");
  client.println("<meta charset='UTF-8'>");
  client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
  client.println("<title>ESP32-CAM Robot Control</title>");
  client.println("<style>");
  client.println("body { font-family: Arial; text-align: center; margin: 20px; }");
  client.println("button { padding: 15px 30px; margin: 5px; font-size: 18px; cursor: pointer; }");
  client.println(".control { background-color: #4CAF50; color: white; border: none; border-radius: 5px; }");
  client.println(".stop { background-color: #f44336; color: white; border: none; border-radius: 5px; }");
  client.println(".mode { background-color: #2196F3; color: white; border: none; border-radius: 5px; }");
  client.println(".servo { background-color: #FF9800; color: white; border: none; border-radius: 5px; }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<h1>ESP32-CAM Robot Control</h1>");
  client.println("<h2>Camera Stream</h2>");
  client.println("<img src='/stream' width='640' height='480'>");
  client.println("<h2>Movement Control</h2>");
  client.println("<button class='control' onclick=\"location.href='/forward'\">Forward</button><br>");
  client.println("<button class='control' onclick=\"location.href='/left'\">Left</button>");
  client.println("<button class='stop' onclick=\"location.href='/stop'\">STOP</button>");
  client.println("<button class='control' onclick=\"location.href='/right'\">Right</button><br>");
  client.println("<button class='control' onclick=\"location.href='/backward'\">Backward</button><br>");
  client.println("<h2>Camera Servo</h2>");
  client.println("<button class='servo' onclick=\"location.href='/servo_up'\">Servo Up</button>");
  client.println("<button class='servo' onclick=\"location.href='/servo_down'\">Servo Down</button><br>");
  client.println("<h2>Auto Modes</h2>");
  client.println("<button class='mode' onclick=\"location.href='/trace'\">Line Tracing</button>");
  client.println("<button class='mode' onclick=\"location.href='/avoid'\">Obstacle Avoidance</button>");
  client.println("<button class='mode' onclick=\"location.href='/follow'\">Follow Mode</button>");
  client.println("</body></html>");
}

void streamCamera(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();
  
  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      continue;
    }
    
    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();
    client.write(fb->buf, fb->len);
    client.println();
    
    esp_camera_fb_return(fb);
    
    delay(30);
  }
}