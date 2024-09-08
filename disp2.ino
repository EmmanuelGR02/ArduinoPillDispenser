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
const int rotateSignal = 95;  // Signal to make the servo rotate
const int rotationTime = 1200;

int open = 0;
int close = 90;

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
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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

  Serial.print("Current time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(hour, DEC);
  Serial.print(':');
  Serial.print(minute, DEC);
  Serial.print(':');
  Serial.print(second, DEC);
  Serial.println();
  

  // Debug message to check if measureDistance is working
  int distance = measureDistance();
  Serial.print("Measured distance: ");
  Serial.println(distance);

  if (distance <= 125 && distance >= 80) {
    digitalWrite(sensorLED, HIGH);
  } else {
    digitalWrite(sensorLED, LOW);
  }

  delay(700);

  //Check if the dispensing condition is met
  if (hour == 20 && minute == 24 && second == 00) {
    Serial.println("Dispensing pills...");
    doorServo.write(open);
    delay(2000);
    dispensePills();
  }
}


void dispensePills() {
  int currDistance = measureDistance();
  int rotationTime = 2000;  // Total desired rotation time in milliseconds
  unsigned long startTime = millis();  // Capture the start time
  unsigned long elapsedTime = 0;       // Initialize elapsed time tracker

  // Start rotating the main servo
  mainServo.write(rotateSignal);
  Serial.println("Servo started rotating.");

  // Continuously check the distance and time while the servo is rotating
  while (elapsedTime < rotationTime) {
    currDistance = measureDistance();
    elapsedTime = millis() - startTime;  // Update elapsed time

    // If the distance is out of the range, stop the servo
    if (currDistance > 125 || currDistance < 80) {
      mainServo.write(stopSignal);
      Serial.println("Servo stopped due to distance out of range.");
      
      // Wait until the distance is back within the valid range
      while (currDistance > 125 || currDistance < 80) {
        currDistance = measureDistance();
        delay(100);  // Small delay to avoid rapid checking
      }

      // Once the distance is back in range, resume the servo rotation
      Serial.println("Distance is back in range. Resuming servo rotation.");
      mainServo.write(rotateSignal);

      // Update the start time to reflect the time when the rotation resumed
      startTime = millis() - elapsedTime;  // Adjust start time so elapsed time continues correctly
    }

    delay(100);  // Small delay to avoid rapid checking
  }

  // Ensure the servo stops after the total rotation time
  mainServo.write(stopSignal);
  Serial.println("Pill dispensed\n");

  // Close the door and play the ready sound
  delay(1000);
  doorServo.write(close);
  readySound();
}


// measure distance logic
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

// play melody 
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
