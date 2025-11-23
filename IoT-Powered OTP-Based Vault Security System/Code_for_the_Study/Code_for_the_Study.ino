#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <base64.h>  // Install this via Library Manager

// WiFi credentials
const char* ssid = "MR_Nov25";
const char* password = "Lokith7778";

// Twilio credentials (from your dashboard)
const char* account_sid = "AC*********************************";
const char* auth_token = "*******************************";

// Twilio numbers
const char* from_number = "+1XXXXXXXXXX";     // Your Twilio trial number
const char* to_number = "+91XXXXXXXXXX";      // Your verified Indian number

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  // Generate a 6-digit OTP
  randomSeed(micros());
  int otp = random(100000, 999999);
  String message = "Your OTP is " + String(otp);
  Serial.println("Generated OTP: " + message);

  // Resolve Twilio domain (DNS test)
  IPAddress ip;
  if (!WiFi.hostByName("api.twilio.com", ip)) {
    Serial.println("❌ DNS lookup failed!");
    return;
  }
  Serial.print("Resolved api.twilio.com to IP: ");
  Serial.println(ip);

  // Prepare HTTPS connection
  WiFiClientSecure client;
  client.setInsecure();  // Disable SSL cert verification for simplicity

  bool connected = false;
  for (int i = 0; i < 3; i++) {
    if (client.connect("api.twilio.com", 443)) {
      connected = true;
      break;
    }
    Serial.println("Retrying Twilio connection...");
    delay(1000);
  }

  if (!connected) {
    Serial.println("❌ Connection to Twilio failed after retries.");
    return;
  }

  // Build message body
  String post_data = "To=" + urlencode(to_number) +
                     "&From=" + urlencode(from_number) +
                     "&Body=" + urlencode(message);

  // Encode auth
  String credentials = String(account_sid) + ":" + String(auth_token);
  String auth = base64::encode(credentials);

  // Build full POST request
  String request = "POST /2010-04-01/Accounts/" + String(account_sid) + "/Messages.json HTTP/1.1\r\n";
  request += "Host: api.twilio.com\r\n";
  request += "Authorization: Basic " + auth + "\r\n";
  request += "Content-Type: application/x-www-form-urlencoded\r\n";
  request += "Content-Length: " + String(post_data.length()) + "\r\n\r\n";
  request += post_data;

  // Send it
  client.print(request);

  // Read response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }

  String response = client.readString();
  Serial.println("📩 Twilio Response:");
  Serial.println(response);

  client.stop();
}

void loop() {
  // Nothing in loop — runs once at boot
}

// URL encode helper
String urlencode(String str) {
  String encoded = "";
  char c;
  char code0, code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      code0 = (c >> 4) & 0xF;
      code1 = c & 0xF;
      encoded += '%';
      encoded += "0123456789ABCDEF"[code0];
      encoded += "0123456789ABCDEF"[code1];
    }
  }
  return encoded;
}