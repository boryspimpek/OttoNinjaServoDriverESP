#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>

// Definicje serw
Servo servoLeftFoot;
Servo servoLeftLeg;
Servo servoRightFoot;
Servo servoRightLeg;

const int pinLeftFoot = D1;
const int pinLeftLeg = D2;
const int pinRightFoot = D5;
const int pinRightLeg = D6;

const bool CONTINUOUS_SERVOS[] = {false, true, false, true}; // LL, LF, RL, RF

// Pozycje neutralne
int currentPos[4] = {70, 92, 113, 92}; // LL, LF, RL, RF

// Ustawienia WiFi
const char* ssid = "xxxx";
const char* password = "xxxx";
WiFiUDP Udp;
unsigned int localUdpPort = 4210;
char incomingPacket[255];

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Inicjalizacja serw
  servoLeftFoot.attach(pinLeftFoot, 544, 2400);   // 360°
  servoLeftLeg.attach(pinLeftLeg, 544, 2400);     // 180°
  servoRightFoot.attach(pinRightFoot, 544, 2400); // 360°
  servoRightLeg.attach(pinRightLeg, 400, 2600);   // 180°
  
  // Ustaw pozycje neutralne
  moveToNeutral();

  // Łączenie z WiFi
  Serial.printf("Łączenie z %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" połączono");
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());
  
  Udp.begin(localUdpPort);
  Serial.printf("Nasłuchiwanie na porcie UDP %d\n", localUdpPort);
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    
    Serial.printf("Odebrano %d bajtów: %s\n", packetSize, incomingPacket);
    processCommand(incomingPacket);
  }
}

void processCommand(char* command) {
  char* token = strtok(command, ",");
  int targets[4] = {-1, -1, -1, -1};
  bool isSpeed[4] = {false, false, false, false}; // Czy wartość to prędkość (dla serw 360)
  
  // Parsowanie komendy
  while (token != NULL) {
    char servoName[3];
    int value;
    char type;
    
    // Nowy format: "LL:90a" (a=angle) lub "LF:90s" (s=speed)
    if (sscanf(token, "%2s:%d%c", servoName, &value, &type) == 3) {
      int servoIndex = -1;
      if (strcmp(servoName, "LL") == 0) servoIndex = 0;
      else if (strcmp(servoName, "LF") == 0) servoIndex = 1;
      else if (strcmp(servoName, "RL") == 0) servoIndex = 2;
      else if (strcmp(servoName, "RF") == 0) servoIndex = 3;
      
      if (servoIndex != -1) {
        targets[servoIndex] = value;
        isSpeed[servoIndex] = (type == 's');
      }
    }
    token = strtok(NULL, ",");
  }

  // Dla serw standardowych - płynny ruch
  const int steps = 10;
  int startAngles[4];
  
  for (int i = 0; i < 4; i++) {
    startAngles[i] = currentPos[i];
    if (targets[i] == -1) targets[i] = startAngles[i];
  }

  // Ruch dla standardowych serw
  for (int step = 1; step <= steps; step++) {
    for (int i = 0; i < 4; i++) {
      if (!CONTINUOUS_SERVOS[i] && targets[i] != startAngles[i]) {
        int intermediate = startAngles[i] + (targets[i] - startAngles[i]) * step / steps;
        
        switch(i) {
          case 0: servoLeftLeg.write(intermediate); break;
          case 1: break; // LF pomijamy - to serwo ciągłe
          case 2: servoRightLeg.write(intermediate); break;
          case 3: break; // RF pomijamy
        }
      }
    }
    delay(20);
  }

  // Natychmiastowe ustawienie serw ciągłych
  for (int i = 0; i < 4; i++) {
    if (CONTINUOUS_SERVOS[i] && targets[i] != -1) {
      int value = targets[i];
      if (isSpeed[i]) {
        // Dla prędkości (serwo ciągłe)
        switch(i) {
          case 1: servoLeftFoot.write(value); break;
          case 3: servoRightFoot.write(value); break;
        }
      } else {
        // Dla pozycji (jeśli ktoś poda kąt dla serwa ciągłego)
        switch(i) {
          case 1: servoLeftFoot.write(value); break;
          case 3: servoRightFoot.write(value); break;
        }
      }
    }
  }

  // Aktualizacja pozycji (tylko dla standardowych serw)
  for (int i = 0; i < 4; i++) {
    if (!CONTINUOUS_SERVOS[i]) {
      currentPos[i] = targets[i];
    }
  }
}

void smoothMove(int servoIndex, int targetAngle) {
  currentPos[servoIndex] = targetAngle;  // Od razu aktualizujemy docelową pozycję
}

void moveToNeutral() {
  smoothMove(0, 70);  // LL
  smoothMove(1, 92);  // LF
  smoothMove(2, 113); // RL
  smoothMove(3, 92);  // RF
}
