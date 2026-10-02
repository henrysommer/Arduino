#include <WiFi.h>
#include <LiquidCrystal.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>

// WLAN-Daten eintragen
const char* WLAN_NAME = "RamRanch";
const char* WLAN_PASSWORT = "18Cowboycocks";

// LCD-Anschlüsse:
// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(23, 22, 21, 19, 18, 17);

// Zeitzone Deutschland inklusive Sommerzeit
const char* ZEITZONE =
  "CET-1CEST,M3.5.0/2,M10.5.0/3";

// Wetterdaten für Aachen
// Für einen anderen Ort Latitude und Longitude ändern
const char* WETTER_URL =
  "https://api.open-meteo.com/v1/forecast"
  "?latitude=50.79"
  "&longitude=6.06"
  "&current=temperature_2m"
  "&timezone=Europe%2FBerlin";

float aktuelleTemperatur = 0.0;
bool temperaturVorhanden = false;

unsigned long letzterWetterAbruf = 0;
bool ersterWetterAbrufErledigt = false;

const unsigned long WETTER_INTERVAL =
  10UL * 60UL * 1000UL;   // 10 Minuten


void setup() {
  lcd.begin(16, 2);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("WLAN verbinden");

  WiFi.begin(WLAN_NAME, WLAN_PASSWORT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTzTime(
    ZEITZONE,
    "pool.ntp.org",
    "time.nist.gov"
  );

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("--:--");

  lcd.setCursor(7, 0);
  lcd.print("--.-");
  lcd.print((char)223);
  lcd.print("C");

  letzterWetterAbruf = millis();
}


void loop() {
  uhrzeitAnzeigen();

  // Erster Wetterabruf nach 10 Sekunden
  if (!ersterWetterAbrufErledigt &&
      millis() >= 10000) {
    wetterAbrufen();
    ersterWetterAbrufErledigt = true;
  }

  // Danach alle 10 Minuten
  if (ersterWetterAbrufErledigt &&
      millis() - letzterWetterAbruf >= WETTER_INTERVAL) {
    wetterAbrufen();
  }

  delay(1000);
}


void uhrzeitAnzeigen() {
  struct tm zeit;

  lcd.setCursor(0, 0);
  lcd.print("                ");

  if (!getLocalTime(&zeit, 100)) {
    lcd.setCursor(0, 0);
    lcd.print("--:--");
  } else {
    char uhrzeit[6];

    strftime(
      uhrzeit,
      sizeof(uhrzeit),
      "%H:%M",
      &zeit
    );

    lcd.setCursor(0, 0);
    lcd.print(uhrzeit);
  }

  lcd.setCursor(7, 0);

  if (temperaturVorhanden) {
    lcd.print(aktuelleTemperatur, 1);
    lcd.print((char)223);
    lcd.print("C");
  } else {
    lcd.print("--.-");
    lcd.print((char)223);
    lcd.print("C");
  }
}


void wetterAbrufen() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  http.setTimeout(5000);

  if (!http.begin(client, WETTER_URL)) {
    return;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String antwort = http.getString();

    StaticJsonDocument<512> json;

    DeserializationError fehler =
      deserializeJson(json, antwort);

    if (!fehler) {
      aktuelleTemperatur =
        json["current"]["temperature_2m"];

      temperaturVorhanden = true;
    }
  }

  http.end();

  letzterWetterAbruf = millis();
}
