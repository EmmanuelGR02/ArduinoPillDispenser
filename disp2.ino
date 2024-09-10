#include <Servo.h>
#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_VL53L0X.h>
#include "pitches.h"

RTC_DS3231 rtc;
Servo mainServo;
Servo doorServo;

int sensorLED = 8;
int errorLED = 11;
const int buzzerPin = 7;

// 1-digit display pins
int a = 2;
int b = 3;
int c = 4;
int d = 5;
int e = 6;
int f = 12;
int g = 13;


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

  // 1-digit display pins
  pinMode(a, OUTPUT);
  pinMode(b, OUTPUT);
  pinMode(c, OUTPUT);
  pinMode(d, OUTPUT);
  pinMode(e, OUTPUT);
  pinMode(f, OUTPUT);
  pinMode(g, OUTPUT);

  pinMode(sensorLED, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(errorLED, OUTPUT);

  // Initialize RCT and Distance sensor. Pause program if not able to initialize
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    digitalWrite(errorLED, HIGH);
    while (1); 
  } else {
    Serial.println("RTC initialized.");
    digitalWrite(errorLED, LOW);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    digitalWrite(errorLED, HIGH);
    while (1);
  }
  Serial.println(F("VL53L0X Initialized"));
  digitalWrite(errorLED, LOW);

  // 
  mainServo.attach(9);
  doorServo.attach(10);
  mainServo.write(90);
  doorServo.write(close);
  Serial.println("\nServo initialized.\n");
}

void loop() {
  DateTime now = rtc.now();

  // Check if RTC is working
  if (!rtc.begin() || rtc.lostPower()) {
    Serial.println("RTC lost signal or power.");
    digitalWrite(errorLED, HIGH);  // Turn on error LED
  } else {
    digitalWrite(errorLED, LOW);   // Turn off error LED
  }

  int minute = now.minute() + 1;
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

  int temp = convertTime(hour);
  Serial.println(temp);
  displayNumber(temp);
  

  // Debug message to check if measureDistance is working
  int distance = measureDistance();
  Serial.print("Measured distance: ");
  Serial.println(distance);
  if (distance < 0) { 
    Serial.println("VL53L0X lost signal or power.");
    digitalWrite(errorLED, HIGH);  // Turn on error LED
  } else if (!rtc.lostPower()) {  // LED stays off if no issues
    digitalWrite(errorLED, LOW);   // Turn off error LED if all is working
  }

  if (distance <= 125 && distance >= 80) {
    digitalWrite(sensorLED, HIGH);
  } else {
    digitalWrite(sensorLED, LOW);
  }

  delay(700);

  //Check if the dispensing condition is met
  if (hour == 7 && minute == 0 && second == 00) {
    Serial.println("Dispensing pills...");
    doorServo.write(open);
    delay(2000);
    dispensePills();
  }
}

// dispensing logic
void dispensePills() {
  int currDistance = measureDistance();
  int rotationTime = 2000;  
  unsigned long startTime = millis();  // Capture the start time
  unsigned long elapsedTime = 0;       //  time tracker

  // Start rotating the main servo
  mainServo.write(rotateSignal);
  Serial.println("Servo started rotating.");

  // Continuously check the distance and time while the servo is rotating
  while (elapsedTime < rotationTime) {
    currDistance = measureDistance();
    elapsedTime = millis() - startTime;  // update time passed

    // if the distance is out of the range, stop the servo
    if (currDistance > 125 || currDistance < 80) {
      mainServo.write(stopSignal);
      Serial.println("Servo stopped due to distance out of range.");
      
      // wait until the distance is back within the valid range
      while (currDistance > 125 || currDistance < 80) {
        currDistance = measureDistance();
        delay(100);  
      }

      //  once the distance is back in range, continue to rotate 
      Serial.println("Distance is back in range. Resuming servo rotation.");
      mainServo.write(rotateSignal);

      // Update the start time to reflect the time when the rotation resumed
      startTime = millis() - elapsedTime;  
    }
    delay(100);  
  }

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

// logic to display numbers
void displayNumber(int num) {
  // Turn off all segments initially
  digitalWrite(a, LOW);
  digitalWrite(b, LOW);
  digitalWrite(c, LOW);
  digitalWrite(d, LOW);
  digitalWrite(e, LOW);
  digitalWrite(f, LOW);
  digitalWrite(g, LOW);
  
  if (num == 1) {
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
  } else if (num == 2) {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(e, HIGH);
    digitalWrite(d, HIGH);
  } else if (num == 3) {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
  } else if (num == 4) {
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
  } else if (num == 5) {
    digitalWrite(a, HIGH);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
  } else if (num == 6) {
    digitalWrite(a, HIGH);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
  } else if (num == 7) {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(c, HIGH);
  } else if (num == 8) {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
    digitalWrite(f, HIGH);
    digitalWrite(g, HIGH);
  } else if (num == 9) {
    digitalWrite(a, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(g, HIGH);
    digitalWrite(f, HIGH);
  } else if (num == 10) {
    digitalWrite(g, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(d, HIGH);
    digitalWrite(e, HIGH);
  } else if (num == 11) {
    digitalWrite(e, HIGH);
    digitalWrite(c, HIGH);
    digitalWrite(b, HIGH);
    digitalWrite(d, HIGH);
  } else if (num == 12) {
    digitalWrite(a, HIGH);
    digitalWrite(d, HIGH);
  } else {
    Serial.println("1-digit display error");
    digitalWrite(errorLED, HIGH);
  }
}

int convertTime(int time) {
  if (time == 0 || time == 00) {
    return 12;  
  } else if (time >= 1 && time <= 12) {
    return time;  
  } else if (time >= 13 && time <= 23) {
    return time - 12;  
  } else {
    return -1;  
    Serial.println("Invallid time input!!");
    digitalWrite(errorLED, HIGH);
  }
}
