#pragma once

void motion_init();
bool motion_detected();
int  motion_get_steps();
bool motion_get_flag();
void motion_sleep_start();  // call before sleeping
void motion_sleep_end();    // call after waking