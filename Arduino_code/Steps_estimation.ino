#define TRIG_PIN 4    // GPIO 4 for TRIG (Output)
#define ECHO_PIN 38   // GPIO 38 for ECHO (Input)

float prevDistance = 0.0;
unsigned long prevStepTime = 0;
float stepFrequency = 0.0;
int stepCount = 0;
bool legNear = false;  // Flag to track when leg is detected near

void setup() {
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
}

float getDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // Timeout of 30ms to avoid blocking
    if (duration == 0) {
        return -1;  // No pulse detected
    }
    
    float distance = duration * 0.034 / 2;  // Convert to cm
    return distance;
}

void loop() {
    float distance = getDistance();
    unsigned long currentTime = millis();

    if (distance > 0) {  // Ignore invalid readings
        if (distance < 20.0 && !legNear) {  // Adjust threshold if needed
            legNear = true;
            stepCount++;

            if (prevStepTime > 0) {
                float stepTime = (currentTime - prevStepTime) / 1000.0;  // Convert to seconds
                stepFrequency = 1.0 / stepTime;
            }
            prevStepTime = currentTime;
        }

        if (distance > 25.0) {  // Leg moved away (reset detection)
            legNear = false;
        }

        // Print step frequency and distance for Serial Plotter (tab-separated)
        Serial.print(stepFrequency);
        Serial.print("\t");  
        Serial.println(distance);  

        delay(50);  // Smooth plotting
