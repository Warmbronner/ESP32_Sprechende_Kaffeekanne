#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

// =========================================
// WLAN
// =========================================
const char* ssid     = "Garten_3";
const char* password = "Schlaegel#1910";

// =========================================
// NTP
// =========================================
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600); // Start mit Winterzeit

// =========================================
// Pins
// =========================================
#define LED_PIN     2
#define NUM_LEDS    3
#define SERVO_PIN   5

#define DF_RX_PIN   16
#define DF_TX_PIN   17

// =========================================
// Servo Einstellungen
// =========================================
#define SERVO_MIN 12
#define SERVO_MAX 120

int servoPause = 600;

// =========================================
// Objekte
// =========================================
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
Servo movementServo;

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfPlayer;

int lastRunHour = -1;
int lastOffset = 3600;

// =========================================
// Sommerzeit (EU)
// =========================================
bool isDST(int year, int month, int day, int hour)
{
  if(month < 3 || month > 10) return false;
  if(month > 3 && month < 10) return true;

  int lastSunday = day - ((day + 6) % 7);

  if(month == 3)
    return lastSunday >= 25 && hour >= 2;

  if(month == 10)
    return !(lastSunday >= 25 && hour >= 3);

  return false;
}

// =========================================
// NeoPixel
// =========================================
void setNeoPixel(bool on)
{
  if (on)
  {
    for(int i=0;i<NUM_LEDS;i++)
      strip.setPixelColor(i, strip.Color(0,200,255));
  }
  else
  {
    strip.clear();
  }
  strip.show();
}

// =========================================
// Servo Bewegung
// =========================================
void servoMovement()
{
  movementServo.attach(SERVO_PIN);
  movementServo.setPeriodHertz(50);

  for(int i=0;i<2;i++)
  {
    movementServo.write(SERVO_MAX);
    delay(1000);

    delay(servoPause);

    movementServo.write(SERVO_MIN);
    delay(400);
  }

  movementServo.detach();
}

// =========================================
// Stundenroutine
// =========================================
void hourlyRoutine(int hour)
{
  Serial.printf(">>> Spiele Stunde: %02d\n", hour);

  setNeoPixel(true);

  dfPlayer.playMp3Folder(hour);

  delay(200);

  servoMovement();

  delay(5000);

  setNeoPixel(false);

  Serial.println(">>> Fertig");
}

// =========================================
// Setup
// =========================================
void setup()
{
  Serial.begin(115200);
  delay(1000);

  strip.begin();
  strip.clear();
  strip.show();

  movementServo.attach(SERVO_PIN);
  movementServo.write(SERVO_MIN);
  delay(300);
  movementServo.detach();

  Serial.print("Verbinde WLAN");

  WiFi.begin(ssid,password);

  while(WiFi.status()!=WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWLAN verbunden");

  timeClient.begin();

  dfSerial.begin(9600,SERIAL_8N1,DF_RX_PIN,DF_TX_PIN);

  if(!dfPlayer.begin(dfSerial))
  {
    Serial.println("DFPlayer Fehler!");
    while(true);
  }

  dfPlayer.volume(25);

  Serial.println("System bereit");
}

// =========================================
// Loop
// =========================================
void loop()
{
  timeClient.update();

  // aktuelle Zeit holen
  int hour24 = timeClient.getHours();
  int minute = timeClient.getMinutes();
  int second = timeClient.getSeconds();

  // Datum holen (für DST)
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);

  int year  = ptm->tm_year + 1900;
  int month = ptm->tm_mon + 1;
  int day   = ptm->tm_mday;

  // Sommer-/Winterzeit bestimmen
  int offset = 3600;
  if(isDST(year, month, day, hour24))
    offset = 7200;

  if(offset != lastOffset)
  {
    timeClient.setTimeOffset(offset);
    lastOffset = offset;

    Serial.printf("Offset geändert: %d Sekunden\n", offset);
  }

  // 12h Format
  int hour12 = hour24 % 12;
  if(hour12 == 0) hour12 = 12;

  Serial.printf("Zeit %02d:%02d:%02d | Ansage: %02d\n",
                hour24, minute, second, hour12);

  // Trigger zur vollen Stunde
  if(minute == 0 && second < 5 && hour12 != lastRunHour)
  {
    lastRunHour = hour12;
    hourlyRoutine(hour12);
  }

  delay(500);
}