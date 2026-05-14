#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>

// ===============================
// Pins
// ===============================
#define LED_PIN     2
#define NUM_LEDS    2
#define SERVO_PIN   5

#define DF_RX_PIN   16
#define DF_TX_PIN   17

// ===============================
// Servo Einstellungen
// ===============================
#define SERVO_MIN 12
#define SERVO_MAX 120

int servoPause = 600;   // Pause nach Max-Position (ms)

// ===============================
// Objekte
// ===============================
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

Servo movementServo;

HardwareSerial dfSerial(1);
DFRobotDFPlayerMini dfPlayer;

// ===============================
// NeoPixel
// ===============================
void setNeoPixel(bool on)
{
  if(on)
  {
    for(int i=0;i<NUM_LEDS;i++)
      strip.setPixelColor(i, strip.Color(0,150,255));
  }
  else
  {
    strip.clear();
  }

  strip.show();
}

// ===============================
// Servo Bewegung
// ===============================
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

// ===============================
// Stundenroutine
// ===============================
void hourlyRoutine(int hour)
{
  Serial.print("Starte Routine fuer Stunde ");
  Serial.println(hour);

  // NeoPixel EIN
  setNeoPixel(true);

  // MP3 starten
  dfPlayer.playMp3Folder(hour);

  delay(200);

  // Servo starten
  servoMovement();

  // warten bis MP3 fertig
  while(dfPlayer.readState()==1)
  {
    delay(100);
  }

  // NeoPixel AUS
  setNeoPixel(false);

  Serial.println("Routine beendet\n");
}

// ===============================
// Setup
// ===============================
void setup()
{
  Serial.begin(115200);

  strip.begin();
  strip.clear();
  strip.show();

  movementServo.attach(SERVO_PIN);
  movementServo.write(SERVO_MIN);
  delay(300);
  movementServo.detach();

  dfSerial.begin(9600,SERIAL_8N1,DF_RX_PIN,DF_TX_PIN);

  if(!dfPlayer.begin(dfSerial))
  {
    Serial.println("DFPlayer Fehler!");
    while(true);
  }

  dfPlayer.volume(25);

  Serial.println("Testprogramm gestartet");
  Serial.println("Bitte Zahl 1 - 12 eingeben:");
}

// ===============================
// Loop
// ===============================
void loop()
{

  if(Serial.available())
  {
    int command = Serial.parseInt();

    if(command>=1 && command<=12)
    {
      hourlyRoutine(command);
    }
    else
    {
      Serial.println("Bitte Zahl 1 bis 12 eingeben.");
    }
  }

}