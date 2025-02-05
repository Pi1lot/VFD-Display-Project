#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi Configuration
const char * ssid = "XXXXXXXX";
const char * password = "XXXXXXXX";

// Steam API Configuration
const char * STEAM_API_KEY = "XXXXXXXX";
const char * STEAM_ID = "XXXXXXXX";

//OpenWeatherMap configuration
const char * OPENWEATHERMAP_KEY = "XXXXXXXX";

String apiUrl = "https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/";

//API URL for OpenWeatherMap, adjust to fit your location and language
String url = "https://api.openweathermap.org/data/2.5/weather?q=Paris&lang=fr&units=metric&appid=";

// Base URL for images; it will be dynamically generated based on the game ID
String imageUrlBase = "https://raw.githubusercontent.com/Pi1lot/VFD-Display-Project/main/icons/raw/";
const char * RANDOM_IMAGE_INDEX = "F";

// Variables to display game and time
String currentGame = "No game";
unsigned long gameStartTime = 0; // Game start time in seconds

// Variables for image downloading
#define IMAGE_WIDTH 48  // Image width (multiple of 8)
#define IMAGE_HEIGHT 48 // Image height (multiple of 8)
#define EXPECTED_SIZE (IMAGE_HEIGHT * IMAGE_WIDTH / 8) // Expected image size
uint8_t imageBuffer[EXPECTED_SIZE] = {0}; // Buffer to store the downloaded image

// Delay for API requests
unsigned long previousMillis = 0;
const unsigned long interval = 5000;

bool idle_downloaded = false;

String city = "Nowhere";
String weather_summary = "Sunny, min. 3.0°C";
// Initialize the U8g2 display
U8G2_GP1287AI_256X50_F_4W_HW_SPI u8g2(U8G2_R2, 15, U8X8_PIN_NONE, 16);

// Create an instance of WiFiClientSecure for HTTPS handling
WiFiClientSecure client;

// Configure NTP
WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 0, 60000); // Offset 0 for UTC, refresh every 60 seconds

void setup() {
  Serial.begin(115200);

  // Initialize the display
  u8g2.begin();
  u8g2.setContrast(255); // Maximum brightness
  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_spleen16x32_mr);
  u8g2.drawStr(2, 29, "Display test...");
  Serial.println("Display test");
  u8g2.sendBuffer();
  delay(10000);
  u8g2.clearDisplay();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("\nWi-Fi connected!");

  // Initialize the NTP client
  timeClient.begin();

  // Adjust UTC offset here (in seconds)
  timeClient.setTimeOffset(3600); // Adjust for the timezone (e.g., 3600 for UTC+1)

  String weatherForecast = fetchWeatherForecast();

  int separatorIndex = weatherForecast.indexOf("#");

  if (separatorIndex != -1) { // Check that '#' exists in the string
    String beforeHash = weatherForecast.substring(0, separatorIndex); // Part before '#'
    String afterHash = weatherForecast.substring(separatorIndex + 1);  // Part after '#'
    city = beforeHash;
    weather_summary = afterHash;
    weather_summary = capitalizeFirstLetter(weather_summary);
    Serial.println("Before #: " + beforeHash);
    Serial.println("After #: " + afterHash);
  }
}

void loop() {
  // Make an API call every 5 seconds
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    updateCurrentGame(); // Retrieve current game information
  }

  // Update NTP time
  timeClient.update();

  // Display information on the screen
  displayGameInfo();
}

String capitalizeFirstLetter(String text) {
  if (text.length() == 0) return text; // Check if the string is empty

  text.setCharAt(0, toupper(text.charAt(0))); // Capitalize the first letter

  return text;
}

char randomLetter(char maxLetter) {
  if (maxLetter < 'A' || maxLetter > 'Z') {
    return 'A'; // Safety: return 'A' if the given letter is invalid
  }
  return 'A' + random(0, maxLetter - 'A' + 1);
}

void updateCurrentGame() {
  if (WiFi.status() == WL_CONNECTED) {
    // Use BearSSL for HTTPS
    client.setInsecure(); // Disable SSL certificate verification

    HTTPClient https;
    String url = apiUrl + "?key=" + STEAM_API_KEY + "&steamids=" + STEAM_ID;

    if (https.begin(client, url)) {
      int httpCode = https.GET();

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String payload = https.getString();

          StaticJsonDocument<1096> doc; // Increase size otherwise some games with long names won't work!

          DeserializationError error = deserializeJson(doc, payload);

          if (!error) {
            JsonArray players = doc["response"]["players"].as<JsonArray>();
            if (players.size() > 0) {
              const char * gameName = players[0]["gameextrainfo"];
              const char * gameId = players[0]["gameid"];

              if (gameName) {
                if (currentGame != String(gameName)) {
                  currentGame = String(gameName);
                  gameStartTime = millis() / 1000; // Set game start time
                  // Generate dynamic image URL
                  String imageUrl = imageUrlBase + String(gameId) + ".raw";
                  // Download image if a game is detected
                  if (downloadImage(imageUrl)) {
                    Serial.println("Image downloaded and displayed!");
                  } else {
                    String imageUrl = imageUrlBase + "0.raw";
                    downloadImage(imageUrl);
                    Serial.println("Default image used because the game doesn't have one!");
                  }
                  idle_downloaded = false;
                  Serial.println(idle_downloaded);
                }
              } else {
                currentGame = "No game";
                gameStartTime = 0;

                // Download the image 0.raw when no game is running
                Serial.println("Downloading IDLE image");
                Serial.println(idle_downloaded);

                // Download a random image
                if (idle_downloaded == false) {
                  char randomChar = randomLetter(RANDOM_IMAGE_INDEX[0]);
                  String imageUrl = imageUrlBase + randomChar + ".raw";
                  downloadImage(imageUrl);

                  idle_downloaded = true;
                }
                Serial.println(idle_downloaded);
              }
            }
          }
        }
      } else {
        Serial.printf("Error calling API: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.println("Unable to connect to the Steam API!");
    }
  } else {
    Serial.println("Wi-Fi not connected!");
  }
}

String fetchWeatherForecast() {
  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure(); // Disable SSL certificate verification
    HTTPClient https;

    url += OPENWEATHERMAP_KEY;

    if (https.begin(client, url)) {
      int httpCode = https.GET();
      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          WiFiClient * stream = https.getStreamPtr();
          StaticJsonDocument<1024> doc; // Small document to save RAM
          DeserializationError error = deserializeJson(doc, *stream);
          if (!error) {
            float tempMin = doc["main"]["temp_min"];
            float tempMax = doc["main"]["temp_max"];
            const char * weatherDesc = doc["weather"][0]["description"];
            const char * city = doc["name"];
            https.end(); // Free resources

            // Return the forecast string provided by OWM, with description and temperatures
            return String(city) + "#" + String(weatherDesc) + ", Min " + String(tempMin, 1) + "C Max " + String(tempMax, 1) + "C";
          } else {
            https.end();
            return "Forecast unavailable (JSON error)";
          }
        } else {
          https.end();
          return "HTTP error: " + String(httpCode);
        }
      } else {
        https.end();
        return "Error calling API";
      }
    } else {
      return "Unable to connect to the weather API!";
    }
  }
  return "WiFi not connected!";
}

bool downloadImage(String imageUrl) {
  HTTPClient https;
  if (https.begin(client, imageUrl)) {
    int httpCode = https.GET();

    if (httpCode == HTTP_CODE_OK) {
      WiFiClient * stream = https.getStreamPtr();
      size_t index = 0;

      while (stream->connected() && stream->available() && index < sizeof(imageBuffer)) {
        imageBuffer[index++] = stream->read();
      }

      if (index == EXPECTED_SIZE) {
        Serial.println("[HTTPS] Image downloaded successfully!");
        https.end();
        invertImage(); // Invert the image
        return true;
      } else {
        Serial.println("[HTTPS] Incorrect file size!");
      }
    } else {
      Serial.printf("[HTTPS] Error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.println("[HTTPS] Unable to connect to the image URL!");
  }
  return false;
}

void invertImage() {
  for (int i = 0; i < IMAGE_HEIGHT; i++) {
    for (int j = 0; j < IMAGE_WIDTH / 8; j++) {
      uint8_t byte = imageBuffer[i * (IMAGE_WIDTH / 8) + j];
      imageBuffer[i * (IMAGE_WIDTH / 8) + j] = reverseBits(byte);
    }
  }
}

uint8_t reverseBits(uint8_t byte) {
  uint8_t reversed = 0;
  for (int i = 0; i < 8; i++) {
    reversed = (reversed << 1) | (byte & 1);
    byte >>= 1;
  }
  return reversed;
}

void displayGameInfo() {
  // Calculate NTP time
  String formattedTime = timeClient.getFormattedTime();
  // Calculate elapsed time since game start
  unsigned long elapsedTime = 0;
  if (gameStartTime > 0) {
    elapsedTime = (millis() / 1000) - gameStartTime;
  }

  // Prepare the text to display
  char timeString[22];
  snprintf(timeString, sizeof(timeString), "Session: %02lu:%02lu:%02lu",
    elapsedTime / 3600, (elapsedTime / 60) % 60, elapsedTime % 60); // Hours:Minutes:Seconds
  // Display on the screen
  u8g2.clearBuffer(); // Clear only the text area

  u8g2.drawLine(50, 0, 54, 4);
  u8g2.drawLine(51, 0, 55, 4);
  u8g2.drawLine(50, 49, 54, 45);
  u8g2.drawLine(51, 49, 55, 45);

  u8g2.drawLine(54, 5, 54, 44);
  u8g2.drawLine(55, 5, 55, 44);

  u8g2.drawLine(55, 35, 249, 35);
  u8g2.drawLine(55, 36, 250, 36);

  u8g2.drawLine(250, 35, 254, 31);
  u8g2.drawLine(250, 36, 255, 31);

  u8g2.drawLine(254, 0, 254, 30);
  u8g2.drawLine(255, 0, 255, 30);

  u8g2.drawLine(200, 49, 204, 45);
  u8g2.drawLine(201, 49, 205, 45);

  u8g2.drawLine(204, 44, 204, 37);
  u8g2.drawLine(205, 44, 205, 37);

  if (currentGame == "No game") {

    // Display "IDLE" with a larger font when no game is running
    u8g2.setFont(u8g2_font_spleen8x16_mr);
    u8g2.drawStr(58, 15, city.c_str());
    u8g2.setFont(u8g2_font_spleen6x12_mf); // f for full -> accentuated characters
    u8g2.drawStr(58, 29, weather_summary.c_str());
    // Display a bar under the game name

    // Display the time via NTP
    u8g2.setFont(u8g2_font_spleen6x12_mr); // Smaller font for time
    u8g2.drawStr(58, 48, formattedTime.c_str()); // Display the time

  } else {

    if (currentGame.indexOf(":") != -1) {
      // Case where the text contains ":"
      String beforeColon = currentGame.substring(0, currentGame.indexOf(":") + 1);
      String afterColon = currentGame.substring(currentGame.indexOf(":") + 1);

      afterColon.trim();
      // Display the part before ":"
      u8g2.setFont(u8g2_font_spleen8x16_mr);
      u8g2.drawStr(58, 15, beforeColon.c_str());

      // Display the part after ":"
      u8g2.setFont(u8g2_font_spleen6x12_mr);
      u8g2.drawStr(58, 29, afterColon.c_str());
    } else {
      // Case where the text does not contain ":", handle based on length
      int textLength = currentGame.length();

      if (textLength <= 12) {
        u8g2.setFont(u8g2_font_spleen16x32_mr);
        u8g2.drawStr(58, 29, currentGame.c_str());
      } else if (textLength > 12 && textLength <= 16) {
        u8g2.setFont(u8g2_font_spleen12x24_mr);
        u8g2.drawStr(58, 29, currentGame.c_str());
      } else if (textLength > 16 && textLength <= 26) {
        u8g2.setFont(u8g2_font_spleen8x16_mr);
        u8g2.drawStr(58, 29, currentGame.c_str());
      } else {
        // If the text exceeds 26 characters (optional based on your needs)
        u8g2.setFont(u8g2_font_spleen6x12_mr);
        u8g2.drawStr(58, 29, currentGame.c_str());
      }
    }

    // Display elapsed time
    u8g2.setFont(u8g2_font_spleen6x12_mr); // Smaller font for time
    u8g2.drawStr(58, 48, timeString); // Elapsed time
  }

  // Display the image on the right (assuming it was downloaded correctly)
  u8g2.drawXBMP(1, 1, IMAGE_WIDTH, IMAGE_HEIGHT, imageBuffer);

  // Send data to the display
  u8g2.sendBuffer();
}
