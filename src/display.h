#pragma once
#include <RTClib.h>

void drawTimeScreen(DateTime &now);
void drawDateScreen(DateTime &now);
void drawNotifyScreen();
void drawNotifyListScreen();
void drawNotifyExpandedScreen();
String stripNonAscii(String input);
String truncate(String s, int maxLen);