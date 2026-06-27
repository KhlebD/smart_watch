#include "display.h"
#include "globals.h"
#include "battery.h"
#include "battery.h"
#include "config.h"

String stripNonAscii(String input) {
    String result = "";
    for (int i = 0; i < input.length(); i++) {
        if ((uint8_t)input[i] < 128) {
            result += input[i];
        }
    }
    return result;
}

void drawTimeScreen(DateTime &now) {
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

void drawDateScreen(DateTime &now) {
    const char* days[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                             "Jul","Aug","Sep","Oct","Nov","Dec"};

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
    sprite.setTextColor(0x7BEF);
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
    sprite.setTextColor(0x7BEF);
    sprite.setCursor(20, 130);
    sprite.print(notifyBody.substring(0, 120));

    sprite.setTextSize(1);
    sprite.setTextColor(0x4208);
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
        sprite.setTextColor(0x4208);
        String msg = "all caught up";
        int w = sprite.textWidth(msg.c_str());
        sprite.setCursor((240 - w) / 2, 110);
        sprite.print(msg);
        sprite.pushSprite(0, 0);
        return;
    }

    // Header
    sprite.setTextSize(1);
    sprite.setTextColor(0x7BEF);
    String hdr = "notifications";
    int hw = sprite.textWidth(hdr.c_str());
    sprite.setCursor((240 - hw) / 2, 14);
    sprite.print(hdr);
    sprite.drawFastHLine(10, 22, 220, 0x2104);

    int listStartY = 28;

    // Draw all cards offset by scrollY
    for (int i = 0; i < total; i++) {
        int cardY = listStartY + i * (CARD_HEIGHT + CARD_GAP) - (int)scrollY;

        // Skip if entirely off screen
        if (cardY + CARD_HEIGHT < listStartY) continue;
        if (cardY > 235)                       break;

        WatchNotification &n = notifQueue[i];

        // Card background
        sprite.fillRoundRect(10, cardY, 220, CARD_HEIGHT, 6, 0x1082);
        sprite.drawRoundRect(10, cardY, 220, CARD_HEIGHT, 6, 0x2104);

        // App name — only draw if within visible area
        if (cardY + 8 >= listStartY && cardY + 8 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(0x7BEF);
            sprite.setCursor(18, cardY + 8);
            sprite.print(truncate(n.src, 20));
        }

        if (cardY + 22 >= listStartY && cardY + 22 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(TFT_WHITE);
            sprite.setCursor(18, cardY + 22);
            sprite.print(truncate(n.title, 28));
        }

        if (cardY + 36 >= listStartY && cardY + 36 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(0x4208);
            sprite.setCursor(18, cardY + 36);
            sprite.print(truncate(n.body, 30));
        }

        if (cardY + 48 >= listStartY && cardY + 48 < 235) {
            sprite.setTextSize(1);
            sprite.setTextColor(0x2104);
            sprite.setCursor(18, cardY + 48);
            sprite.print("tap to expand");
        }
    }

    // Redraw header on top to cover any cards that scrolled into it
    sprite.fillRect(0, 0, 240, 26, TFT_BLACK);
    sprite.setTextSize(1);
    sprite.setTextColor(0x7BEF);
    sprite.setCursor((240 - hw) / 2, 14);
    sprite.print(hdr);
    sprite.drawFastHLine(10, 22, 220, 0x2104);

    sprite.pushSprite(0, 0);
}

void drawNotifyExpandedScreen() {
    sprite.fillScreen(TFT_BLACK);

    if (expandedIndex >= (int)notifQueue.size()) return;
    WatchNotification &n = notifQueue[expandedIndex];

    // Back indicator + app name
    sprite.setTextSize(1);
    sprite.setTextColor(0x4208);
    sprite.setCursor(10, 14);
    sprite.print("<");
    sprite.setTextColor(0x7BEF);
    String src = truncate(n.src, 22);
    int sw = sprite.textWidth(src.c_str());
    sprite.setCursor((240 - sw) / 2, 14);
    sprite.print(src);
    sprite.drawFastHLine(10, 22, 220, 0x2104);

    // Card
    sprite.fillRoundRect(10, 28, 220, 168, 6, 0x1082);
    sprite.drawRoundRect(10, 28, 220, 168, 6, 0x2104);

    // Title
    sprite.setTextSize(1);
    sprite.setTextColor(TFT_WHITE);
    sprite.setTextWrap(true);
    sprite.setCursor(18, 38);
    sprite.print(n.title);

    sprite.drawFastHLine(18, 52, 204, 0x2104);

    // Body — full text with wrapping
    sprite.setTextColor(0x7BEF);
    sprite.setCursor(18, 60);
    // Manual wrap at ~30 chars per line
    String body = n.body;
    int lineY = 60;
    while (body.length() > 0 && lineY < 180) {
        String line = body.substring(0, min((int)body.length(), 30));
        int lastSpace = line.lastIndexOf(' ');
        if (lastSpace > 0 && (int)body.length() > 30) {
            line = body.substring(0, lastSpace);
            body = body.substring(lastSpace + 1);
        } else {
            body = body.substring(line.length());
        }
        sprite.setCursor(18, lineY);
        sprite.print(line);
        lineY += 12;
    }

    // Dismiss hint
    sprite.setTextSize(1);
    sprite.setTextColor(0x2104);
    int dw = sprite.textWidth("tap to dismiss");
    sprite.setCursor((240 - dw) / 2, 208);
    sprite.print("tap to dismiss");

    sprite.pushSprite(0, 0);
}