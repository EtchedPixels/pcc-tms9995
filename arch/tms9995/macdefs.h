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

#define ARGINIT		48	/* # bits above fp where arguments start */
#define AUTOINIT	(-16)	/* # bits below fp where automatics start */

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
#define	R6	0x06	/* Scratch register or register variable */
#define R7	0x07	/* Scratch register or register variable */
#define	R8	0x08	/* Scratch register or register variable */
#define	R9	0x09	/* Scratch register or register variable */
#define	R10	0x0A	/* Upper half of R910 */
#define	R11	0x0B	/* Link register */
#define R12	0x0C	/* Frame pointer */
#define R13	0x0D	/* Stack pointer */
#define R14	0x0E	/* Relocatable code pointer */
#define R15	0x0F	/* PIC data pointer */

#define	SP	R13

#define	RP01	0x10	/* Pairs. We don't seem to gain much from */
#define	RP12	0x11	/* Register longs or many pairs due to all the helpers */
#define	RP23	0x12
#define	RP34	0x13
#define RP45	0x14
#define RP56	0x15
#define RP67	0x16
#define RP78	0x17
#define RP89	0x18
#define RP910	0x19
#define RP1011	0x1A	/* Not available beyond this point */
#define RP1112	0x1B
#define RP1213	0x1C
#define RP1314	0x1D
#define RP1415	0x1E
#define RP150	0x1F

#define	FR0	0x20	/* Results accumulate here */
#define FR1	0x21	/* Fake FP register to keep code generator sane */
#define FR2	0x22	/* Ditto */

#define	LL0	0x30
#define	LL1	0x31
#define	LL2	0x32
#define	LL3	0x33

#define	MAXREGS	0x34	/* 52 registers */

#define	RSTATUS	\
	SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|TEMPREG, SAREG|PERMREG, SAREG|PERMREG, \
		SAREG|PERMREG, SAREG|PERMREG, 0, 0, 0, 0, 0, 0, \
	SBREG, SBREG, SBREG, SBREG, SBREG, SBREG, SBREG, SBREG,\
		SBREG, SBREG, 0, 0, 0, 0, 0, 0, \
	SCREG|TEMPREG, SCREG|TEMPREG, SCREG|TEMPREG, 0, 0, 0, 0, 0, 0,		\
		0, 0, 0, 0, 0, 0, 0, 0, \
	SDREG, SDREG, SDREG, SDREG,

#define	ROVERLAP \
	/* 16 basic registers */\
	{ /* R0: */ RP01, RP150, FR0, -1 },		\
	{ /* R1: */ RP01, RP12, FR0, -1 },		\
	{ /* R2: */ RP12, RP23, -1 },			\
	{ /* R3: */ RP23, RP34, -1 },			\
	{ /* R4: */ RP34, RP45, -1 },			\
	{ /* R5: */ RP45, RP56, -1 },			\
	{ /* R6: */ RP56, RP67, -1 },			\
	{ /* R7: */ RP67, RP78, -1 },			\
	{ /* R8: */ RP78, RP89, -1 },			\
	{ /* R9: */ RP89, RP910, -1 },			\
	{ /* R10: */ RP910, RP1011, -1 },		\
	{ /* R11: */ RP1011, RP1112, -1 },		\
	{ /* R12: */ RP1112, RP1213, -1 },		\
	{ /* R13: */ RP1213, RP1314, -1 },		\
	{ /* R14: */ RP1314, RP1415, -1 },		\
	{ /* R15: */ RP1415, RP150, -1 },		\
\
	/* 4 long registers made using pairs */\
	{ /* RP01: */ R0, R1, RP12, FR0, -1 },		\
	{ /* RP12: */ R1, R2, RP01, RP23, FR0, -1 },	\
	{ /* RP23: */ R2, R3, RP12, RP34, -1 },		\
	{ /* RP34: */ R3, R4, RP23, RP45,  -1 },	\
	{ /* RP45: */ R4, R5, RP34, -1 },		\
	{ /* RP56: */ R5, R6, RP45, RP67, -1 },		\
	{ /* RP67: */ R6, R7, RP56, RP78, -1 },		\
	{ /* RP78: */ R7, R8, RP67, RP89, -1 },		\
	{ /* RP89: */ R8, R9, RP78, RP910, -1 },	\
	{ /* RP910: */ R9, R10, RP89, RP1011, -1 },	\
	{ /* RP1011: */ R10, R11, RP910, RP1112, -1 },	\
	{ /* RP1112: */ R11, R12, RP1011, RP1213, -1 },	\
	{ /* RP1213: */ R12, R13, RP1112, RP1314, -1 },	\
	{ /* RP1314: */ R13, R14, RP1213, RP1415, -1 },	\
	{ /* RP1415: */ R14, R15, RP1314, RP150, -1 },	\
	{ /* RP150: */  R15, R0, RP1415, RP01, FR0, -1 },\
\
	/* The fp register is R0/1 */\
	{ /* FR0 */ R0, R1, RP01, RP12, RP150, -1 },\
	{ /* FR1 */ -1 },\
	{ /* FR2 */ -1 },\
	{ /* FR3 */ -1 },\
	{ /* FR4 */ -1 },\
	{ /* FR5 */ -1 },\
	{ /* FR6 */ -1 },\
	{ /* FR7 */ -1 },\
	{ /* FR8 */ -1 },\
	{ /* FR9 */ -1 },\
	{ /* FR10 */ -1 },\
	{ /* FR11 */ -1 },\
	{ /* FR12 */ -1 },\
	{ /* FR13 */ -1 },\
	{ /* FR14 */ -1 },\
	{ /* FR15 */ -1 },\
\
	/* Four emulated long long regs */\
	{ /* LL0 */ -1 },\
	{ /* LL1 */ -1 },\
	{ /* LL2 */ -1 },\
	{ /* LL3 */ -1 }


/* Return a register class based on the type of the node */

#define PCLASS(p) (p->n_type < LONG || p->n_type > BTMASK ? SAREG : \
		  (p->n_type == LONG || p->n_type == ULONG ? SBREG : SCREG))

#define	NUMCLASS 	3	/* highest number of reg classes used */

int COLORMAP(int c, int *r);
#define	GCLASS(x) (x < 16 ? CLASSA : x < 32 ? CLASSB : x < 48 ? CLASSC : CLASSD)
#define DECRA(x,y)	(((x) >> (y*6)) & 63)	/* decode encoded regs */
#define	ENCRD(x)	(x)		/* Encode dest reg in n_reg */
#define ENCRA1(x)	((x) << 6)	/* A1 */
#define ENCRA2(x)	((x) << 12)	/* A2 */
#define ENCRA(x,y)	((x) << (6+y*6))	/* encode regs in int */
#define	RETREG(x)	((x) == LONG || (x) == ULONG ? RP01 : \
	(x) == FLOAT || (x) == DOUBLE ? FR0 : \
	(x) == LONGLONG || (x) == ULONGLONG ? LL0: R1)

//#define R2REGS	1	/* permit double indexing */

/* XXX - to die */
#define FPREG	R12	/* frame pointer */
#define STKREG	SP	/* stack pointer */

/* A bunch of specials to make life easier for tms9995 */
#define	STWO		(MAXSPECIAL+1)	/* exactly two */
#define	SMTWO		(MAXSPECIAL+2)	/* exactly minus two */

/* Not used yet FIXME: Tidy up */
#define	SINCB		(MAXSPECIAL+4)	/* post-increment */
#define	SINCW		(MAXSPECIAL+5)	/* post-increment */
#if 0
#define	SARGSUB		(MAXSPECIAL+5)	/* arg pointer to array */
#define	SARGINC		(MAXSPECIAL+6)	/* post-increment arg */
#endif

#define	ATTR_P1_TARGET	ATTR_TMS9995_BEENHERE

/* floating point definitions */
#define	FDFLOAT
#define	DEFAULT_FPI_DEFS { &fpi_ffloat, &fpi_ffloat, &fpi_ffloat }

/* vararg tracking */
#define TARGET_IPP_MEMBERS			\
	int ipp_va;
