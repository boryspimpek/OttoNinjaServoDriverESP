#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

const char* ssid = "XXXXXXXXX";
const char* password = "XXXXXXXXXXXXX";

ESP8266WebServer server(80);

// Cztery obiekty Servo
Servo servoLeftFoot;
Servo servoLeftLeg;
Servo servoRightFoot;
Servo servoRightLeg;

// Piny
const int pinLeftFoot = D1;
const int pinLeftLeg = D2;
const int pinRightFoot = D5;
const int pinRightLeg = D6;

void handleServo() {
  if (!server.hasArg("name") || !server.hasArg("pos")) {
    server.send(400, "text/plain", "Brakuje 'name' lub 'pos'");
    return;
  }

  String name = server.arg("name");
  int pos = server.arg("pos").toInt();
  // pos = constrain(pos, 0, 180);  // Ogranicz zakres

  bool known = true;

  if (name == "left_foot") {
    servoLeftFoot.write(pos);
  } else if (name == "left_leg") {
    servoLeftLeg.write(pos);
  } else if (name == "right_foot") {
    servoRightFoot.write(pos);
  } else if (name == "right_leg") {
    servoRightLeg.write(pos);
  } else {
    known = false;
  }

  if (known) {
    Serial.println("Ustawiono " + name + " na: " + String(pos));
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Nieznane serwo: " + name);
  }
}

void setup() {
  Serial.begin(115200);

  // Dołącz serwa
  servoLeftFoot.attach(pinLeftFoot, 544, 2400);   // 360°
  servoLeftLeg.attach(pinLeftLeg, 544, 2400);     // 180°
  servoRightFoot.attach(pinRightFoot, 544, 2400); // 360°
  servoRightLeg.attach(pinRightLeg, 400, 2600);   // 180°

  // Połączenie WiFi
  WiFi.begin(ssid, password);
  Serial.print("Łączenie z WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nPołączono! IP: " + WiFi.localIP().toString());

  // Obsługa HTTP
  server.on("/servo", handleServo);
  server.begin();
  Serial.println("Serwer HTTP działa.");
}

void loop() {
  server.handleClient();
}
