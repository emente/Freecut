/*
 * main.c
 * 
 * Freecut firmware, main program
 *
 *
 * Copyright 2010 <freecutfirmware@gmail.com> 
 *
 * This file is part of Freecut.
 *
 * Freecut is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2.
 *
 * Freecut is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Freecut. If not, see http://www.gnu.org/licenses/.
 *
 */

#define byte unsigned char

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include <stdio.h>
#include "usb.h"
#include "keypad.h"
#include "lcd.h"
#include "timer.h"
#include "stepper.h"
#include "flash.h"
#include "version.h"
#include "dial.h"
#include "string.h"
#include "gcode.h"


static FILE usb = FDEV_SETUP_STREAM(usb_putchar, usb_getchar, _FDEV_SETUP_RW);


#define LOAD_PAPER	0x4c
#define UNLOAD_PAPER	0x4d

#define B_CUT   0x2e
#define B_UP    0x3e
#define B_DOWN  0x4e
#define B_LEFT  0x1e
#define B_RIGHT 0x0e


void poll_keypad(void) {
    if (keypad_stop_pressed()) {
        nullMode(true);
        return;
    }
    
    int key = keypad_scan();

    switch (key) {
        case B_UP:
            moveHead(0,-10);
            break;
        case B_DOWN:
            moveHead(0,10);
            break;
        case B_LEFT:
            moveHead(-10,0);
            break;
        case B_RIGHT:
            moveHead(10,0);
            break;
        case B_CUT:
            nullMode(false);    
            break;
        case LOAD_PAPER:
            stepper_load_paper();
            break;
        case UNLOAD_PAPER:
            stepper_unload_paper();
            break;

        default:
            break;
    }
}


int main(void) {
    wdt_enable(WDTO_30MS);
    usb_init();
    timer_init();
    stepper_init();
    sei();
    keypad_init( );
    dial_init();

    wdt_reset();

    // short beep to show we're awake
    beeper_on(1760);
    msleep(10);
    beeper_off();

    // connect stdout to USB port
    stdout = &usb;
    printf("start\n");
    while (1) {
        wdt_reset();
        
        gcode_loop();

        if (flag_25Hz) {
            flag_25Hz = 0;
            poll_keypad( );
        }

        if (flag_Hz) {
            flag_Hz = 0;

            dial_poll();
            stepper_speed(dial_get_speed());
            stepper_pressure(dial_get_pressure());
        }
    }
}
        