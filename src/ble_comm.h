#pragma once

// Nordic UART Service UUIDs (Gadgetbridge/Bangle.js protocol)
#define BLE_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

void initBLE();
void send_music_cmd(const char* cmd);