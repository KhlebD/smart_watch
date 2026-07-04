#include "display.h"

#include "NotoHebrew16.h"
#include "battery.h"
#include "config.h"
#include "globals.h"
#include "textutils.h"

String stripNonAscii(String input) {
    String result = "";
    for (int i = 0; i < input.length(); i++) {
        if ((uint8_t)input[i] < 128) {
            result += input[i];
        }
    }
    return result;
}

void drawTimeScreen(DateTime& now) {
    sprite.fillScreen(TFT_BLACK);

    char timeBuf[6];
    sprintf(timeBuf, "%02d:%02d", now.hour(), now.minute());
    sprite.setTextSize(4);
    sprite.setTextColor(TFT_WHITE);
    int tw = sprite.textWidth(timeBuf);
    sprite.setCursor((240 - tw) / 2, 80);
    sprite.print(timeBuf);

    char secBuf[3];
    sprintf(secBuf, "%02d", now.second());
    sprite.setTextSize(2);
    sprite.setTextColor(0x7BEF);
    int sw = sprite.textWidth(secBuf);
    sprite.setCursor((240 - sw) / 2, 135);
    sprite.print(secBuf);

    sprite.setTextSize(1);
    sprite.setTextColor(bleConnected ? TFT_GREEN : 0x4208);
    sprite.setCursor(10, 10);
    sprite.print(bleConnected ? "BLE" : "...");

    int battery = getBatteryPercent();
    char batBuf[8];
    sprintf(batBuf, "BAT %d%%", battery);
    sprite.setTextSize(1);
    sprite.setTextColor(battery > 20 ? TFT_GREEN : TFT_RED);
    int bw = sprite.textWidth(batBuf);
    sprite.setCursor((240 - bw) / 2, 185);
    sprite.print(batBuf);

    sprite.pushSprite(0, 0);
}

void drawDateScreen(DateTime& now) {
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    sprite.fillScreen(TFT_BLACK);

    sprite.setTextSize(3);
    sprite.setTextColor(TFT_WHITE);
    const char* day = days[now.dayOfTheWeek()];
    int dw = sprite.textWidth(day);
    sprite.setCursor((240 - dw) / 2, 80);
    sprite.print(day);

    char dmBuf[8];
    sprintf(dmBuf, "%02d %s", now.day(), months[now.month() - 1]);
    sprite.setTextSize(2);
    sprite.setTextColor(0x7BEF);
    int mw = sprite.textWidth(dmBuf);
    sprite.setCursor((240 - mw) / 2, 125);
    sprite.print(dmBuf);

    char yearBuf[5];
    sprintf(yearBuf, "%04d", now.year());
    sprite.setTextSize(2);
    sprite.setTextColor(TFT_WHITE);
    int yw = sprite.textWidth(yearBuf);
    sprite.setCursor((240 - yw) / 2, 155);
    sprite.print(yearBuf);

    sprite.pushSprite(0, 0);
}

void drawNotifyScreen() {
    sprite.fillScreen(TFT_BLACK);

    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    int sw = sprite.textWidth(notifySrc.c_str());
    sprite.setCursor((240 - sw) / 2, 55);
    sprite.print(notifySrc);

    sprite.drawFastHLine(40, 70, 160, 0x4208);

    sprite.setTextSize(2);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextWrap(true);
    sprite.setCursor(20, 85);
    sprite.print(notifyTitle);

    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    sprite.setCursor(20, 130);
    sprite.print(notifyBody.substring(0, 120));

    sprite.setTextSize(1);
    sprite.setTextColor(0xBDF7);
    String hint = "tap to dismiss";
    int hw = sprite.textWidth(hint.c_str());
    sprite.setCursor((240 - hw) / 2, 200);
    sprite.print(hint);

    sprite.pushSprite(0, 0);
}

String truncate(String s, int maxLen) {
    if (s.length() <= (unsigned)maxLen) return s;
    return s.substring(0, maxLen - 3) + "...";
}
void drawNotifyListScreen() {
    sprite.fillScreen(TFT_BLACK);

    int total = notifQueue.size();

    if (total == 0) {
        sprite.setTextSize(1);
        sprite.setTextColor(0xBDF7);
        String msg = "all caught up";
        int w = sprite.textWidth(msg.c_str());
        sprite.setCursor((240 - w) / 2, 110);
        sprite.print(msg);
        sprite.pushSprite(0, 0);
        return;
    }

    // Header
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    String hdr = "notifications";
    int hw = sprite.textWidth(hdr.c_str());
    sprite.setCursor((240 - hw) / 2, 14);
    sprite.print(hdr);
    sprite.drawFastHLine(20, 22, 200, 0x7BEF);

    int listStartY = 28;

    for (int i = 0; i < total; i++) {
        int cardY = listStartY + i * (CARD_HEIGHT + CARD_GAP) - (int)scrollY;

        if (cardY + CARD_HEIGHT < listStartY) continue;
        if (cardY > 235) break;

        WatchNotification& n = notifQueue[i];

        sprite.fillRoundRect(20, cardY, 200, CARD_HEIGHT, 6, 0x1082);
        sprite.drawRoundRect(20, cardY, 200, CARD_HEIGHT, 6, 0x7BEF);

        if (cardY + 8 >= listStartY && cardY + 8 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(TFT_WHITE);
            sprite.setCursor(28, cardY + 8);
            sprite.print(truncate(n.src, 20));
        }

        if (cardY + 22 >= listStartY && cardY + 22 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(TFT_WHITE);
            sprite.setCursor(28, cardY + 22);
            drawBiDiText(28, cardY + 22, truncate(n.title, 26), TFT_WHITE);
        }

        if (cardY + 36 >= listStartY && cardY + 36 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(0xBDF7);
            sprite.setCursor(28, cardY + 36);
            drawBiDiText(28, cardY + 36, truncate(n.body, 28), 0xBDF7);
        }

        if (cardY + 48 >= listStartY && cardY + 48 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(0x7BEF);
            sprite.setCursor(28, cardY + 48);
            sprite.print("tap to expand");
        }
    }

    // Redraw header on top
    sprite.fillRect(0, 0, 240, 26, TFT_BLACK);
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    sprite.setCursor((240 - hw) / 2, 14);
    sprite.print(hdr);
    sprite.drawFastHLine(20, 22, 200, 0x7BEF);

    sprite.pushSprite(0, 0);
}

void drawNotifyExpandedScreen() {
    sprite.fillScreen(TFT_BLACK);

    if (expandedIndex >= (int)notifQueue.size()) return;
    WatchNotification& n = notifQueue[expandedIndex];

    // Header
    sprite.setTextSize(1);
    sprite.setTextColor(0x7BEF);
    sprite.setCursor(10, 14);
    sprite.print("<");
    sprite.setTextColor(TFT_WHITE);
    String src = truncate(n.src, 22);
    int sw = sprite.textWidth(src.c_str());
    sprite.setCursor((240 - sw) / 2, 14);
    sprite.print(src);
    sprite.drawFastHLine(20, 22, 200, 0x7BEF);

    // Card
    sprite.fillRoundRect(20, 28, 200, 168, 6, 0x1082);
    sprite.drawRoundRect(20, 28, 200, 168, 6, 0x7BEF);

    // Title
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextWrap(true);
    sprite.setCursor(28, 38);
    sprite.print(n.title);

    sprite.drawFastHLine(28, 52, 184, 0x7BEF);

    // Body with manual wrapping
    String body = n.body;
    int lineY = 60;
    auto cps = applyBiDi(body);
    String displayLine = "";
    int lineWidth = 0;

    for (auto cp : cps) {
        bool hebrew = isHebrew(cp);
        sprite.setFont(hebrew ? &NotoHebrew16pt : nullptr);
        int charW = sprite.fontWidth();

        if (lineWidth + charW > 184) {
            // Draw current line
            drawBiDiText(28, lineY, displayLine, 0xBDF7);
            displayLine = "";
            lineWidth = 0;
            lineY += 14;
            if (lineY >= 180) break;
        }

        // Append character
        if (cp < 0x80) {
            displayLine += (char)cp;
        } else {
            // Re-encode to UTF-8 for String append
            if (cp < 0x800) {
                displayLine += (char)(0xC0 | (cp >> 6));
                displayLine += (char)(0x80 | (cp & 0x3F));
            } else {
                displayLine += (char)(0xE0 | (cp >> 12));
                displayLine += (char)(0x80 | ((cp >> 6) & 0x3F));
                displayLine += (char)(0x80 | (cp & 0x3F));
            }
        }
        lineWidth += charW;
    }
    if (!displayLine.isEmpty() && lineY < 180) {
        drawBiDiText(28, lineY, displayLine, 0xBDF7);
    }
    sprite.setFont(nullptr);

    // Dismiss hint
    sprite.setTextSize(1);
    sprite.setTextColor(0x7BEF);
    String hint = "tap to dismiss";
    int dw = sprite.textWidth(hint.c_str());
    sprite.setCursor((240 - dw) / 2, 208);
    sprite.print(hint);

    sprite.pushSprite(0, 0);
}
int biDiTextWidth(const String& text) {
    auto cps = applyBiDi(text);
    int w = 0;
    for (auto cp : cps) {
        if (isHebrew(cp)) {
            sprite.setFont(&NotoHebrew16pt);
        } else {
            sprite.setFont(nullptr);
        }
        w += sprite.fontWidth();
    }
    sprite.setFont(nullptr);
    return w;
}

void drawBiDiText(int x, int y, const String &text, uint16_t color) {
    auto cps = applyBiDi(text);
    int curX = x;
    for (auto cp : cps) {
        if (isHebrew(cp)) {
            sprite.setFont(&NotoHebrew16pt);
        } else {
            sprite.setFont(nullptr);
        }
        sprite.setTextColor(color, TFT_BLACK);
        sprite.drawChar(cp, curX, y);
        curX += sprite.fontWidth();
    }
    sprite.setFont(nullptr);
}