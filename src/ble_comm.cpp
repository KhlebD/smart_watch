#include "ble_comm.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <NimBLEDevice.h>

#include "globals.h"
#include "notif_screen.h"

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) override {
        bleConnected = true;
        Serial.println("BLE connected");
    }

    void onDisconnect(NimBLEServer* pServer) override {
        bleConnected = false;
        Serial.println("BLE disconnected");
        NimBLEDevice::startAdvertising();
        Serial.println("BLE advertising restarted");
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        String value = pCharacteristic->getValue().c_str();
        if (value.length() == 0) return;

        while (value.length() > 0 && (uint8_t)value[0] < 32) {
            value = value.substring(1);
        }

        if (value.startsWith("GB(")) {
            bleBuffer = "";
        }

        bleBuffer += value;
        Serial.print("BLE buffer: ");
        Serial.println(bleBuffer);

        String toParse = bleBuffer;
        while (toParse.length() > 0 &&
               (uint8_t)toParse[toParse.length() - 1] < 32) {
            toParse = toParse.substring(0, toParse.length() - 1);
        }

        if (toParse.startsWith("GB(")) {
            if (!toParse.endsWith(")")) return;
            toParse = toParse.substring(3, toParse.length() - 1);
        } else {
            int open = 0, close = 0;
            for (char c : toParse) {
                if (c == '{') open++;
                if (c == '}') close++;
            }
            if (open == 0 || open != close) return;
        }

        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, toParse);
        Serial.print("Parsing: ");
        Serial.println(toParse);
        bleBuffer = "";

        if (err) {
            Serial.print("JSON parse error: ");
            Serial.println(err.c_str());
            return;
        }

        const char* type = doc["t"];
        if (!type) return;

        if (strcmp(type, "notify") == 0) {
            WatchNotification notif;
            notif.src = doc["src"] | "Unknown";
            notif.title = doc["title"] | "";
            notif.body = doc["body"] | "";
            notifQueue.push_back(notif);
            Serial.printf("Queued notification, total: %d\n",
                          notifQueue.size());
            pendingNotification = true;
        }

        if (strcmp(type, "notify-remove") == 0) {
            // Nothing to do — user dismisses manually on watch
        }

        if (strcmp(type, "setTime") == 0) {
            long unixTime = doc["time"];
            if (unixTime > 0) {
                struct timeval tv = {.tv_sec = (time_t)unixTime};
                settimeofday(&tv, nullptr);
                Serial.println("Internal RTC synced via BLE");
            }
        }
        if (strcmp(type, "musicInfo") == 0) {
            music_track = doc["track"] | "Unknown";
            music_artist = doc["artist"] | "";
            music_state = doc["state"] | "paused";
            music_pos = doc["pos"] | 0;
            music_dur = doc["dur"] | 0;
            music_vol = doc["vol"] | 70;
            music_pos_updated_at = millis();
            Serial.printf("Music: %s - %s (%s) %d/%ds vol=%d\n",
                          music_track.c_str(), music_artist.c_str(),
                          music_state.c_str(), music_pos, music_dur, music_vol);
        }
    }
};

void initBLE() {
    NimBLEDevice::init("Watch_GB");

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

    pTxCharacteristic =
        pService->createCharacteristic(BLE_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

    NimBLECharacteristic* pRxCharacteristic =
        pService->createCharacteristic(BLE_RX_UUID, NIMBLE_PROPERTY::WRITE);
    pRxCharacteristic->setCallbacks(new RxCallbacks());

    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    NimBLEDevice::startAdvertising();

    Serial.println("BLE advertising as Watch_GB");
}

void send_music_cmd(const char* cmd) {
    if (!pTxCharacteristic || !bleConnected) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "GB({\"t\":\"musicCmd\",\"cmd\":\"%s\"})", cmd);
    pTxCharacteristic->setValue((uint8_t*)buf, strlen(buf));
    pTxCharacteristic->notify();
    Serial.printf("Sent music cmd: %s\n", cmd);
}
