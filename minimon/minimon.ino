#include <LiquidCrystal.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <DHT.h>

// WIFI and time setup
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* time_zone = "YOUR_TIME_ZONE"; 
const char* ntpServer = "pool.ntp.org";
const char* sloMonths[] = {"jan", "feb", "mar", "apr", "maj", "jun", "jul", "avg", "sep", "okt", "nov", "dec"};

// LCD setup
LiquidCrystal lcd(5, 7, 9, 11, 12, 16);
byte wifiOn[8] = {
  0b00000, 0b01110, 0b10001, 0b00100, 0b01010, 0b00000, 0b00100, 0b00000
};
byte wifiOff[8] = {
  0b00000, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b00000, 0b00000
};
byte degreeIcon[8] = {
  0b00110, 0b01001, 0b01001, 0b00110, 0b00000, 0b00000, 0b00000, 0b00000
};

// Weather setup
const char* meteoUrl = "METEO_ARSO_URL.xml"; 
String temp = "--";
String humidity = "--";
String windDir = "--";
String windSpeed = "--";

// DHT11 Setup
#define DHTPIN 15
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float inTemp = 0.0;
float inHum = 0.0;

// Checkers
unsigned long lastConnected;
unsigned long lastWeatherUpdate = 0;
unsigned long lastDisplaySwitch = 0;
unsigned long lastDHTRead = 0;

// 0 = Time, 1 = Outdoor Weather, 2 = Indoor Sensor
int currentScreen = 0;

void setup() {
  // LCD init
  lcd.begin(16, 2);
    // Save the custom characters to the LCD memory
  lcd.createChar(0, wifiOn);
  lcd.createChar(1, wifiOff);
  lcd.createChar(2, degreeIcon);

  // Initialize the DHT sensor
  dht.begin();

  // Other
  lcd.setCursor(0, 0);
  lcd.print("Povezujem se na:");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  lastConnected = millis();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Povezan!");
  delay(1000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sinhroniziram...");
  configTzTime(time_zone, ntpServer);
  delay(2000); 

  fetchWeatherData();
  lcd.clear();
}

// Helper function to extract text between XML tags
String extractString(String data, String startTag, String endTag) {
  int startIndex = data.indexOf(startTag);

  if (startIndex == -1) {
    return "--"; // Tag not found
  }

  startIndex += startTag.length();
  int endIndex = data.indexOf(endTag, startIndex);

  if (endIndex == -1) {
    return "--";
  }
  
  return data.substring(startIndex, endIndex);
}

void fetchWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(meteoUrl);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      temp = extractString(payload, "<t>", "</t>");
      humidity = extractString(payload, "<rh>", "</rh>");
      windDir = extractString(payload, "<dd_shortText>", "</dd_shortText>");
      windSpeed = extractString(payload, "<ff_val>", "</ff_val>");
    }
    http.end();
  }
}


void loop() {
  unsigned long currentMillis = millis();

  // --- 1. Manage Wi-Fi Connection ---
  if (WiFi.status() == WL_CONNECTED) {
    lastConnected = currentMillis;
  } else {
    if (currentMillis - lastConnected > 60000) {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      lastConnected = currentMillis; 
    }
  }

  // --- 2. Update Weather Data (Every 10 minutes) ---
  if (currentMillis - lastWeatherUpdate >= 600000) {
    fetchWeatherData();
    lastWeatherUpdate = currentMillis;
  }

  /// --- 3. Update DHT11 Sensor (Every minute) ---
  if (currentMillis - lastDHTRead >= 10000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // Check if the read failed, only update if it was successful
    if (!isnan(t)){
      inTemp = t;
    } 
    if (!isnan(h)) {
      inHum = h;
    } 
    
    lastDHTRead = currentMillis;
  }

  // --- 4. Switch Display Mode (Every 10 seconds) ---
  if (currentMillis - lastDisplaySwitch >= 10000) {
    currentScreen = (currentScreen + 1) % 3;
    lastDisplaySwitch = currentMillis;
    lcd.clear(); 
  }

// --- 5. Draw the Screen ---
  if (currentScreen == 1) { // Outdoor Weather
    lcd.setCursor(0, 0);
    lcd.print(temp);
    lcd.write(byte(2));
    lcd.print("C");
    
    lcd.setCursor(16 - windDir.length(), 0);
    lcd.print(windDir);
    
    lcd.setCursor(0, 1);
    lcd.print(humidity);
    lcd.print("%");

    String windFull = windSpeed + "m/s";
    lcd.setCursor(16 - windFull.length(), 1);
    lcd.print(windFull);

  } else if (currentScreen == 2) { // Indoor Sensor
    // Top Row: "V sobi: 24.5°C"
    lcd.setCursor(0, 0);
    lcd.print("V sobi: ");
    
    lcd.setCursor(8, 0);
    lcd.print(inTemp, 1); // The '1' means 1 decimal place
    lcd.write(byte(2));
    lcd.print("C");
    
    // Bottom Row: Aligned humidity
    lcd.setCursor(8, 1);
    lcd.print(inHum, 1);
    lcd.print("%");

  } else { // Time and Date
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char timeString[9];
      strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
      
      lcd.setCursor(4, 0); 
      lcd.print(timeString);
      
      lcd.setCursor(15, 0);
      if (WiFi.status() == WL_CONNECTED) lcd.write(byte(0));
      else lcd.write(byte(1));
      
      char dateString[16];
      snprintf(dateString, sizeof(dateString), "%d. %s %d", timeinfo.tm_mday, sloMonths[timeinfo.tm_mon], timeinfo.tm_year + 1900);
      int startPos = (16 - strlen(dateString)) / 2;
      
      lcd.setCursor(startPos, 1);
      lcd.print(dateString);
    }
  }

  // Delay slightly to prevent flickering, but keep loop responsive
  delay(200); 
}































