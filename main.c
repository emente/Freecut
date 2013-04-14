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

#define B_CUT   46
#define B_UP    62
#define B_DOWN  78
#define B_LEFT  30
#define B_RIGHT 14
#define B_MM    12
#define B_CM    13
#define B_CENTER    60
#define B_ZERO      61

int speed=0,lastSpeed=0;
int press=0,lastPress=0;
int step=10;


void poll_keypad(void) {
    if (keypad_stop_pressed()) {
        nullMode(true);
        return;
    }
    
    int key = keypad_scan();
    if (key == -1) {
        return;
    }
    //printf("ok key %d pressed\n",key);

    switch (key) {
        case B_UP:
            gcode_move(step,0,false,true);
            break;
        case B_DOWN:
            gcode_move(-step,0,false,true);
            break;
        case B_LEFT:
            gcode_move(0,step,false,true);
            break;
        case B_RIGHT:
            gcode_move(0,-step,false,true);
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
        case B_MM:
            step=1;
            break;
        case B_CM:
            step=10;
            break;
        case B_CENTER:
            
            break;
        case B_ZERO:

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
    while (1) {
        wdt_reset();
        
        gcode_loop();

        if (flag_25Hz) {
            flag_25Hz = 0;
            poll_keypad( );
            dial_poll();

        }

        if (flag_Hz) {
            flag_Hz = 0;

            printf("start\n");


            speed = dial_get_speed();
            press = dial_get_pressure();
            if (speed != lastSpeed) {
                stepper_speed(speed);
                lastSpeed = speed;
                printf("speed %i\n",speed);
            }
            if (press != lastPress) {
                stepper_pressure(press);
                lastPress = press;
                printf("pressure %i\n",speed);
            }

        }
    }
}
        