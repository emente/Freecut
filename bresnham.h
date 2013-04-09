/* Copyright (c) 1991 - 1994 Heinz W. Werntges.  All rights reserved. */

/** bresnham.h: Header for Bresenham utility
 **
 ** 91/01/04  V 1.00  HWW Originating
 ** 92/01/12  V 1.01  HWW ANSI prototypes required now
 **/


#define	BRESENHAM_EOL	0x04
#define	BRESENHAM_ERR	0xff



typedef struct {
	int	x,y;		/* 2d - pseudo device coord	*/
} DevPt;


DevPt	*bresenham_init	(DevPt *, DevPt *);
int	bresenham_next	(void);
