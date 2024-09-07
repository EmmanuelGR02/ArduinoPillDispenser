#include <Servo.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_VL53L0X.h>
#include "pitches.h"

RTC_DS3231 rtc;
Servo mainServo;
Servo doorServo;

int sensorLED = 8;
const int buzzerPin = 7;


int melody[] = {
  NOTE_E5, NOTE_D5, NOTE_FS4, NOTE_GS4, 
  NOTE_CS5, NOTE_B4, NOTE_D4, NOTE_E4, 
  NOTE_B4, NOTE_A4, NOTE_CS4, NOTE_E4,
  NOTE_A4
};

int durations[] = {
  8, 8, 4, 4,
  8, 8, 4, 4,
  8, 8, 4, 4,
  2
};

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// positions of the pills and default
const int stopSignal = 90;  // Signal to stop the servo 
const int rotateSignal = 180;  // Signal to make the servo rotate
const int rotationTime = 1200;

int open = 0;
int close = 90;
int daily = 0;

void setup() {
  Serial.begin(9600);

  pinMode(sensorLED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // Initialize RCT and Distance sensor. Pause program if not able to initialize
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1); 
  } else {
    Serial.println("RTC initialized.");
  }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while (1);
  }
  Serial.println(F("VL53L0X Initialized"));

  // 
  mainServo.attach(9);
  doorServo.attach(10);
  mainServo.write(90);
  doorServo.write(close);
  Serial.println("\nServo initialized.\n");
}

void loop() {
  DateTime now = rtc.now();

  int minute = now.minute();
  int hour = now.hour(); 
  int second = now.second();

  hour -= 5;
  if (hour < 0) {
    hour += 24;
  }

  String meridiem = "AM";
  if (hour >= 12) {
    meridiem = "PM";
    if (hour > 12) {
      hour -= 12;
    }
  } else if (hour == 0) {
    hour = 12;
  }

  // Print current time
  Serial.print(hour, DEC);
  Serial.print(':');
  Serial.print(minute, DEC);
  Serial.print(':');
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.println(meridiem);

  // Debug message to check if measureDistance is working
  int distance = measureDistance();
  Serial.print("Measured distance: ");
  Serial.println(distance);

  if (distance <= 125) {
    digitalWrite(sensorLED, HIGH);
  } else {
    digitalWrite(sensorLED, LOW);
  }

  delay(700);

  // Check if the dispensing condition is met
  if (hour == 7 && minute == 9 && meridiem == "PM" && daily == 0) {
    Serial.println("Dispensing pills...");
    doorServo.write(open);
    delay(2000);
    dispensePills();
    daily = 1;
  }
}


void dispensePills() {
  int currDistance = measureDistance();
  unsigned long startTime = millis();

  // Start rotating the serv
  mainServo.write(100);

  // Keep rotating until one full rotation or distance exceeds 225
  while (millis() - startTime < rotationTime) {
    currDistance = measureDistance();
    if (currDistance >= 225) {
      mainServo.write(stopSignal);  // Stop the servo
      while (measureDistance() >= 225);  // Wait until distance is back to < 225
      break;
    }
  }

  mainServo.write(stopSignal);  // Ensure the servo stops after the rotation or distance check
  Serial.println("Pill dispensed\n");
  delay(1000);
  doorServo.write(close);
  readySound();

}

int measureDistance() {
  VL53L0X_RangingMeasurementData_t measure;

  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {  
    Serial.print("Distance (mm): "); Serial.println(measure.RangeMilliMeter);
    return measure.RangeMilliMeter;  
  } else {
    Serial.println("Error: No valid data received");
    return 0;
  }
}



void readySound() {
  int size = sizeof(durations) / sizeof(int);

  for (int note = 0; note < size; note++) {
    int duration = 1000 / durations[note];
    tone(buzzerPin, melody[note], duration);

    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    
    noTone(buzzerPin);
  }
}
