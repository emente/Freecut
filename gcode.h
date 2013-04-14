/*
mostly lifted from https://github.com/ErikZalm/Marlin


*/

#ifndef GCODE_H
#define GCODE_H

void gcode_loop(void);

void gcode_move(double x, double y, bool down, bool rel);
void nullMode(bool b);

void get_command(void);
void process_commands(void);

void FlushSerialRequestResend(void);
void ClearToSend(void);

void enquecommand(const char *cmd); //put an ascii command at the end of the current buffer.

#endif
