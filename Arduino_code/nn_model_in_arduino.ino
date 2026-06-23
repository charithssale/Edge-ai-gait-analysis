#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include <float.h>

#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21
#define TRIG_PIN 4
#define ECHO_PIN 39

float W1[2][10] = {
  {-3.10040451, -0.08790189, 4.45056796, 6.04434460, -0.34442951, -1.19261733, 11.8925587, 0.05383058, -0.71778514, -5.59220223},
  {-0.06947660, -0.24884962, 0.01157892, 0.03001448, -0.19047243, -0.93576308, -0.09986304, -0.10858069, -0.13750988, 0.1088018}
};
float b1[10] = {-2.41859308, -2.84315199, -5.54913339, -7.66488596, -3.36641391, -2.4058977, 7.23615425, -3.7337946, -2.44712898, -3.19801313};
float W2[10][3] = {
  {2.84339815, -0.19122471, -1.34477276}, {-0.65587516, -0.10194031, -1.10039286}, {-3.35886542, -3.86961187, 5.85254304},
  {-2.53729713, -5.16512653, 8.66130149}, {0.34667179, 0.70826482, -1.30359985}, {0.84661081, -0.19939261, -3.61886837},
  {-10.32929116, 9.36964121, 0.56679172}, {-0.90468898, 1.08420005, -0.42489477}, {0.21132478, -0.35421908, -2.51611067},
  {6.11767206, -2.70217893, -3.77779276}
};
float b2[3] = {3.03952821, 0.1725231, -3.21205131};

float mean[2] = {1.0, 30.0};
float stddev[2] = {0.5, 20.0};

const char* speedLabels[] = {"Slow", "Normal", "Fast"};
SSD1306Wire OledDisplay(0x3C, 400000, OLED_SDA, OLED_SCL, GEOMETRY_128_64, OLED_RST);

unsigned long prevStepTime = 0;
bool legNear = false;
float stepFrequency = 0.0;
int stepCount = 0;

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  OledDisplay.init();
  OledDisplay.clear();
  OledDisplay.display();
  Serial.println("Walking Speed Estimator Ready");
}

void loop() {
  float legDistance = getDistance();
  unsigned long currentTime = millis();
  if (legDistance < 25.0 && !legNear) {
    legNear = true;
    stepCount++;
    if (prevStepTime > 0) {
      float stepTime = (currentTime - prevStepTime) / 1000.0;
      stepFrequency = 1.0 / stepTime;
    }
    prevStepTime = currentTime;
  }
  if (legDistance > 35.0) {
    legNear = false;
  }

  float X[2];
  X[0] = (stepFrequency - mean[0]) / stddev[0];
  X[1] = (legDistance - mean[1]) / stddev[1];

  float hidden[10];
  for (int i = 0; i < 10; i++) {
    hidden[i] = b1[i];
    for (int j = 0; j < 2; j++) {
      hidden[i] += X[j] * W1[j][i];
    }
    hidden[i] = 1.0 / (1.0 + exp(-hidden[i]));
  }

  float output[3];
  float sumExp = 0;
  for (int i = 0; i < 3; i++) {
    output[i] = b2[i];
    for (int j = 0; j < 10; j++) {
      output[i] += hidden[j] * W2[j][i];
    }
    output[i] = exp(output[i]);
    sumExp += output[i];
  }
  for (int i = 0; i < 3; i++) {
    output[i] /= sumExp;
  }

  int predictedClass = 0;
  for (int i = 1; i < 3; i++) {
    if (output[i] > output[predictedClass]) {
      predictedClass = i;
    }
  }
  float strideLength = 0.65; // in meters (you can calibrate this)
  float walkingSpeed = strideLength * stepFrequency; // in m/s
  Serial.print(stepFrequency);
  Serial.print(',');
  Serial.print(legDistance);
  Serial.print(',');
  Serial.print(walkingSpeed, 2);
  Serial.print(',');
  Serial.println(speedLabels[predictedClass]);

  OledDisplay.clear();
  OledDisplay.setFont(ArialMT_Plain_10);
  OledDisplay.drawString(0, 28, "Dist cm: " + String(legDistance, 1));
  OledDisplay.drawString(0, 16, "Speed m/s: " + String(walkingSpeed, 2));
  OledDisplay.drawString(0, 40, "Class: " + String(speedLabels[predictedClass]));
  OledDisplay.display();

  delay(100); // Faster loop for better responsiveness
}