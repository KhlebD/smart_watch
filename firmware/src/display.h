#pragma once
#include <RTClib.h>


void drawTimeScreen(DateTime &now);
void drawDateScreen(DateTime &now);
void drawNotifyScreen();
void drawNotifyListScreen();
void drawNotifyExpandedScreen();
void drawBiDiText(int x, int y, const String &text, uint16_t color);
int  biDiTextWidth(const String &text);
String stripNonAscii(String input);
String truncate(String s, int maxLen);