#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Adafruit_Fingerprint.h>
#include <base64.h>

// Twilio credentials
String account_sid = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
String auth_token  = "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY";
String from_number = "+1XXXXXXXXXXXX";
String to_number   = "+91YYYYYYYYYYY";

// WiFi credentials
const char* ssid = "AAAAAAAA;
const char* password = "BBBBBBBBB";

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fingerprint setup
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Keypad setup
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Relay pin
#define RELAY_PIN 23

// Variables
String generatedOTP = "";
String enteredOTP = "";
int enrolledID = -1;

// Function declarations
bool sendSMS(String message);
String generateOTP();
int getEnrollID();
void enrollFingerprint(int id);
void verifyFingerprint();
void handleOTP();
void getOTPInput();

// --- Send SMS via Twilio ---
bool sendSMS(String message) {
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("api.twilio.com", 443)) {
    Serial.println("❌ Connection to Twilio failed!");
    return false;
  }

  String url = "/2010-04-01/Accounts/" + account_sid + "/Messages.json";
  String postData = "To=" + to_number + "&From=" + from_number + "&Body=" + message;
  String auth = base64::encode(account_sid + ":" + auth_token);

  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: api.twilio.com");
  client.println("Authorization: Basic " + auth);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(postData.length());
  client.println();
  client.println(postData);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line.startsWith("HTTP/1.1 201")) {
      Serial.println("✅ OTP sent successfully!");
      return true;
    } else if (line.startsWith("HTTP/1.1 400") || line.startsWith("HTTP/1.1 401")) {
      Serial.println("❌ Failed to send OTP via Twilio");
      return false;
    }
  }
  return false;
}

// --- Generate OTP ---
String generateOTP() {
  int otp = random(1000, 9999);
  return String(otp);
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  lcd.init();
  lcd.backlight();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print(".");
  }

  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);

  if (finger.verifyPassword()) {
    Serial.println("✅ Fingerprint sensor OK");
  } else {
    Serial.println("❌ Fingerprint sensor not found!");
    while (1);
  }

  lcd.clear();
  lcd.print("Press A to Enroll");
}

// --- Main Loop ---
void loop() {
  char key = keypad.getKey();

  if (key == 'A') {
    lcd.clear();
    lcd.print("Enter ID 1-127");
    int id = getEnrollID();
    if (id > 0) {
      enrollFingerprint(id);
      enrolledID = id;
    } else {
      lcd.clear();
      lcd.print("Invalid ID");
      delay(2000);
    }
    lcd.clear();
    lcd.print("Press # to Start");
  }

  if (key == '#') {
    if (enrolledID == -1) {
      lcd.clear();
      lcd.print("Enroll first!");
      delay(2000);
      lcd.clear();
      lcd.print("Press A to Enroll");
      return;
    }

    lcd.clear();
    lcd.print("Place Finger");
    verifyFingerprint();
  }
}

// --- Get Enroll ID ---
int getEnrollID() {
  String idStr = "";
  lcd.setCursor(0,1);
  lcd.print("ID: ");
  while (true) {
    char key = keypad.getKey();
    if (key >= '0' && key <= '9') {
      idStr += key;
      lcd.print(key);
    } else if (key == '#') break;
  }
  return idStr.toInt();
}

// --- Enroll Fingerprint ---
void enrollFingerprint(int id) {
  int p = -1;
  lcd.clear();
  lcd.print("Place Finger #1");

  while ((p = finger.getImage()) != FINGERPRINT_OK);
  finger.image2Tz(1);

  lcd.clear();
  lcd.print("Remove Finger");
  delay(2000);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear();
  lcd.print("Place Finger #2");
  while ((p = finger.getImage()) != FINGERPRINT_OK);
  finger.image2Tz(2);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    finger.storeModel(id);
    lcd.clear();
    lcd.print("Enroll Success!");
    Serial.println("✅ Enrolled ID: " + String(id));
  } else {
    lcd.clear();
    lcd.print("Enroll Failed!");
  }
  delay(2000);
}

// --- Verify fingerprint ---
void verifyFingerprint() {
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) return;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return;

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK && finger.fingerID == enrolledID) {
    lcd.clear();
    lcd.print("Match Found!");
    Serial.println("✅ Finger matched ID: " + String(finger.fingerID));
    delay(1000);
    handleOTP();
  } else {
    lcd.clear();
    lcd.print("Not Matched!");
    Serial.println("❌ Finger not recognized");
    delay(2000);
    lcd.clear();
    lcd.print("Press # to Try");
  }
}

// --- Handle OTP ---
void handleOTP() {
  generatedOTP = generateOTP();
  Serial.println("Generated OTP: " + generatedOTP);
  lcd.clear();
  lcd.print("Sending OTP...");
  
  bool sent = sendSMS("Your OTP is " + generatedOTP);
  delay(1000);

  if (!sent) {
    lcd.clear();
    lcd.print("SMS Failed");
    delay(2000);
    lcd.clear();
    lcd.print("Press # to Retry");
    return;
  }

  lcd.clear();
  lcd.print("Enter OTP:");
  enteredOTP = "";
  getOTPInput();
}

// --- Get OTP input ---
void getOTPInput() {
  while (true) {
    char key = keypad.getKey();
    if (key >= '0' && key <= '9') {
      enteredOTP += key;
      lcd.setCursor(enteredOTP.length() - 1, 1);
      lcd.print("*");
    }

    if (enteredOTP.length() == 4) {
      if (enteredOTP == generatedOTP) {
        lcd.clear();
        lcd.print("OTP Correct!");
        Serial.println("✅ OTP matched, unlocking...");
        digitalWrite(RELAY_PIN, HIGH);
        lcd.clear();
        lcd.print("Door Unlocked");
        delay(10000);
        digitalWrite(RELAY_PIN, LOW);
        lcd.clear();
        lcd.print("Door Locked");
        delay(2000);
      } else {
        lcd.clear();
        lcd.print("Wrong OTP");
        Serial.println("❌ Incorrect OTP");
        delay(2000);
      }
      lcd.clear();
      lcd.print("Press # to Start");
      break;
    }
  }
}
