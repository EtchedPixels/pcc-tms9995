/*	$Id: macdefs.h,v 1.18 2021/10/08 15:59:07 ragge Exp $	*/
/*
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
#define	BOOL_TYPE	CHAR	/* what used to store _Bool */

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
 * The classes used on pdp11 are:
 *	A - 16-bit
 *	B - 32-bit (concatenated 16-bit)
 *	C - floating point
 *	D - 64-bit (on stack, emulated)
 *
 * To get started we are not using all the registers
 */
#define	R0	000	/* Scratch and secondary return register */
#define	R1	001	/* Scratch and return register */
#define	R2	002	/* Scratch register */
#define	R3	003	/* Scratch register */
#define	R4	004	/* Scratch register or register variable */
#define	R5	005	/* Scratch register or register variable */
#define	SP	006	/* Stack pointer */
#define R7	007	/* Frame pointer */

#define	R01	010
#define	R12	011
#define	R23	012
#define	R34	013

#define	FR0	020	/* Results accumulate here */
#define	FR1	021
#define	FR2	022

#define	LL0	030
#define	LL1	031
#define	LL2	032
#define	LL3	033

#define	MAXREGS	034	/* 28 registers */

#define	RSTATUS	\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|PERMREG, SAREG|PERMREG, 0, 0, \
	SBREG, SBREG, SBREG, SBREG, 0, 0, 0, 0,		\
	SCREG|TEMPREG, 0, 0, 0, 0, 0, 0, 0,		\
	SDREG, SDREG, SDREG, SDREG,

#define	ROVERLAP \
	/* 8 basic registers */\
	{ R01, FR0, -1 },	\
	{ R01, R12, FR0, -1 },	\
	{ R12, R23, FR1, -1 },	\
	{ R23, R34, FR1, -1 },	\
	{ R34, FR2,-1 },	\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
\
	/* 4 long registers */\
	{ R0, R1, R12, FR0, -1 },		\
	{ R1, R2, R01, R23, FR0, FR1, -1 },		\
	{ R2, R3, R12, R34, FR1, FR2, -1 },		\
	{ R3, R4, R23, FR2, -1 },		\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
	{ -1 },			\
\
	/* The fp register is R0/1 */\
	{ R0, R1, R01, R12, -1 },\
	{ R2, R3, R12, R23, -1 },\
	{ R4, R5, R34, -1 },\
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
#define	GCLASS(x) (x < 8 ? CLASSA : x < 16 ? CLASSB : x < 24 ? CLASSC : CLASSD)
#define DECRA(x,y)	(((x) >> (y*5)) & 31)	/* decode encoded regs */
#define	ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 5)	/* A1 */
#define ENCRA2(x)	((x) << 10)	/* A2 */
#define ENCRA(x,y)	((x) << (5+y*5))	/* encode regs in int */
#define	RETREG(x)	((x) == LONG || (x) == ULONG ? R01 : \
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

/* floating point definitions */
#define	FDFLOAT
#define	DEFAULT_FPI_DEFS { &fpi_ffloat, &fpi_ffloat, &fpi_ffloat }
