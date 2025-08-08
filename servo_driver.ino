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

// Ustawienia Access Point
const char* ap_ssid = "OttoNinja";      // Nazwa hotspotu
const char* ap_password = "12345678";       // Hasło (min. 8 znaków)
IPAddress local_IP(192, 168, 4, 1);        // IP ESP8266
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

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

  // Konfiguracja Access Point
  Serial.println("Konfigurowanie Access Point...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.println("Access Point utworzony!");
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Hasło: ");
  Serial.println(ap_password);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  
  // Uruchomienie serwera UDP
  Udp.begin(localUdpPort);
  Serial.printf("Nasłuchiwanie na porcie UDP %d\n", localUdpPort);
  
  Serial.println("Robot gotowy do pracy!");
  Serial.println("Połącz się z siecią WiFi: " + String(ap_ssid));
  Serial.println("Wysyłaj komendy UDP na adres: " + WiFi.softAPIP().toString() + ":" + String(localUdpPort));
}

void loop() {
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    int len = Udp.read(incomingPacket, 255);
    if (len > 0) {
      incomingPacket[len] = 0;
    }
    
    Serial.printf("Odebrano %d bajtów od %s: %s\n", 
                  packetSize, 
                  Udp.remoteIP().toString().c_str(), 
                  incomingPacket);
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
