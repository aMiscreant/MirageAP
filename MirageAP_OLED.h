// MirageAP_OLED.h
#ifndef MIRAGEAP_OLED_H
#define MIRAGEAP_OLED_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306* display;

void initOLED();
void showMessage(const String& line1, const String& line2 = "");

#endif
