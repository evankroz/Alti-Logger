#include <Wire.h>
#include <Adafruit_DPS310.h>
#include <SD.h>

Adafruit_DPS310 dps;
File logfile;

void setup() {
  Serial.begin(115200);
  //while (!Serial) delay(10);

  // Start SD card
  while (!SD.begin(4)) {
    Serial.println("SD initialization failed!");
    delay(1000);
  }

  logfile = SD.open("alt.txt", FILE_WRITE);
  if (!logfile) {
    Serial.println("Couldn't open log file!");
    while (1);
  }

  // Start DPS310 sensor
  if (!dps.begin_I2C()) {
    Serial.println("Couldn't find DPS310!");
    while (1);
  }
}

void loop() {
  sensors_event_t temp_event, pressure_event;
  dps.getEvents(&temp_event, &pressure_event);

  // Calculate altitude (using sea level pressure 1013.25 hPa)
  float altitude = 44330 * (1.0 - pow(pressure_event.pressure / 1013.25, 0.1903));
  Serial.println(altitude);
  logfile.print(altitude, 2);
  logfile.println();

  logfile.flush();  // Ensure data is written to SD

  delay(500);       // Log every half second
}
