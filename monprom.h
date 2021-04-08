
/*

Taken from MEMU - Andy's Memotech Emulator http://www.nyangau.org/memu/memu.htm
with small modifications.

monprom.h - Character generator PROM

*/

#ifndef MONPROM_H
#define	MONPROM_H

/*...sincludes:0:*/
#define byte    unsigned char

/*...vtypes\46\h:0:*/
/*...e*/

#define	GLYPH_WIDTH   8
#define	GLYPH_HEIGHT 10

extern byte mon_alpha_prom[0x100][GLYPH_HEIGHT];

#endif
