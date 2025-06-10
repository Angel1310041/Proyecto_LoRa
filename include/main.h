#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <AsyncTCP.h>
#include <RCSwitch.h>
#include "esp_task_wdt.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#define LED_PIN 35
#define MQ6_PIN 48
#define prog 0
#define ROWS 3
#define COLS 3

struct SENSOR {
  int id;
  int zona;
  int tipo;
};

extern const char* TipoSensor[9][2];
extern boolean variableDetectada;
extern byte rowPins[ROWS];
extern byte colPins[COLS];
extern bool modoprog;
extern SENSOR activo;
void imprimir(String m, String c="");

void setup();
void loop();

//C:\Users\angel\.platformio\packages\toolchain-xtensa-esp32s3\bin\xtensa-esp32s3-elf-addr2line.exe -pfiaC -e .pio\build\heltec_wifi_lora_32_V3\firmware.elf 0x4200ebe9 0x4200c387 0x4200c4d9 0x4200c6c1 0x4207b0b5 0x4207b12d 0x4207b95e

#endif 
