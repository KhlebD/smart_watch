#include "ble_comm.h"

#include <ArduinoJson.h>
#include <NimBLEDevice.h>

#include "config.h"
#include "display.h"
#include "globals.h"
#include "notif_screen.h"

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        bleConnected = true;
        Serial.println("BLE connected");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo,
                      int reason) override {
        bleConnected = false;
        Serial.println("BLE disconnected, reason:");
        Serial.println(reason);
        NimBLEDevice::startAdvertising();
        Serial.println("BLE advertising restarted");
    }
};
class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic,
                 NimBLEConnInfo& connInfo) override {
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
            // Nothing to do for now — user dismisses manually on watch
        }

        if (strcmp(type, "setTime") == 0) {
            long unixTime = doc["time"];
            if (unixTime > 0) {
                rtc.adjust(DateTime(unixTime));
                Serial.println("RTC synced via BLE");
            }
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