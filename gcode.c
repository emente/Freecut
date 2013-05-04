#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <inttypes.h>
#include <stdio.h>
#include "usb.h"
#include "timer.h"
#include "stepper.h"
#include "string.h"
#include "gcode.h"
#include "keypad.h"



#define MAX_CMD_SIZE 96
#define BUFSIZE 4
#define DPI_X   401/2.54/10
#define DPI_Y   401/2.54/10


char cmdbuffer[BUFSIZE][MAX_CMD_SIZE];
int bufindr = 0;
int bufindw = 0;
int buflen = 0;
char serial_char;
int serial_count = 0;
bool comment_mode = false;
char *strchr_pointer;
long gcode_N, gcode_LastN;

bool relative_mode = false;
bool penup = true;
bool null_mode = false;
double xpos = 0;
double ypos = 0;
double xoff = 0;
double yoff = 0;

void gcode_loop(void) {
    if (buflen < (BUFSIZE - 1))
        get_command();

    if (buflen) {
        process_commands();
        buflen = (buflen - 1);
        bufindr = (bufindr + 1) % BUFSIZE;
    }
}

void enquecommand(const char *cmd) {
    if (buflen < BUFSIZE) {
        //this is dangerous if a mixing of serial and this happsens
        strcpy(&(cmdbuffer[bufindw][0]), cmd);
        bufindw = (bufindw + 1) % BUFSIZE;
        buflen += 1;
    }
}

void get_command() {
    while (1) {
        int foo = usb_peek();
        if (foo < 0 || buflen >= BUFSIZE) {
            break;
        }
        serial_char = foo;

        if (serial_char == '\n' ||
                serial_char == '\r' ||
                (serial_char == ':' && comment_mode == false) ||
                serial_count >= (MAX_CMD_SIZE - 1)) {
            if (!serial_count) { //if empty line
                comment_mode = false; //for new command
                return;
            }
            cmdbuffer[bufindw][serial_count] = 0; //terminate string
            if (!comment_mode) {
                comment_mode = false; //for new command





                if (strstr(cmdbuffer[bufindw], "N") != NULL) {
                    strchr_pointer = strchr(cmdbuffer[bufindw], 'N');
                    gcode_N = (strtol(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL, 10));
                    if (gcode_N != gcode_LastN + 1 && (strstr(cmdbuffer[bufindw], "M110") == NULL)) {
                        printf("Error: Line Number is not Last Line Number+1, Last Line:%lu\n", gcode_LastN);
                        //Serial.println(gcode_N);
                        FlushSerialRequestResend();
                        serial_count = 0;
                        return;
                    }

                    if (strstr(cmdbuffer[bufindw], "*") != NULL) {
                        unsigned char checksum = 0;
                        unsigned char count = 0;
                        while (cmdbuffer[bufindw][count] != '*') checksum = checksum^cmdbuffer[bufindw][count++];
                        strchr_pointer = strchr(cmdbuffer[bufindw], '*');

                        if ((int) (strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)) != checksum) {
                            printf("Error:checksum mismatch, Last Line:%lu\n", gcode_LastN);
                            FlushSerialRequestResend();
                            serial_count = 0;
                            return;
                            //if no errors, continue parsing
                        }
                    } else {
                        printf("Error:No Checksum with line number, Last Line:%lu\n", gcode_LastN);
                        FlushSerialRequestResend();
                        serial_count = 0;
                        return;
                    }

                    gcode_LastN = gcode_N;
                    //if no errors, continue parsing
                } else // if we don't receive 'N' but still see '*'
                {
                    if ((strstr(cmdbuffer[bufindw], "*") != NULL)) {
                        printf("Error:No Line Number with checksum, Last Line:%lu\n", gcode_LastN);
                        serial_count = 0;
                        return;
                    }
                }
                if ((strstr(cmdbuffer[bufindw], "G") != NULL)) {
                    strchr_pointer = strchr(cmdbuffer[bufindw], 'G');
                    switch ((int) ((strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)))) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            printf("ok\n");
                            break;
                        default:
                            break;
                    }

                }
                bufindw = (bufindw + 1) % BUFSIZE;
                buflen += 1;
            }
            serial_count = 0; //clear buffer
        } else {
            bool skipmode = false;
            if (serial_char == ';') {

                // IN;SP1;PU698,727;PD453,632;PD236,779;PD251,517;PD44,356;PD298,289;PD387,42;PD529,263;PD791,272;PD625,475;PD698,727;PU;

                if (serial_count == 2 &&
                        cmdbuffer[bufindw][0] == 'I' &&
                        cmdbuffer[bufindw][1] == 'N') {
                    skipmode = true;
                    printf("HPGL INIT\n");
                }
                else if (serial_count == 3 &&
                        cmdbuffer[bufindw][0] == 'S' &&
                        cmdbuffer[bufindw][1] == 'P' &&
                        cmdbuffer[bufindw][2] == '1') {
                    skipmode = true;
                    printf("HPGL PEN1\n");
                }
                else if (serial_count >= 2 &&
                        cmdbuffer[bufindw][0] == 'P' &&
                        (cmdbuffer[bufindw][1] == 'U' ||
                        cmdbuffer[bufindw][1] == 'D')) {
                    if (serial_count > 2) {
                        printf("plot\n");
                        cmdbuffer[bufindw][serial_count] = '\0';
                        strchr_pointer = strchr(cmdbuffer[bufindw], ',');
                        if (strchr_pointer != NULL) {
                            printf("coords\n");
                            int y = atoi(strchr_pointer + 1);
                            *strchr_pointer = '\0';
                            int x = atoi(&cmdbuffer[bufindw][2]);
                            printf("coords %d %d\n", x, y);
                            while (stepper_queued() > 0) {
                                wdt_reset();
                            }
                            if (cmdbuffer[bufindw][1] == 'U') {
                                stepper_move(x, y);
                            } else {
                                stepper_draw(x, y);
                            }
                            printf("done\n");
                        }
                    } else {
                        printf("no params\n");
                        if (cmdbuffer[bufindw][1] == 'U') {
                            while (stepper_queued() > 0) {
                                wdt_reset();
                            }
                            stepper_move(0, 0);
                            beeper_on(1360);
                            msleep(30);
                            beeper_off();
                        }
                    }
                    skipmode = true;
                } else {
                    comment_mode = true;
                }
            }

            if (skipmode) {
                serial_count = 0;
                skipmode = false;
            } else {
                if (!comment_mode) {
                    cmdbuffer[bufindw][serial_count++] = serial_char;
                }
            }
        }
    }

}

void ClearToSend(void) {
    printf("ok\n");
}

double code_value(void) {
    return (strtod(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1], NULL));
}

long code_value_long(void) {
    return (strtol(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1], NULL, 10));
}

/*
bool code_seen(char code_string[]) //Return True if the string was found
{
    return (strstr(cmdbuffer[bufindr], code_string) != NULL);
}
 */
bool code_seen(char code) {
    strchr_pointer = strchr(cmdbuffer[bufindr], code);
    return (strchr_pointer != NULL); //Return True if a character was found
}

void blink(void) {
    // keypad_set_leds(keypad_get_leds() ^ 1);
}

void gcode_move(double x, double y, bool down, bool rel) {
    if (null_mode) return;

    if (rel) {
        xpos += x;
        ypos += y;
        x = xpos;
        y = ypos;
    } else {
        xpos = x;
        ypos = y;
    }
    printf("move to %f,%f\n", x, y);

    x += xoff;
    y += yoff;

    x = x*DPI_X;
    y = y*DPI_Y;

    blink();
    if (!down) {
        stepper_move(x, y);
    } else {
        stepper_draw(x, y);
    }
}

void nullMode(bool b) {
    null_mode = b;
}

void FlushSerialRequestResend() {
    printf("Resend:%lu\n", gcode_LastN + 1);
    ClearToSend();
}

void process_commands() {
    unsigned long codenum; //throw away variable
    //char *starpos = NULL;

    if (code_seen('G')) {
        switch ((int) code_value()) {
            case 0: // G0 -> G1
            case 1: // G1
                if (code_seen('Z')) {
                    codenum = code_value();
                    penup = codenum > 0;
                }

                if (relative_mode) {
                    double newx = 0;
                    double newy = 0;
                    if (code_seen('X')) {
                        newx = code_value();
                    }
                    if (code_seen('Y')) {
                        newy = code_value();
                    }
                    gcode_move(newx, newy, !penup, true);
                } else {
                    double newx = xpos;
                    double newy = ypos;
                    if (code_seen('X')) {
                        newx = code_value();
                    }
                    if (code_seen('Y')) {
                        newy = code_value();
                    }
                    gcode_move(newx, newy, !penup, false);
                }
                break;

            case 4: // G4 dwell
                codenum = 0;
                if (code_seen('P')) codenum = code_value(); // milliseconds to wait
                if (code_seen('S')) codenum = code_value() * 1000; // seconds to wait
                codenum = codenum / 10;
                while (codenum-- > 0) {
                    wdt_reset();
                    msleep(10);
                }
                break;

            case 28: //G28 Home all Axis one at a time
                stepper_move(0, 0);
                xoff = 0;
                yoff = 0;
                break;

            case 90: // G90
                relative_mode = false;
                break;

            case 91: // G91
                relative_mode = true;
                break;

            case 92: // G92 set coords
                if (code_seen('X')) {
                    xoff = -xpos + code_value();
                    xpos = 0;
                } else {
                    xoff = -xpos;
                    xpos = 0;
                }
                if (code_seen('Y')) {
                    yoff = -ypos + code_value();
                    ypos = 0;
                } else {
                    yoff = -ypos;
                    ypos = 0;
                }
                break;
        }
    } else
        if (code_seen('M')) {
        switch ((int) code_value()) {
            case 80://power on
                break;

            case 106://load paper
                stepper_load_paper();
                break;

            case 107://unload paper
                stepper_unload_paper();
                break;

            case 300://S30=down S50=up S255=off
                if (code_seen('S')) {
                    penup = code_value() > 40;
                }
                break;
        }
    }//M
    else {
        printf("Error:unknown commando\n");
    }

}
