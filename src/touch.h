#pragma once

#define GESTURE_NONE  0
#define GESTURE_UP    1
#define GESTURE_DOWN  2
#define GESTURE_LEFT  3
#define GESTURE_RIGHT 4

struct TouchEvent {
    bool active;      // finger currently down
    int  x, y;       // current position
    int  startX, startY; // where finger landed
    int  prevY;       // previous Y for delta calculation
    bool wasDrag;     // did this touch move more than threshold
};

void resetTouch();
void configureTouch();
bool readTouch(int &x, int &y);
int  readGesture();
bool readTouchRaw(TouchEvent &evt);
void recoverI2C();