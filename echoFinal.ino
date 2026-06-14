#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <FluxGarage_RoboEyes.h>//github/yt

// ---------- Pins ----------
#define I2C_ADDR    0x3c
const int ldrPin     = A1; //light detection
const int buzzerPin  = 8; //buzzer feedback
const int motorPin   = 3; //motor feedback  
const int trigPin    = 11; //pulse trigger
const int echoPin    = 10; //echo receiver
const int pirPin     = 4; //motion detection

// ---------- Display ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SH1106G> roboEyes(display);

// ---------- Thresholds ----------
const int lowLight = 200;
const int normLight = 250;
const int badPosture = 40;  
const int goodPosture = 45; 

// ---------- Timers ----------
unsigned long userAbsent = 0;
const unsigned long standbyMode = 10000; 
unsigned long userBadposture = 0;
const unsigned long angryMode = 2000; 
unsigned long lightLow = 0;
const unsigned long tiredMode = 1000; 
unsigned long moodTimer = 0;
const unsigned long happyFace = 2500; 

// ---------- States ----------
bool standby = false;
bool tired = false;
bool angry = false;
bool happy = false;
bool motorActive = false; 

// ---------- Setup ----------
void setup() {
  delay(3000);          
  Wire.begin();         
  delay(500);           

  pinMode(buzzerPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pirPin, INPUT);

  Serial.begin(115200);

  display.begin(I2C_ADDR, true);
  Wire.setClock(400000); 

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60); 
  roboEyes.setMood(DEFAULT);
  roboEyes.setIdleMode(ON, 5, 5); 
  roboEyes.setAutoblinker(ON, 5, 1);
  
  userAbsent = millis(); 
  standby = false;
}

// ---------- Functions ----------

void moodState(int mood, int buzzerFreq) {
    roboEyes.setMood(mood);        
    tone(buzzerPin, buzzerFreq, 200);   
    moodTimer = millis();       
}

int readDist() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 5000); 
  if (duration == 0) return 100; 

  int distanceCm = duration * 0.034 / 2;
  return distanceCm;
}

void checkDist() {
  if (millis() - userBadposture < angryMode) return;
  userBadposture = millis();

  int dist = readDist();
  if (!angry && dist <= badPosture) {
    angry = true;
    happy = false;
    moodState(ANGRY, 300);
    motorActive = true; 
  } else if (angry && dist > goodPosture) {
    angry = false;
    happy = true;
    moodState(HAPPY, 1200);
    motorActive = true;
  }

  if (happy && millis() - moodTimer >= happyFace) {
    roboEyes.setMood(DEFAULT);
    happy = false;
    motorActive = false;
    analogWrite(motorPin, 0); 

    if (tired) {
      roboEyes.setMood(TIRED);
      motorActive = true; 
    } else {
      roboEyes.setMood(DEFAULT);
  }
}
}

void checkLight() {
  if (millis() - lightLow < tiredMode) return;
  lightLow = millis();

  int ldrVal = analogRead(ldrPin);

  if (!tired && ldrVal <= lowLight) {
    tired = true;
    happy = false;
    moodState(TIRED, 300);
    motorActive = true;
  } else if (tired && ldrVal >= normLight) {
    tired = false;
    happy = true;
    moodState(HAPPY, 1200);
    motorActive = true;
    moodTimer = millis();
  }

  if (happy && millis() - moodTimer >= happyFace) {
    roboEyes.setMood(DEFAULT);
    happy = false;
    motorActive = false;
    analogWrite(motorPin, 0);

    if (tired) {
      roboEyes.setMood(TIRED);
      motorActive = true; 
    } else {
      roboEyes.setMood(DEFAULT); 
  }
}
}

void standbyCheck() {
  int pirState = digitalRead(pirPin);
  if (pirState == HIGH) {
    userAbsent = millis();
    if (standby) {
      standby = false;
      delay(100);             
      Wire.begin();          
      display.begin(I2C_ADDR, true); 
      roboEyes.setMood(DEFAULT);
    }
  } else {
    if (!standby && millis() - userAbsent > standbyMode) {
      standby = true;
      motorActive = false;
      analogWrite(motorPin, 0);
      
      display.clearDisplay();
      display.display(); 
      display.oled_command(0xAE); 
    }
  }
}

// ---------- Main Loop ----------
void loop() {
  standbyCheck();

  if (!standby) {
    if (motorActive) {
      unsigned long elapsed = millis() % 500; 

      if (elapsed < 150) {//first 150ms
        analogWrite(motorPin, 1); 
      } 
      else {
        analogWrite(motorPin, 0);
        roboEyes.update(); 
      }
    } 
    else {
      analogWrite(motorPin, 0);
      roboEyes.update();
    }

    checkLight();
    checkDist();
  }
}