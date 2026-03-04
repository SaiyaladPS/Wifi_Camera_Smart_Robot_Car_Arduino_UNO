#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// ================== ตั้งค่า Wi-Fi ==================
const char* ssid = "Jay";
const char* password = "jay96778932";

// ================== GPIO ของ ESP32-CAM ==================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ================== Server ==================
WebServer server(80);       // UI + control
WiFiServer streamServer(81); // MJPEG stream

// ================== UART ==================
HardwareSerial mySerial(1);

// ================== Camera setup ==================
void setupCamera(){
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA; // ขนาดเล็กเพื่อ stream + control พร้อมกัน
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed 0x%x\n", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);    // พลิกแนวตั้ง
  s->set_hmirror(s, 0);  // ไม่พลิกแนวนอน
}

// ================== ส่งคำสั่งไป Arduino ==================
void sendCommandToArduino(String cmd){
  mySerial.print("%" + cmd + "#");
}

// ================== HTML หน้า UI ==================
String htmlPage() {
  String ipStr = WiFi.localIP().toString();
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="th">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<meta charset="UTF-8">
<title>ESP32-CAM Smart Car</title>
<link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;700;900&family=Share+Tech+Mono&display=swap" rel="stylesheet">
<style>
  :root {
    --cyan: #00f5ff;
    --green: #00ff88;
    --orange: #ff6b35;
    --red: #ff2244;
    --purple: #bf5fff;
    --blue: #2979ff;
    --bg: #040810;
    --panel: #080f1a;
    --border: rgba(0,245,255,0.18);
    --glow-c: 0 0 8px var(--cyan), 0 0 20px rgba(0,245,255,0.3);
    --glow-g: 0 0 8px var(--green), 0 0 20px rgba(0,255,136,0.3);
    --glow-o: 0 0 8px var(--orange), 0 0 20px rgba(255,107,53,0.3);
  }

  * { box-sizing: border-box; -webkit-tap-highlight-color: transparent;
      -webkit-user-select: none; user-select: none; touch-action: none; }

  body {
    margin: 0; padding: 0; background: var(--bg);
    font-family: 'Share Tech Mono', monospace;
    color: var(--cyan); overflow: hidden;
    width: 100vw; height: 100dvh;
  }

  /* Animated background grid */
  body::before {
    content: ''; position: fixed; inset: 0; z-index: 0;
    background-image:
      linear-gradient(rgba(0,245,255,0.03) 1px, transparent 1px),
      linear-gradient(90deg, rgba(0,245,255,0.03) 1px, transparent 1px);
    background-size: 40px 40px;
    animation: gridScroll 20s linear infinite;
  }
  @keyframes gridScroll {
    0% { background-position: 0 0; }
    100% { background-position: 40px 40px; }
  }

  /* Scanline overlay */
  body::after {
    content: ''; position: fixed; inset: 0; z-index: 1; pointer-events: none;
    background: repeating-linear-gradient(
      0deg, transparent, transparent 2px,
      rgba(0,0,0,0.15) 2px, rgba(0,0,0,0.15) 4px
    );
  }

  /* ============ LAYOUT ============ */
  .cockpit {
    position: relative; z-index: 2;
    width: 100%; height: 100dvh;
    display: grid;
    padding: 6px;
    gap: 6px;
  }

  /* Portrait (default) */
  @media (orientation: portrait) {
    .cockpit {
      grid-template-rows: auto 1fr auto;
      grid-template-columns: 1fr;
      grid-template-areas:
        "header"
        "camera"
        "controls";
    }
    .controls-area {
      grid-template-columns: 1fr auto 1fr;
      grid-template-rows: auto auto;
      grid-template-areas:
        "modes modes modes"
        "left center right";
    }
  }

  /* Landscape */
  @media (orientation: landscape) {
    .cockpit {
      grid-template-rows: auto 1fr;
      grid-template-columns: 90px 1fr 90px;
      grid-template-areas:
        "header header header"
        "left camera right";
    }
    .controls-area {
      display: contents;
    }
    .left-drive { grid-area: left; }
    .right-steer { grid-area: right; }
    .center-panel {
      display: flex; flex-direction: column;
      gap: 6px; align-items: center;
      overflow: hidden;
    }
    .mode-bar {
      order: 2; width: 100%;
    }
    .camera-section { order: 1; flex: 1; }
  }

  /* ============ HEADER ============ */
  .header {
    grid-area: header;
    display: flex; align-items: center; justify-content: space-between;
    padding: 4px 8px;
    border: 1px solid var(--border);
    border-radius: 8px;
    background: rgba(0,245,255,0.04);
    backdrop-filter: blur(4px);
  }
  .logo {
    font-family: 'Orbitron', sans-serif;
    font-weight: 900; font-size: clamp(11px, 3vw, 16px);
    letter-spacing: 2px; color: var(--cyan);
    text-shadow: var(--glow-c);
  }
  .status-dots { display: flex; gap: 6px; align-items: center; }
  .dot {
    width: 8px; height: 8px; border-radius: 50%;
    animation: pulse 2s ease-in-out infinite;
  }
  .dot-online { background: var(--green); box-shadow: var(--glow-g); }
  .dot-cam { background: var(--cyan); box-shadow: var(--glow-c); animation-delay: 0.5s; }
  @keyframes pulse {
    0%,100% { opacity: 1; } 50% { opacity: 0.3; }
  }
  .status-text { font-size: 9px; color: var(--green); letter-spacing: 1px; }

  /* ============ CAMERA ============ */
  .camera-section {
    grid-area: camera;
    position: relative;
    border-radius: 10px;
    overflow: hidden;
    border: 1px solid var(--border);
    background: #000;
  }
  @media (orientation: portrait) {
    .camera-section { max-height: 45vh; }
  }

  .camera-section img {
    width: 100%; height: 100%;
    object-fit: contain; display: block;
  }

  /* HUD overlay corners */
  .hud-corner {
    position: absolute; width: 18px; height: 18px;
    border-color: var(--cyan); border-style: solid; opacity: 0.7;
  }
  .hud-tl { top: 6px; left: 6px; border-width: 2px 0 0 2px; }
  .hud-tr { top: 6px; right: 6px; border-width: 2px 2px 0 0; }
  .hud-bl { bottom: 6px; left: 6px; border-width: 0 0 2px 2px; }
  .hud-br { bottom: 6px; right: 6px; border-width: 0 2px 2px 0; }

  .hud-label {
    position: absolute; bottom: 8px; left: 50%; transform: translateX(-50%);
    font-size: 9px; color: var(--cyan); opacity: 0.6; letter-spacing: 2px;
    white-space: nowrap;
  }
  .cam-status {
    position: absolute; top: 8px; left: 50%; transform: translateX(-50%);
    font-size: 9px; color: var(--green); letter-spacing: 1px;
    display: flex; align-items: center; gap: 4px;
  }
  .rec-dot {
    width: 6px; height: 6px; border-radius: 50%;
    background: var(--red); box-shadow: 0 0 6px var(--red);
    animation: pulse 1s ease-in-out infinite;
  }

  /* ============ CONTROLS AREA ============ */
  .controls-area {
    grid-area: controls;
    display: grid;
    gap: 6px;
  }

  /* ============ PANEL BASE ============ */
  .panel {
    background: rgba(8,15,26,0.9);
    border: 1px solid var(--border);
    border-radius: 10px;
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    padding: 6px;
    gap: 6px;
    backdrop-filter: blur(8px);
  }
  .panel-label {
    font-family: 'Orbitron', sans-serif;
    font-size: 8px; letter-spacing: 2px;
    color: rgba(0,245,255,0.45);
    text-transform: uppercase;
  }

  /* ============ BUTTONS ============ */
  .btn {
    border: none; cursor: pointer;
    font-family: 'Orbitron', sans-serif;
    font-weight: 700; letter-spacing: 1px;
    border-radius: 10px;
    display: flex; align-items: center; justify-content: center;
    flex-direction: column;
    gap: 2px;
    transition: transform 0.08s, box-shadow 0.08s;
    position: relative; overflow: hidden;
  }
  .btn::before {
    content: ''; position: absolute; inset: 0;
    background: rgba(255,255,255,0.08);
    opacity: 0; transition: opacity 0.1s;
  }
  .btn:active::before { opacity: 1; }
  .btn:active { transform: scale(0.93); }

  /* Drive buttons */
  .btn-drive {
    width: clamp(54px, 16vw, 72px);
    height: clamp(54px, 16vw, 72px);
    font-size: clamp(9px, 2.5vw, 13px);
  }
  .btn-fwd {
    background: linear-gradient(145deg, #005c28, #00ff88);
    box-shadow: 0 4px 12px rgba(0,255,136,0.4), inset 0 1px rgba(255,255,255,0.2);
    color: #001a0e;
  }
  .btn-fwd:active { box-shadow: 0 1px 4px rgba(0,255,136,0.3); }

  .btn-rev {
    background: linear-gradient(145deg, #6b2900, #ff6b35);
    box-shadow: 0 4px 12px rgba(255,107,53,0.4), inset 0 1px rgba(255,255,255,0.2);
    color: #1a0a00;
  }

  .btn-turn {
    width: clamp(54px, 16vw, 72px);
    height: clamp(54px, 16vw, 72px);
    border-radius: 50%;
    font-size: clamp(9px, 2.5vw, 13px);
    background: linear-gradient(145deg, #001566, #2979ff);
    box-shadow: 0 4px 12px rgba(41,121,255,0.45), inset 0 1px rgba(255,255,255,0.2);
    color: #e8f0ff;
  }

  .btn-stop {
    width: clamp(54px, 14vw, 68px);
    height: clamp(38px, 9vw, 46px);
    font-size: clamp(9px, 2.2vw, 11px);
    border-radius: 8px;
    background: linear-gradient(145deg, #550011, #ff2244);
    box-shadow: 0 4px 14px rgba(255,34,68,0.5), inset 0 1px rgba(255,255,255,0.2);
    color: #fff;
    letter-spacing: 2px;
  }

  /* Mode buttons */
  .btn-mode {
    height: clamp(32px, 7vw, 40px);
    flex: 1;
    font-size: clamp(8px, 1.8vw, 10px);
    letter-spacing: 1px;
    border-radius: 7px;
  }
  .btn-servo-up {
    background: linear-gradient(145deg, #3a0066, #bf5fff);
    box-shadow: 0 3px 10px rgba(191,95,255,0.4);
    color: #f0e0ff;
  }
  .btn-servo-dn {
    background: linear-gradient(145deg, #3a0066, #7b2fbe);
    box-shadow: 0 3px 10px rgba(123,47,190,0.4);
    color: #f0e0ff;
  }
  .btn-trace {
    background: linear-gradient(145deg, #003344, #00c4d4);
    box-shadow: 0 3px 10px rgba(0,196,212,0.4);
    color: #e0faff;
  }
  .btn-avoid {
    background: linear-gradient(145deg, #443300, #ffc400);
    box-shadow: 0 3px 10px rgba(255,196,0,0.35);
    color: #1a1000;
  }
  .btn-follow {
    background: linear-gradient(145deg, #1a0044, #6c5ce7);
    box-shadow: 0 3px 10px rgba(108,92,231,0.4);
    color: #e8e0ff;
  }

  .btn-icon { font-size: 1.4em; line-height: 1; }

  /* ============ LEFT/RIGHT PANELS ============ */
  .left-drive, .right-steer {
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    gap: 8px;
  }

  @media (orientation: portrait) {
    .left-drive { grid-area: left; }
    .right-steer { grid-area: right; }
    .center-panel { grid-area: center; }
    .mode-bar { grid-area: modes; }
  }

  /* ============ CENTER PANEL ============ */
  .center-panel {
    display: flex; flex-direction: column;
    align-items: center; justify-content: center;
    gap: 6px;
  }
  .stop-row {
    display: flex; align-items: center; gap: 8px;
  }
  .divider {
    width: 1px; height: 30px;
    background: linear-gradient(to bottom, transparent, var(--border), transparent);
  }

  /* ============ MODE BAR ============ */
  .mode-bar {
    display: flex; gap: 5px;
    padding: 6px;
    background: rgba(8,15,26,0.85);
    border: 1px solid var(--border);
    border-radius: 10px;
    backdrop-filter: blur(8px);
  }

  /* ============ LANDSCAPE SIDE PANELS ============ */
  @media (orientation: landscape) {
    .left-drive, .right-steer {
      padding: 8px 4px;
      background: rgba(8,15,26,0.85);
      border: 1px solid var(--border);
      border-radius: 10px;
      backdrop-filter: blur(8px);
    }
    .btn-drive, .btn-turn {
      width: clamp(50px, 7vw, 68px);
      height: clamp(50px, 7vw, 68px);
    }
    .mode-bar {
      flex-wrap: wrap; justify-content: center;
    }
    .btn-mode {
      flex: none;
      width: clamp(60px, 10vw, 80px);
      height: clamp(30px, 5vh, 38px);
    }
  }

  /* Active state indicator on current mode */
  .btn-mode.active-mode {
    outline: 1.5px solid rgba(255,255,255,0.6);
    filter: brightness(1.2);
  }

  /* Speed indicator */
  .speed-ring {
    width: 38px; height: 38px;
    border-radius: 50%;
    border: 2px solid rgba(0,245,255,0.2);
    display: flex; align-items: center; justify-content: center;
    font-size: 9px; color: var(--cyan);
    flex-direction: column;
    position: relative;
  }
  .speed-ring::before {
    content: ''; position: absolute; inset: -2px;
    border-radius: 50%;
    border: 2px solid transparent;
    border-top-color: var(--cyan);
    animation: spinRing 1.5s linear infinite;
    opacity: 0;
    transition: opacity 0.2s;
  }
  .speed-ring.moving::before { opacity: 1; }
  @keyframes spinRing {
    from { transform: rotate(0deg); }
    to { transform: rotate(360deg); }
  }
  .speed-val { font-family: 'Orbitron', sans-serif; font-size: 8px; }
</style>
</head>
<body>

<div class="cockpit">

  <!-- HEADER -->
  <div class="header">
    <div class="logo">ESP32 ◈ ROVER</div>
    <div class="cam-status"><div class="rec-dot"></div><span>LIVE</span></div>
    <div class="status-dots">
      <div class="dot dot-online"></div>
      <div class="dot dot-cam"></div>
      <span class="status-text">ONLINE</span>
    </div>
  </div>

  <!-- CAMERA (portrait: own area | landscape: inside center-panel) -->
  <!-- Portrait layout wraps camera in its own grid area -->
  <!-- Landscape: camera is inside center-panel via flex order -->
  <div class="camera-section" id="camSection">
    <div class="hud-corner hud-tl"></div>
    <div class="hud-corner hud-tr"></div>
    <div class="hud-corner hud-bl"></div>
    <div class="hud-corner hud-br"></div>
    <div class="cam-status" style="top:8px;bottom:auto;">
      <div class="rec-dot"></div><span>CH-01 · WIDE</span>
    </div>
    <img src="http://192.168.1.100:81/" id="stream" alt="Camera Feed">
    <div class="hud-label">◈ VISUAL FEED ◈</div>
  </div>

  <!-- CONTROLS AREA -->
  <div class="controls-area">

    <!-- MODE BAR -->
    <div class="mode-bar">
      <button class="btn btn-mode btn-servo-up" onclick="send('H')">
        <span class="btn-icon">▲</span>CAM UP
      </button>
      <button class="btn btn-mode btn-servo-dn" onclick="send('G')">
        <span class="btn-icon">▼</span>CAM DN
      </button>
      <button class="btn btn-mode btn-trace" id="mTrace" onclick="setMode('T')">
        <span class="btn-icon">〰</span>TRACE
      </button>
      <button class="btn btn-mode btn-avoid" id="mAvoid" onclick="setMode('A')">
        <span class="btn-icon">⊕</span>AVOID
      </button>
      <button class="btn btn-mode btn-follow" id="mFollow" onclick="setMode('Z')">
        <span class="btn-icon">◎</span>FOLLOW
      </button>
      <button class="btn btn-mode btn-stop" onclick="send('S')">■ STOP</button>
    </div>

    <!-- LEFT: DRIVE (Forward / Backward) -->
    <div class="left-drive panel">
      <div class="panel-label">DRIVE</div>
      <button class="btn btn-drive btn-fwd"
        ontouchstart="startCmd('F')" ontouchend="stopCmd()"
        onmousedown="startCmd('F')" onmouseup="stopCmd()">
        <span class="btn-icon">▲</span>FWD
      </button>
      <div class="speed-ring" id="speedRing">
        <div class="speed-val" id="speedVal">—</div>
      </div>
      <button class="btn btn-drive btn-rev"
        ontouchstart="startCmd('B')" ontouchend="stopCmd()"
        onmousedown="startCmd('B')" onmouseup="stopCmd()">
        <span class="btn-icon">▼</span>REV
      </button>
    </div>

    <!-- CENTER: STOP -->
    <div class="center-panel">
      <div class="stop-row">
        <button class="btn btn-stop" onclick="send('S')">■ STOP</button>
      </div>
    </div>

    <!-- RIGHT: STEER (Left / Right) -->
    <div class="right-steer panel">
      <div class="panel-label">STEER</div>
      <button class="btn btn-turn"
        ontouchstart="startCmd('L')" ontouchend="stopCmd()"
        onmousedown="startCmd('L')" onmouseup="stopCmd()">
        <span class="btn-icon">◀</span>
      </button>
      <button class="btn btn-turn"
        ontouchstart="startCmd('R')" ontouchend="stopCmd()"
        onmousedown="startCmd('R')" onmouseup="stopCmd()">
        <span class="btn-icon">▶</span>
      </button>
    </div>

  </div><!-- /controls-area -->

</div><!-- /cockpit -->

<script>
  // ── Replace with your ESP32 IP ──
  const WEB_STREAM = "http://)rawliteral" + ipStr + R"rawliteral(:81/";
  const WEB_CONTROL = "http://)rawliteral" + ipStr + R"rawliteral(:80/"
  document.getElementById('stream').src = WEB_STREAM;

  let currentCmd = null;
  let cmdInterval = null;
  let activeMode = null;


  function send(cmd) {
    fetch(`${WEB_CONTROL}${cmd}`).catch(() => {});
  }

  function startCmd(cmd) {
    if (currentCmd === cmd) return;
    currentCmd = cmd;
    send(cmd);
    updateSpeedUI(cmd);
    cmdInterval = setInterval(() => send(cmd), 150);
  }

  function stopCmd() {
    clearInterval(cmdInterval);
    cmdInterval = null;
    currentCmd = null;
    send('S');
    updateSpeedUI(null);
  }

  function updateSpeedUI(cmd) {
    const ring = document.getElementById('speedRing');
    const val = document.getElementById('speedVal');
    if (cmd) {
      ring.classList.add('moving');
      const labels = { F:'FWD', B:'REV', L:'◀', R:'▶' };
      val.textContent = labels[cmd] || cmd;
    } else {
      ring.classList.remove('moving');
      val.textContent = '—';
    }
  }

  function setMode(cmd) {
    // Toggle mode off if already active
    if (activeMode === cmd) {
      send('S'); activeMode = null;
    } else {
      send(cmd); activeMode = cmd;
    }
    // Update button highlight
    ['mTrace','mAvoid','mFollow'].forEach(id => {
      document.getElementById(id)?.classList.remove('active-mode');
    });
    if (activeMode) {
      const map = { T:'mTrace', A:'mAvoid', Z:'mFollow' };
      document.getElementById(map[cmd])?.classList.add('active-mode');
    }
  }

  // ── Keyboard control ──
  const keyMap = { w:'F', arrowup:'F', s:'B', arrowdown:'B', a:'L', arrowleft:'L', d:'R', arrowright:'R' };
  document.addEventListener('keydown', e => {
    if (e.repeat) return;
    const cmd = keyMap[e.key.toLowerCase()];
    if (cmd) startCmd(cmd);
  });
  document.addEventListener('keyup', () => stopCmd());

  // ── Prevent pinch zoom ──
  document.addEventListener('gesturestart', e => e.preventDefault());
  document.addEventListener('touchmove', e => { if (e.touches.length > 1) e.preventDefault(); }, { passive: false });

  // ── Landscape: rearrange DOM ──
  function applyLayout() {
    const isLandscape = window.innerWidth > window.innerHeight;
    const cockpit = document.querySelector('.cockpit');
    const camSection = document.getElementById('camSection');
    const centerPanel = document.querySelector('.center-panel');

    if (isLandscape) {
      cockpit.style.gridTemplateAreas = '"header header header" "left camera right"';
      cockpit.style.gridTemplateColumns = '80px 1fr 80px';
      cockpit.style.gridTemplateRows = 'auto 1fr';
      camSection.style.gridArea = 'camera';
      document.querySelector('.controls-area').style.display = 'contents';
      document.querySelector('.left-drive').style.gridArea = 'left';
      document.querySelector('.right-steer').style.gridArea = 'right';
      document.querySelector('.mode-bar').style.cssText = 'grid-area:unset; position:absolute; bottom:8px; left:50%; transform:translateX(-50%); z-index:10; background:rgba(4,8,16,0.92);';
    } else {
      cockpit.style.gridTemplateAreas = '"header" "camera" "controls"';
      cockpit.style.gridTemplateColumns = '1fr';
      cockpit.style.gridTemplateRows = 'auto 1fr auto';
      camSection.style.gridArea = 'camera';
      document.querySelector('.controls-area').style.display = 'grid';
      document.querySelector('.mode-bar').style.cssText = '';
    }
  }

  window.addEventListener('resize', applyLayout);
  applyLayout();
</script>
</body>
</html>
  )rawliteral";
    return html;
  }
// ================== Handler ==================
void handleCommand() { 
  String cmd = server.uri();
  cmd = cmd.substring(1);
  sendCommandToArduino(cmd);
  server.send(200,"text/plain","OK");
}
void handleRoot() { server.send(200,"text/html", htmlPage()); }

// ================== Non-blocking stream ==================
void handleStream(WiFiClient client) {
client.printf("HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n");
  
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.printf("\r\n");
    esp_camera_fb_return(fb);

    // เช็คคำสั่งควบคุมจากพอร์ต 80 ระหว่าง stream ด้วย (ป้องกันอาการค้าง)
    server.handleClient(); 
    
    delay(1); // ให้เวลาระบบจัดการ Network เล็กน้อย
  }
  client.stop();
}

// ================== Setup ==================
void setup() {
  Serial.begin(9600);
  mySerial.begin(115200, SERIAL_8N1, -1, 13); // TX = GPIO13

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED){ delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Connected! IP: "); Serial.println(WiFi.localIP());

  setupCamera();

  server.on("/", handleRoot);
  server.on("/F", handleCommand);
  server.on("/B", handleCommand);
  server.on("/L", handleCommand);
  server.on("/R", handleCommand);
  server.on("/S", handleCommand);
  server.on("/H", handleCommand);
  server.on("/G", handleCommand);
  server.on("/T", handleCommand);
  server.on("/A", handleCommand);
  server.on("/Z", handleCommand);

  server.begin();
  streamServer.begin();
  Serial.println("HTTP server + Stream server started");
}

// ================== Loop ==================
void loop() {
  server.handleClient();  // Control/UI
      // Stream MJPEG
  WiFiClient sClient = streamServer.available();
  if (sClient) {
    handleStream(sClient);
  }
}