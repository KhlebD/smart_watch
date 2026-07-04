#pragma once
#include <Arduino.h>
#include <vector>

std::vector<uint32_t> decodeUTF8(const String &s) {
    std::vector<uint32_t> codepoints;
    const uint8_t* bytes = (const uint8_t*)s.c_str();
    int len = s.length();
    int i = 0;
    while (i < len) {
        uint32_t cp = 0;
        uint8_t b = bytes[i];
        if (b < 0x80) {
            cp = b;
            i += 1;
        } else if ((b & 0xE0) == 0xC0) {
            cp = (b & 0x1F) << 6;
            cp |= (bytes[i+1] & 0x3F);
            i += 2;
        } else if ((b & 0xF0) == 0xE0) {
            cp = (b & 0x0F) << 12;
            cp |= (bytes[i+1] & 0x3F) << 6;
            cp |= (bytes[i+2] & 0x3F);
            i += 3;
        } else {
            i += 1;
        }
        codepoints.push_back(cp);
    }
    return codepoints;
}

bool isHebrew(uint32_t cp) {
    return cp >= 0x05D0 && cp <= 0x05EA;
}

bool containsHebrew(const String &s) {
    auto cps = decodeUTF8(s);
    for (auto cp : cps) {
        if (isHebrew(cp)) return true;
    }
    return false;
}

std::vector<uint32_t> applyBiDi(const String &s) {
    auto cps = decodeUTF8(s);
    if (cps.empty()) return cps;

    bool baseRTL = false;
    for (auto cp : cps) {
        if (isHebrew(cp)) { baseRTL = true; break; }
        if ((cp >= 'A' && cp <= 'Z') ||
            (cp >= 'a' && cp <= 'z')) { baseRTL = false; break; }
    }

    if (!baseRTL) return cps;

    struct Run {
        std::vector<uint32_t> chars;
        bool isRTL;
    };
    std::vector<Run> runs;
    Run current;
    current.isRTL = isHebrew(cps[0]);

    for (auto cp : cps) {
        bool hebrew = isHebrew(cp);
        if (cp == ' ' || hebrew == current.isRTL) {
            current.chars.push_back(cp);
        } else {
            runs.push_back(current);
            current.chars.clear();
            current.isRTL = hebrew;
            current.chars.push_back(cp);
        }
    }
    runs.push_back(current);

    std::reverse(runs.begin(), runs.end());

    std::vector<uint32_t> result;
    for (auto &run : runs) {
        if (run.isRTL) {
            std::reverse(run.chars.begin(), run.chars.end());
        }
        for (auto cp : run.chars) {
            result.push_back(cp);
        }
    }
    return result;
}