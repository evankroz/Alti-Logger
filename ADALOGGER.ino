#include <Wire.h>
#include <Adafruit_DPS310.h>
#include <SD.h>
#include <RTCZero.h>

#define BUTTON_PIN A5
#define LED_PIN    8  // Green user LED next to SD card slot

Adafruit_DPS310 dps;
File logfile;
RTCZero rtc;

bool loggingStarted = false;
bool waiting = false;
unsigned long buttonPressMillis = 0;

char filename[20]; // Enough for "MMDDhhmm.TXT\0"

// Helper function to make a unique filename based on date and time
void makeFilename(char *buf) {
  uint8_t m = rtc.getMonth();
  uint8_t d = rtc.getDay();
  uint8_t h = rtc.getHours();
  uint8_t min = rtc.getMinutes();
  // Format: MMDDhhmm.TXT
  sprintf(buf, "%02d%02d%02d%02d.TXT", m, d, h, min);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  // Start RTC
  rtc.begin();
  // Make sure RTC time is set! If not, set it manually here
  // rtc.setTime(hours, minutes, seconds);
  // rtc.setDate(day, month, year);

  // Initialize SD card
  while (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    delay(1000);
  }

  // Generate unique filename using RTC
  makeFilename(filename);

  // Try to open logfile with the created name
  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) {
    Serial.printf("Couldn't open log file: %s\n", filename);
    while (1);
  } else {
    Serial.print("Logging to file: ");
    Serial.println(filename);
  }

  // Initialize DPS310 sensor
  if (!dps.begin_I2C()) {
    Serial.println("Couldn't find DPS310!");
    while (1);
  }

  Serial.println("Ready! Press the button to start logging.");
}

void loop() {
  if (!loggingStarted) {
    // Check if button is pressed (active LOW)
    if (!waiting && digitalRead(BUTTON_PIN) == LOW) {
      waiting = true;
      buttonPressMillis = millis();
      Serial.println("Button pressed. Waiting 10 seconds...");
    }

    // If in waiting period, flash LED every second
    if (waiting) {
      unsigned long elapsed = millis() - buttonPressMillis;
      // Flash every 1 second: even seconds ON, odd seconds OFF
      if ((elapsed / 1000) % 2 == 0) {
        digitalWrite(LED_PIN, HIGH); // LED ON
      } else {
        digitalWrite(LED_PIN, LOW);  // LED OFF
      }

      // After 10 seconds, start logging
      if (elapsed >= 10000) {
        loggingStarted = true;
        waiting = false;
        digitalWrite(LED_PIN, LOW); // Turn off LED
        Serial.println("10 seconds elapsed. Logging started!");
      }
    }

    delay(20);
    return;
  }

  // ---- Normal logging code ----
  sensors_event_t temp_event, pressure_event;
  dps.getEvents(&temp_event, &pressure_event);

  // Calculate altitude (using sea level pressure 1013.25 hPa)
  float altitude = 44330 * (1.0 - pow(pressure_event.pressure / 1013.25, 0.1903));
  Serial.println(altitude);
  logfile.println(altitude, 2);
  logfile.flush();  // Ensure data is written to SD
  delay(100);       // Log every 0.1 second
}
