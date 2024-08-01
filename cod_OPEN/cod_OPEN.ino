#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <math.h>

const char* ssid = "Alex's S24";
const char* password = "qwertyui";

ESP8266WebServer server(80);

Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

int pos1 = 90;
int pos2 = 180;
int pos3 = 180;
int pos4 = 0;
bool servo3State = false;
bool servo4State = false;

const char* webpage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Catapulta Open</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background-color: #f0f0f0;
      margin: 0;
      padding: 0;
    }
    h1 {
      color: white;
      background-color: #4CAF50;
      padding: 20px;
      margin: 0;
      border-bottom: 2px solid #333;
    }
    h2 {
      color: #555;
    }
    button, input[type=range], input[type=text] {
      padding: 15px 30px;
      font-size: 20px;
      margin: 10px;
      cursor: pointer;
      border: none;
      color: white;
      background-color: #4CAF50;
      border-radius: 5px;
      transition: background-color 0.3s;
    }
    button:hover, input[type=range]:hover {
      background-color: #45a049;
    }
    .servo-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      background-color: white;
      margin: 20px;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
    .container {
      display: flex;
      flex-wrap: wrap;
      justify-content: center;
      padding: 20px;
    }
    canvas {
      border: 1px solid #000;
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <h1>Open InfoEducatie 2024</h1>
  <div class="container">
    <div class="servo-container">
      <h2>Direction Servo</h2>
      <input type="range" min="0" max="180" value="90" class="slider" id="servo1" onchange="updateServo(1, this.value)">
      <p>Angle: <span id="servo1Angle">90</span>&#176;</p>
    </div>
    <div class="servo-container">
      <h2>Arm Servo</h2>
      <input type="range" min="0" max="180" value="180" class="slider" id="servo2" onchange="updateServo(2, this.value)">
      <p>Angle: <span id="servo2Angle">180</span>&#176;</p>
    </div>
    <div class="servo-container">
      <h2>Launch Servo</h2>
      <button onclick="toggleLaunch()">Launch</button>
      <p>Angle: <span id="servo3Angle">180</span>&#176;</p>
    </div>
    <div class="servo-container">
      <h2>Extra Servo</h2>
      <button onclick="toggleExtra()">Toggle Extra</button>
      <p>Angle: <span id="servo4Angle">0</span>&#176;</p>
    </div>
    <div class="servo-container">
      <h2>Target Coordinates</h2>
      <input type="text" id="coordX" placeholder="X coordinate">
      <input type="text" id="coordY" placeholder="Y coordinate">
      <button onclick="setTarget()">Set Target</button>
    </div>
  </div>
  <canvas id="trajectoryCanvas" width="1400" height="300"></canvas>
  <script>
    function updateServo(servo, angle) {
      fetch(`/servo${servo}/move?angle=${angle}`)
      .then(response => response.text())
      .then(data => {
        document.getElementById(`servo${servo}Angle`).innerText = angle;
        drawTrajectory();
      });
    }
    function toggleLaunch() {
      fetch(`/servo3/toggle`)
      .then(response => response.text())
      .then(data => {
        document.getElementById('servo3Angle').innerText = data;
        drawTrajectory();
      });
    }
    function toggleExtra() {
      fetch(`/servo4/toggle`)
      .then(response => response.text())
      .then(data => {
        document.getElementById('servo4Angle').innerText = data;
      });
    }
    function setTarget() {
      const x = document.getElementById('coordX').value;
      const y = document.getElementById('coordY').value;
      fetch(`/setTarget?x=${x}&y=${y}`)
      .then(response => response.text())
      .then(data => {
        console.log(data);
        const angleDeg = Math.atan2(x, y) * (180 / Math.PI);
        const hypotenuse = Math.sqrt(x * x + y * y);
        const angle1 = map(angleDeg, -45, 45, 0, 180);
        const angle2 = map(hypotenuse, 0, 450, 180, 0) + 20;
        document.getElementById('servo1Angle').innerText = angle1;
        document.getElementById('servo2Angle').innerText = angle2;
        drawTrajectory();
      });
    }
    function drawTrajectory() {
      const canvas = document.getElementById('trajectoryCanvas');
      const ctx = canvas.getContext('2d');
      const angle2 = parseInt(document.getElementById('servo2Angle').innerText, 10);
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      // Draw X and Y axes
      ctx.beginPath();
      ctx.moveTo(50, canvas.height - 50);
      ctx.lineTo(canvas.width - 50, canvas.height - 50); // X axis
      ctx.moveTo(50, canvas.height - 50);
      ctx.lineTo(50, 50); // Y axis
      ctx.strokeStyle = 'black';
      ctx.stroke();

      // Map the servo angle to the launch angle (75 degrees at 0, 10 degrees at 180)
      const launchAngle = map(angle2, 0, 180, 10, 85) * (Math.PI / 180);
      const initialSpeed = map(angle2, 180, 0, 0, 5); // Max speed at 0 degrees, min at 180 degrees
      const g = 9.81; // Gravity in m/s^2

      // Time of flight calculation
      const timeOfFlight = (2 * initialSpeed * Math.sin(launchAngle)) / g;

      // Draw trajectory
      ctx.beginPath();
      ctx.moveTo(75, canvas.height - 75); 

      for (let t = 0;; t += 0.05) {
        const x = initialSpeed * t * Math.cos(launchAngle);
        const y = (initialSpeed * t * Math.sin(launchAngle)) - (0.1 * g * t * t);
        if (canvas.height - 75 - y * 200 >= canvas.height - 50) {
          break; // Stop the loop when the projectile hits the ground (X-axis)
        }
        ctx.lineTo(75 + x * 200, canvas.height - 75 - y * 200); // Scaling for visualization
      }


      ctx.strokeStyle = 'red';
      ctx.stroke();
    }

    drawTrajectory();

    function map(value, in_min, in_max, out_min, out_max) {
      return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
  </script>
</body>
</html>


)rawliteral";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  servo1.attach(D4);
  servo2.attach(D5);
  servo3.attach(D6);
  servo4.attach(D7);
  
  servo1.write(pos1);
  servo2.write(pos2);
  servo3.write(pos3);
  servo4.write(pos4);

  server.on("/", handleRoot);
  server.on("/servo1/move", [](){ handleServoMove(servo1, pos1); });
  server.on("/servo2/move", [](){ handleServoMove(servo2, pos2); });
  server.on("/servo3/toggle", [](){ handleServoToggle(servo3, pos3, servo3State); });
  server.on("/servo4/toggle", [](){ handleServoToggle(servo4, pos4, servo4State); });
  server.on("/setTarget", handleSetTarget);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void handleServoMove(Servo &servo, int &pos) {
  if (server.hasArg("angle")) {
    pos = server.arg("angle").toInt();
    servo.write(pos);
    server.send(200, "text/plain", String(pos));
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleServoToggle(Servo &servo, int &pos, bool &state) {
  if (state) {
    pos = 180;
  } else {
    pos = 0;
  }
  state = !state;
  servo.write(pos);
  server.send(200, "text/plain", String(pos));
}

void handleSetTarget() {
  if (server.hasArg("x") && server.hasArg("y")) {
    int x = server.arg("x").toInt();
    int y = server.arg("y").toInt();
    
    double hypotenuse = sqrt((double)x * x + (double)y * y);
    
    // Calculate the angle using atan2
    double angleRad = atan2(x, y);  // Angle in radians
    double angleDeg = angleRad * (180.0 / M_PI);  // Convert radians to degrees
    
    int angle1 = map(angleDeg, -45, 45, 0, 180);
    
    // Map the hypotenuse to the arm servo angle
    int angle2 = map(hypotenuse, 0, 450, 180, 0);  // Adjust as necessary based on your specific range requirements
    
    // Move the servos to the calculated angles
    servo1.write(angle1);
    servo2.write(angle2 + 20);
    
    // Send response back to the client
    server.send(200, "text/plain", "Target set: X=" + String(x) + ", Y=" + String(y) + ", Hypotenuse=" + String(hypotenuse) + ", Angle=" + String(angleDeg));
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}
