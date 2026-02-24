// ===== ESP32-CAM CODE =====
void setup() {
  Serial.begin(115200);  // Serial UART ส่งไป Arduino
  delay(1000);
}

void loop() {
  // ส่งคำสั่งตัวอย่างไป Arduino
  sendCommand('F');  // Forward
  delay(2000);

  sendCommand('S');  // Stop
  delay(2000);

  sendCommand('L');  // Left
  delay(2000);

  sendCommand('R');  // Right
  delay(2000);
}

// ฟังก์ชันสร้างคำสั่งแบบ %X# และส่ง Serial
void sendCommand(char cmd) {
  String command = "%";
  command += cmd;
  command += "#";

  Serial.print(command);   // ส่งไป Arduino
  Serial.println();        // เพิ่ม newline ช่วย debug (ถ้าต้องการ)
}