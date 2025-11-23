const int PIR_SENSOR_PIN = 8;
const int LED_PIN = 9;       
const int BUZZER_PIN = 10;   
void setup() {
  Serial.begin(9600);
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Motion Detection System Initialized");
}
void loop() {
  int sensorValue = digitalRead(PIR_SENSOR_PIN);
  if (sensorValue == HIGH) { 
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Motion Detected");
  } 
  else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("No Motion Detected");
  }
  delay(200);
}