/*	$Id: macdefs.h,v 1.18 2021/10/08 15:59:07 ragge Exp $	*/
/*
 * Copyright (c) 2022 Alan Cox <etchedpixels@gmail.com>
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Machine-dependent defines for both passes.
 */

/*
 * Convert (multi-)character constant to integer.
 */
#define makecc(val,i)	lastcon = i ? (val<<8)|lastcon : val

#define ARGINIT		48	/* # bits above r7 where arguments start */
#define AUTOINIT	(-16)	/* # bits below r7 where automatics start */

/*
 * Storage space requirements
 */
#define SZCHAR		8
#define SZBOOL		8
#define SZINT		16
#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLDOUBLE	64
#define SZLONG		32
#define SZSHORT		16
#define SZLONGLONG	64
#define SZPOINT(t)	16

/*
 * Alignment constraints
 */
#define ALCHAR		8
#define ALBOOL		8
#define ALINT		16
#define ALFLOAT		16
#define ALDOUBLE	16
#define ALLDOUBLE	16
#define ALLONG		16
#define ALLONGLONG	16
#define ALSHORT		16
#define ALPOINT		16
#define ALSTRUCT	16
#define ALSTACK		16 

/*
 * Min/max values.
 */
#define	MIN_CHAR	-128
#define	MAX_CHAR	127
#define	MAX_UCHAR	255
#define	MIN_SHORT	-32768
#define	MAX_SHORT	32767
#define	MAX_USHORT	65535
#define	MIN_INT		(-0x7fff-1)
#define	MAX_INT		0x7fff
#define	MAX_UNSIGNED	0xffff
#define	MIN_LONG	(-0x7fffffff-1)
#define	MAX_LONG	0x7fffffff
#define	MAX_ULONG	0xffffffff
#define	MIN_LONGLONG	0x8000000000000000LL
#define	MAX_LONGLONG	0x7fffffffffffffffLL
#define	MAX_ULONGLONG	0xffffffffffffffffULL

/* Default char is unsigned */
#define	CHAR_UNSIGNED
#define	BOOL_TYPE	UNSIGNED	/* what used to store _Bool */

/*
 * Use large-enough types.
 */
typedef	long long CONSZ;
typedef	unsigned long long U_CONSZ;
typedef long long OFFSZ;

#define CONFMT	"%llo"		/* format for printing constants */
#define LABFMT	"L%d"		/* format for printing labels */
#ifdef LANG_F77
#define BLANKCOMMON "_BLNK_"
#define MSKIREG  (M(TYSHORT)|M(TYLONG))
#define TYIREG TYLONG
#define FSZLENG  FSZLONG
#define	AUTOREG	EBP
#define	ARGREG	EBP
#define ARGOFFSET 4
#endif

#define STACK_DOWN 		/* stack grows negatively for automatics */

#undef	FIELDOPS		/* no bit-field instructions */
#define TARGET_ENDIAN TARGET_BE /* big endian */
#define	MYINSTRING
#define	MYALIGN

/* Definitions mostly used in pass2 */

#define BYTEOFF(x)	((x)&01)
#define wdal(k)		(BYTEOFF(k)==0)

#define STOARG(p)
#define STOFARG(p)
#define STOSTARG(p)

#define	FINDMOPS	/* We have memory operations */
#define	MYDOTFILE
#define	printdotfile(x)

#define szty(t) ((t) == DOUBLE || (t) == LONGLONG || (t) == ULONGLONG ? 4 : \
	(t) == FLOAT || (t) == LONG || (t) == ULONG ? 2 : 1)

/*
 * The TMS9995 has 4 register classes, 16-bit, 32-bit, floats and 64-bit.
 * Class membership and overlaps are defined in the macros RSTATUS
 * and ROVERLAP below.
 *
 * The classes used the TMS9995 are:
 *	A - 16-bit
 *	B - 32-bit (concatenated 16-bit)
 *	C - floating point
 *	D - 64-bit (on stack, emulated, not yet)
 *
 */
#define	R0	0x00	/* Scratch and secondary return register */
#define	R1	0x01	/* Scratch and return register */
#define	R2	0x02	/* Scratch register */
#define	R3	0x03	/* Scratch register */
#define	R4	0x02	/* Scratch register */
#define	R5	0x03	/* Scratch register */
#define	SP	0x06	/* Stack pointer */
#define R7	0x07	/* Frame pointer */
#define	R8	0x08	/* Scratch register or register variable */
#define	R9	0x09	/* Scratch register or register variable */
#define	R10	0x0A	/* Scratch register or register variable */
#define	R11	0x0B	/* Link register */
#define R12	0x0C	/* CRU offset */
#define R13	0x0D	/* Unused */
#define R14	0x0E	/* Relocatable code pointer */
#define R15	0x0F	/* PIC data pointer */

#define	RP01	0x10	/* Pairs. We don't seem to gain much from */
#define	RP12	0x11	/* Register longs or many pairs due to all the helpers */
#define	RP23	0x12
#define	RP34	0x13

#define	FR0	0x20	/* Results accumulate here */
#define	FR1	0x21	/* Temporary to see if fake FR fixes stuff */

#define	LL0	0x30
#define	LL1	0x31
#define	LL2	0x32
#define	LL3	0x33

#define	MAXREGS	0x34	/* 52 registers */

#define	RSTATUS	\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, 0, 0, \
		SAREG|PERMREG, SAREG|PERMREG, SAREG|PERMREG, 0, 0, 0, 0, 0, \
	SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG, SBREG|TEMPREG, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0, 0, 0, \
	SCREG|TEMPREG, SCREG|TEMPREG, 0, 0, 0, 0, 0, 0,		\
		0, 0, 0, 0, 0, 0, 0, 0, \
	SDREG, SDREG, SDREG, SDREG,

#define	ROVERLAP \
	/* 16 basic registers */\
	{ RP01, FR0, -1 },	\
	{ RP01, RP12, FR0, -1 },	\
	{ RP12, RP23, -1 },	\
	{ RP23, RP34, -1 },	\
	{ RP34, -1 },		\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
\
	/* 4 long registers made using pairs */\
	{ R0, R1, RP12, FR0, -1 },		\
	{ R1, R2, RP01, RP23, FR0, -1 },		\
	{ R2, R3, RP12, RP34, -1 },		\
	{ R3, R4, RP23, -1 },		\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
\
	/* The fp register is R0/1 */\
	{ R0, R1, RP01, RP12, -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 },\
\
	/* Neither the four emulated long long regs */\
	{ -1 },\
	{ -1 },\
	{ -1 },\
	{ -1 }


/* Return a register class based on the type of the node */

#define PCLASS(p) (p->n_type < LONG || p->n_type > BTMASK ? SAREG : \
		  (p->n_type == LONG || p->n_type == ULONG ? SBREG : SCREG))

#define	NUMCLASS 	3	/* highest number of reg classes used */

int COLORMAP(int c, int *r);
#define	GCLASS(x) (x < 16 ? CLASSA : x < 32 ? CLASSB : x < 48 ? CLASSC : CLASSD)
#define DECRA(x,y)	(((x) >> (y*5)) & 31)	/* decode encoded regs */
#define	ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 5)	/* A1 */
#define ENCRA2(x)	((x) << 10)	/* A2 */
#define ENCRA(x,y)	((x) << (5+y*5))	/* encode regs in int */
#define	RETREG(x)	((x) == LONG || (x) == ULONG ? RP01 : \
	(x) == FLOAT || (x) == DOUBLE ? FR0 : \
	(x) == LONGLONG || (x) == ULONGLONG ? LL0: R1)

//#define R2REGS	1	/* permit double indexing */

/* XXX - to die */
#define FPREG	R7	/* frame pointer */
#define STKREG	SP	/* stack pointer */

/* A bunch of specials to make life easier for tms9995
   FIXME: Tidy up */
#define	STWO		(MAXSPECIAL+1)	/* exactly two */
#define	SMTWO		(MAXSPECIAL+2)	/* exactly minus two */
#define	SINCB		(MAXSPECIAL+3)	/* post-increment */
#define	SINCW		(MAXSPECIAL+4)	/* post-increment */
#if 0
#define	SARGSUB		(MAXSPECIAL+5)	/* arg pointer to array */
#define	SARGINC		(MAXSPECIAL+6)	/* post-increment arg */
#endif

#define	ATTR_P1_TARGET	ATTR_TMS9995_BEENHERE

/* floating point definitions */
#define	FDFLOAT
#define	DEFAULT_FPI_DEFS { &fpi_ffloat, &fpi_ffloat, &fpi_ffloat }
