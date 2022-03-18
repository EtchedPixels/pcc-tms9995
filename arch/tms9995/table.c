/*	$Id: table.c,v 1.17 2019/04/27 20:35:52 ragge Exp $	*/
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


# include "pass2.h"

# define TLL TLONGLONG|TULONGLONG
# define ANYSIGNED TINT|TLONG|TSHORT|TCHAR
# define ANYUSIGNED TUNSIGNED|TULONG|TUSHORT|TUCHAR
# define ANYFIXED ANYSIGNED|ANYUSIGNED
# define TUWORD TUNSIGNED
# define TSWORD TINT
# define TWORD TUWORD|TSWORD
# define ANYSH	SCON|SAREG|SOREG|SNAME
# define ARONS	SAREG|SOREG|SNAME

/*
 *	Be careful of the L/R orders. The TMS99xx instruction set is
 *	source,dest except for immediate operations which are reg, immediate
 *	(ie dest, source).
 */
struct optab table[] = {
/* First entry must be an empty entry */
{ -1, FOREFF, SANY, TANY, SANY, TANY, 0, 0, "", },

/* PCONVs are usually not necessary */
{ PCONV,	INAREG,
	SAREG,	TWORD|TPOINT,
	SAREG,	TWORD|TPOINT,
		0,	RLEFT,
		"", },

/* long -> ptr */
/* No work really needed */
{ PCONV,	INAREG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SAREG,			TPOINT,
		NAREG|NASL,	RESC1,
		"mov	UL,A1\n", },

/* convert uchar to char; sign-extend byte */
/* No work needed as the char is in the upper 8bits */
{ SCONV,	INAREG,
	SAREG,	TUCHAR,
	SAREG,	TCHAR,
		0,	RLEFT,
		"", },

/* convert char to uchar; zero-extend byte */
/* Again no work needed */
{ SCONV,	INAREG,
	SAREG,	TCHAR,
	SAREG,	TUCHAR,
		0,	RLEFT,
		"andi	AL,0xff\n", },

/* convert char to int or unsigned.  Already so in regs */
/* send it right */
{ SCONV,	INAREG,
	SAREG,	TCHAR,
	SAREG,	TINT,
		0,	RLEFT,
		"asr	AL,8\n", },

/* Same idea different shift - we can't use swpb as the other half may
   contain crud */
{ SCONV,	INAREG,
	SAREG,	TCHAR,
	SAREG,	TUNSIGNED,
		0,	RLEFT,
		"lsr	AL,8\n", },

/* char (in mem) to (u)int */
/* for now copy and mask. */
{ SCONV,	INAREG,
	SOREG|SNAME,		TCHAR,
	SAREG,			TINT,
		NAREG|NASL,	RESC1,
		"movb	AL,A1\nasr	A1,8\n", },

{ SCONV,	INAREG,
	SCON,			TCHAR,
	SAREG,			TINT,
		NAREG|NASL,	RESC1,
		"li	A1,AL * 256\n", },

/* char (in mem) to (u)int */
/* for now copy and mask. */
/* Needs a helper to generate the signed n << 8 for const */
{ SCONV,	INAREG,
	SCON,	TCHAR,
	SAREG,	TUNSIGNED,
		NAREG|NASL,	RESC1,
		"li	A1, AL\n", },

{ SCONV,	INAREG,
	SOREG|SNAME,	TCHAR,
	SAREG,	TUNSIGNED,
		NAREG|NASL,	RESC1,
		"movb	AL,A1\nlsr	A1,8\n", },

/* convert uchar to (u)int in reg */
/* send it down 8bits */
{ SCONV,	INAREG,
	SAREG,	TUCHAR,
	SAREG,	TINT|TUNSIGNED,
		0,	RLEFT,
		"lsr	AL,8", },

/* convert uchar to (u)int from mem */
/* clear the target, move the byte in and swap */
{ SCONV,	INAREG,
	SOREG|SNAME,	TUCHAR,
	SAREG,	TINT|TUNSIGNED,
		NAREG,	RESC1,
		"clr	A1\nmovb	AL,A1\nswpb	A1\n", },

/* convert char to (u)long */
/* Use a helper as we have no sign ext ops*/
{ SCONV,	INBREG,
	SAREG,	TCHAR,
	SANY,	TULONG|TLONG,
		NSPECIAL,	RLEFT,
		"bl	@s8_32\n", },

/* convert uchar to ulong */
/* Just clear the extra */
{ SCONV,	INBREG,
	SAREG,	TUCHAR,
	SANY,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"mov	AL,U1\nlsr	U1,8\nclr	A1\n", },

/* (u)char -> float/double */
/* Constant is forced into R0, masked and converted */
{ SCONV,	INCREG,
	SAREG,	TCHAR|TUCHAR,
	SANY,	TFLOAT|TDOUBLE,
		NSPECIAL,	RLEFT,
		"andi	AL,0xff\ncir	AL\n", },

/* convert (u)int to char */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TCHAR,
		NAREG|NASL,	RLEFT,
		"swpb	AL\n", },

/* convert (u)int to uchar  */
/* FIXME: need to look at this for reg cases versus memory */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TUCHAR,
		NAREG,	RESC1,
		"clr	A1\nmovb	AL,A1\nswpb	A1\n", },

/* convert (u)int to (u)int */
/* No work needed */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* FIXME: uchar->ptr flip... */

/* convert pointer to char */
/* flip bytes */
{ SCONV,	INAREG,
	SAREG,	TPOINT,
	SAREG,	TCHAR,
		0,	RLEFT,
		"swpb	AL\n", },

/* convert pointer to (u)int */
/* No work needed */
{ SCONV,	INAREG,
	SAREG,	TPOINT,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* convert int to long from memory */
{ SCONV,	INBREG,
	SAREG,	TINT,
	SANY,	TLONG,
		NSPECIAL,	RLEFT,
		"bl	@s16_32\n", },

/* int -> (u)long. XXX - only in r0 and r1 */
{ SCONV,	INBREG,
	SAREG,	TINT|TPOINT,
	SANY,	TLONG|TULONG,
		NSPECIAL|NBREG|NBSL,	RLEFT,
		"bl	@s8_32\n", },

/* int -> float/double */
{ SCONV,	INCREG,
	SAREG|SNAME|SOREG,	TINT,
	SANY,	TFLOAT|TDOUBLE,
		NSPECIAL,	RLEFT,
		"cir	AL\n", },


/* unsigned -> (u)long. XXX - only in r0 and r1 */
/* FIXME: we can do this in any reg pair with a Z helper ?? */
{ SCONV,	INBREG,
	SAREG,	TUNSIGNED,
	SANY,	TLONG|TULONG,
		NSPECIAL|NBREG|NBSL,	RLEFT,
		"clr	r0\n", },

/* uint -> double. We don't have this operation so use the 32bit one having
   cleared the other half */
{ SCONV,	INCREG,
	SAREG,		TUNSIGNED,
	SANY,		TFLOAT|TDOUBLE,
		NSPECIAL|NCSL,	RLEFT,
		"clr	r0\ncer AL\n", },

/* (u)long -> char */
/* Works for memory and register */
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SAREG,			TCHAR,
		NAREG|NASL,	RESC1,
		"mov	UL,A1\nswpb	A1\n", },

/* (u)long -> uchar */
/* Works for memory and register */
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SAREG,			TUCHAR,
		NAREG|NASL,	RESC1,
		"mov	UL,A1\nswpb	A1\n", },

/* long -> int */
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SAREG,			TWORD,
		NAREG|NASL,	RESC1,
		"mov	UL,A1\n", },

/* (u)long -> (u)long, nothing */
{ SCONV,	INBREG,
	SBREG,	TLONG|TULONG,
	SANY,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"", },

/* long -> float/double */
{ SCONV,	INCREG,
	SBREG|SNAME|SOREG|SCON,	TLONG,
	SANY,		TFLOAT|TDOUBLE,
		NCREG|NCSL,	RESC1,
		"cer AL\n", },

/* ulong -> float/double */
{ SCONV,	INCREG,
	SBREG,		TULONG,
	SANY,		TFLOAT|TDOUBLE,
		NSPECIAL|NCSL,	RLEFT,
		"bl	@u32fp\n", },

/* float/double -> (u)long */
{ SCONV,	INBREG,
	SCREG,	TFLOAT|TDOUBLE,
	SANY,	TLONG|TULONG,
		NBREG,	RESC1,
		"cre A1\n" },

/* double -> int*/
{ SCONV,	INAREG,
	SCREG,	TFLOAT|TDOUBLE,
	SANY,	TINT,
		NAREG,	RESC1,
		"cri	A1\n", },

/* float/double -> float/double */
/* In regs. floating point always stored as double in regs */
{ SCONV,	INCREG,
	SCREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT,
		"", },

{ SCONV,	INCREG,
	SCON|SOREG|SNAME,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		NCREG|NCSL,	RLEFT,
		"mov	AL,A1\nmov	UL,U1\n", },

/*
 * Subroutine calls.
 */
{ CALL,		INAREG,
	SCON,	TANY,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INAREG,
	SCON,	TANY,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"bl	AL\n", },

{ CALL,		INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	AL\nZC", },

{ UCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	AL\n", },

{ CALL,		INBREG,
	SCON,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INBREG,
	SCON,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	AL\n", },

{ CALL,		INBREG,
	SAREG,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INBREG,
	SAREG,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	AL\n", },

{ CALL,		INCREG,
	SCON,	TANY,
	SCREG,	TFLOAT|TDOUBLE,
		NCREG,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INCREG,
	SCON,	TANY,
	SCREG,	TFLOAT|TDOUBLE,
		NCREG,	RESC1,
		"bl	AL\n", },

{ CALL,		INCREG,
	SAREG,	TANY,
	SCREG,	TFLOAT|TDOUBLE,
		NCREG|NCSL,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INCREG,
	SAREG,	TANY,
	SCREG,	TFLOAT|TDOUBLE,
		NCREG|NCSL,	RESC1,
		"bl	AL\n", },

{ CALL,		FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ UCALL,	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\n", },

{ CALL,		FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ UCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\n", },

{ STCALL,	INAREG,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"bl	AL\nZC", },

{ USTCALL,	INAREG,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"bl	AL\n", },

{ STCALL,	FOREFF,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ USTCALL,	FOREFF,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\n", },

{ STCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	AL\nZC", },

{ USTCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	AL\n", },

/*
 * The next rules handle all binop-style operators.
 *
 * TODO: spot two for INCT/DECT
 */
#if 0
/* Post-increment read, byte */
{ PLUS,		INAREG,
	SINCB,	TCHAR|TUCHAR,
	SONE,	TANY,
		NAREG,	RESC1,
		"movb	*zg,A1\nincb	zg\n", },

/* Post-increment read, int */
{ PLUS,		INAREG,
	SINCB,	TWORD|TPOINT,
	SONE,	TANY,
		NAREG,	RESC1,
		"mov	ZG,A1\ninc	ZG\n", },
#endif		

/* Add one to anything left but use only for side effects */
{ PLUS,		FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"inc	AL\n", },

{ PLUS,		FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	STWO,			TANY,
		0,	RLEFT|RESCC,
		"inct	AL\n", },

/* add one for char to reg, special handling */
{ PLUS,		FOREFF|INAREG|FORCC,
	SAREG,	TCHAR|TUCHAR,
	SONE,		TANY,
		0,	RLEFT|RESCC,
		"ai	AL, 0x100\n", },

#if 0
/* add one for char to memory. No obvious way to do this - no incb
   only ab =@lit, */
{ PLUS,		FOREFF|FORCC,
	SNAME|SOREG,	TCHAR|TUCHAR,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"inc	AL\n", },
#endif		

/* No ADC */
{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SONE,			TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@inc32\n", },

{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@add32\n", },

{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,	RLEFT,
		/* Words reversed for speed */
		"bl	@add32i\n.word	AR\n.word	UR\n", },

/* Integer to pointer addition */
{ PLUS,		INAREG,
	SAREG,	TPOINT|TWORD,
	SAREG,	TINT|TUNSIGNED,
		0,	RLEFT,
		"a	AR,AL\n", },

/* Add to reg left and reclaim reg */
{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SCON,			TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"ai	AL,AR\n", },

/* Add to reg left and reclaim reg */
{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"a	AR,AL\n", },

/* Add to anything left but use only for side effects */
{ PLUS,		FOREFF|FORCC,
	SNAME|SOREG,	TWORD|TPOINT,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"a	AR,AL\n", },

{ PLUS,		FOREFF|FORCC,
	SNAME|SOREG,	TWORD|TPOINT,
	SCON,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"ai	AL,AR\n", },

{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG,			TCHAR|TUCHAR,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"ab	AR,AL\n", },

/* No aib so use a literal */
{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG,			TCHAR|TUCHAR,
	SCON,			TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"a	@__litb_AR,AL\n", },

/* floating point */
{ PLUS,		INCREG|FOREFF|FORCC,
	SCREG,			TFLOAT|TDOUBLE,
	/*SCREG|*/SNAME|SOREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
		"ar	AR\n", },


{ MINUS,		INBREG|FOREFF,
	SBREG,		TLONG|TULONG,
	SONE,		TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@dec32\n", },

{ MINUS,		INBREG|FOREFF,
	SBREG,		TLONG|TULONG,
	SCON,		TLONG|TULONG,
		NSPECIAL,	RLEFT,
		/* Words reversed for speed */
		"bl	@sub32i\n.word	AR\n.word	UR\n", },

{ MINUS,		INBREG|FOREFF,
	SBREG,		TLONG|TULONG,
	SBREG,		TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@sub32\n", },

/* Sub one from anything left */
{ MINUS,	FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"dec	AL\n", },

{ MINUS,	FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	STWO,			TANY,
		0,	RLEFT|RESCC,
		"dect	AL\n", },

{ MINUS,		INAREG|FOREFF,
	SAREG,			TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT,
		"s	AR,AL\n", },

/* Sub from anything left but use only for side effects */
{ MINUS,	FOREFF|INAREG|FORCC,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SAREG|SNAME|SOREG|SCON,	TWORD|TPOINT,
		0,	RLEFT|RESCC,
		"s	AR,AL\n", },

/* Sub one left but use only for side effects */
{ MINUS,	FOREFF|FORCC,
	SAREG,		TCHAR|TUCHAR,
	SONE,			TANY,
		0,	RLEFT|RESCC,
		"ai	AL, -0x100\n", },

/* Sub from anything left but use only for side effects. Need to review
   because of the SUB 0 funny */
{ MINUS,		FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"sb	AR,AL\n", },

{ MINUS,		FOREFF|FORCC,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,			TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"swpb	AL\nsi	AR,AL\nswpb	AL\n", },

/* floating point */
{ MINUS,	INCREG|FOREFF|FORCC,
	SCREG,			TFLOAT|TDOUBLE,
	SNAME|SOREG,	TFLOAT|TDOUBLE,
		0,	RLEFT|RESCC,
		"sr	AR\n", },

/*
 * The next rules handle all shift operators.
 *
 * r0 is the only permitted 'amount', and constants
 * must be 1-15. Need to check compiler never tries to output silly values
 *
 * TODO: for breg see if it's used enough to warrant ls32i1 etc
 */
 
{ LS,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SAREG,	TINT|TUNSIGNED,
		NSPECIAL,	RLEFT,
		"bl	@ls32\n", },


/* Shift of 8 or 16bit type by constant*/

/* Shift of a register by a constant, works for 8 and 16bit */
{ LS,	INAREG|FOREFF,
	SAREG,	TINT|TCHAR|TUNSIGNED|TUCHAR,
	SCON,	TWORD,
	0,	RLEFT,
		"asl	AL,AR\n", },
		
/* Shift of a register by a register. We must shift by R0, so we cannot
   keep the data in R0 */
{ LS,	INAREG|FOREFF,
	SAREG,	TINT|TCHAR|TUNSIGNED|TUCHAR,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"asl	AL,AR\n", },

/* Constant shift of a word in memory */
{ LS,	INAREG|FOREFF,
	SOREG|SNAME, TINT|TUNSIGNED,
	SCON,	TWORD,
	0,	RLEFT,
		"asl	AL,AR\n", },

/* Register shift of a word in memory */
{ LS,	INAREG|FOREFF,
	SOREG|SNAME,	TINT|TUNSIGNED,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"asl	AL,AR\n", },

/* Same again - right unsigned 8 or 16bit shifts */

/* Shift of a register by a constant, works for 8 and 16bit */
{ RS,	INAREG|FOREFF,
	SAREG,	TUNSIGNED|TUCHAR,
	SCON,	TWORD,
	0,	RLEFT,
		"lsr	AL,AR\n", },
		
/* Shift of a register by a register. We must shift by R0, so we cannot
   keep the data in R0 */
{ RS,	INAREG|FOREFF,
	SAREG,	TUNSIGNED|TUCHAR,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"lsr	AL,AR\n", },

/* Constant shift of a word in memory */
{ RS,	INAREG|FOREFF,
	SOREG|SNAME, TINT|TUNSIGNED,
	SCON,	TWORD,
	0,	RLEFT,
		"lsr	AL,AR\n", },

/* Register shift of a word in memory */
{ RS,	INAREG|FOREFF,
	SOREG|SNAME,	TUNSIGNED,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"lsr	AL,AR\n", },

/* And signed */

/* Shift of a register by a constant, works for 8 and 16bit */
{ RS,	INAREG|FOREFF,
	SAREG,	TINT|TCHAR,
	SCON,	TWORD,
	0,	RLEFT,
		"asr	AL,AR\n", },
		
/* Shift of a register by a register. We must shift by R0, so we cannot
   keep the data in R0 */
{ RS,	INAREG|FOREFF,
	SAREG,	TINT|TCHAR,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"asr	AL,AR\n", },

/* Constant shift of a word in memory */
{ RS,	INAREG|FOREFF,
	SOREG|SNAME, TINT,
	SCON,	TWORD,
	0,	RLEFT,
		"asr	AL,AR\n", },

/* Register shift of a word in memory */
{ RS,	INAREG|FOREFF,
	SOREG|SNAME,	TINT,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"asr	AL,AR\n", },

{ RS,	INBREG|FOREFF,
	SBREG,	TLONG,
	SAREG,	TINT|TUNSIGNED,
		NSPECIAL,	RLEFT,
		"bl	@rss32\n", },

{ RS,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SAREG,	TUNSIGNED,
		NSPECIAL,	RLEFT,
		"bl	@rsu32\n", },




/*
 * The next rules takes care of assignments. "=".
 */

/* First optimizations, in lack of weight it uses first found */
/* Start with class A registers */

/* Clear word at address */
{ ASSIGN,	FOREFF|FORCC,
	ARONS,	TWORD|TPOINT,
	SZERO,		TANY,
		0,	RESCC,
		"clr	AL\n", },

/* Clear word at reg */
{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TWORD|TPOINT,
	SZERO,		TANY,
		0,	RDEST,
		"clr	AL\n", },

/* Clear byte at address.  No reg here. */
{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TCHAR|TUCHAR,
	SZERO,		TANY,
		0,	RDEST,
		"clrb	AL\n", },

/* Clear byte in reg */
{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TCHAR|TUCHAR,
	SZERO,	TANY,
		0,	RDEST,
		"clrb	AL\n", },

/* The next is class B regs */

/* Clear long at address or in reg */
{ ASSIGN,	FOREFF|INBREG,
	SNAME|SOREG|SBREG,	TLONG|TULONG,
	SZERO,			TANY,
		0,	RDEST,
		"clr	AL\nclr	UL\n", },

/* Must have multiple rules for long otherwise regs may be trashed */
/* FIXME: use a Z helper for this to turn one or the other into CLR or
   into LI, MOV for eg ffffffff */
{ ASSIGN,	FOREFF|INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		0,	RDEST,
		"li	AL,AR\nli	UL,UR\n", },

{ ASSIGN,	FOREFF|INBREG,
	SBREG,			TLONG|TULONG,
	SNAME|SOREG,		TLONG|TULONG,
		0,	RDEST,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF|INBREG,
	SNAME|SOREG,	TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		0,	RDEST,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLONG|TULONG,
	SCON,		TLONG|TULONG,
		0,	0,
		"li	AR,AL\nli	UR,UL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLONG|TULONG,
	SNAME|SOREG,	TLONG|TULONG,
		0,	0,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SBREG,	TLONG|TULONG,
		0,	RDEST,
		"ZE\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,			TWORD|TPOINT,
	SCON,			TWORD|TPOINT,
		0,	RDEST|RESCC,
		"li	AL,AR\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,			TWORD|TPOINT,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
		0,	RDEST|RESCC,
		"mov	AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	ARONS,	TWORD|TPOINT,
	SAREG,	TWORD|TPOINT,
		0,	RDEST|RESCC,
		"mov	AR,AL\n", },

{ ASSIGN,	FOREFF|FORCC,
	SNAME|SOREG,		TWORD|TPOINT,
	SCON,			TWORD|TPOINT,
		0,	RESCC,
		"li	AL,AR\n", },

{ ASSIGN,	FOREFF|FORCC,
	SNAME|SOREG,		TWORD|TPOINT,
	SNAME|SOREG,		TWORD|TPOINT,
		0,	RESCC,
		"mov	AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,		TCHAR|TUCHAR,
	ARONS	,	TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"movb	AR,AL\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,		TCHAR|TUCHAR,
	SCON,		TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"li	AL*256, AR\n", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	ARONS,	TCHAR|TUCHAR,
	SAREG,	TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"movb	AR,AL\n", },

{ ASSIGN,	FOREFF|FORCC,
	SNAME|SOREG,		TCHAR|TUCHAR,
	SNAME|SOREG,		TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"movb	AR,AL\n", },

{ ASSIGN,	FOREFF|FORCC,
	SNAME|SOREG,		TCHAR|TUCHAR,
	SCON,			TCHAR|TUCHAR,
		0,	RDEST|RESCC,
		"li	AR*256,AL\n", },

/* Floating point */

{ ASSIGN,	FOREFF|INCREG,
	SCREG,		TDOUBLE|TFLOAT,
	SCREG|SNAME|SOREG,	TDOUBLE|TFLOAT,
		0,	RDEST,
		"lr	AR\n", },

{ ASSIGN,	FOREFF|INCREG,
	SCREG,		TDOUBLE|TFLOAT,
	SCON	,	TDOUBLE|TFLOAT,
		0,	RDEST,
		"li	AL,AR\nli	UL,UR\n", },

{ ASSIGN,	FOREFF|INCREG,
	SCREG,		TDOUBLE|TFLOAT,
	SCON,		TDOUBLE|TFLOAT,
		0,	RDEST,
		"li	AL,AR\nli	UL,UR\n", },

{ ASSIGN,	FOREFF|INCREG,
	SNAME|SOREG,	TDOUBLE|TFLOAT,
	SNAME|SOREG,	TDOUBLE|TFLOAT,
		0,	RDEST,
		"mov	AR,AL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF|INCREG,
	SNAME|SOREG,	TDOUBLE|TFLOAT,
	SCREG	,	TDOUBLE|TFLOAT,
		0,	RDEST,
		"str	AL\n", },

/* Struct assigns */
{ STASG,	FOREFF|INAREG,
	SOREG|SNAME,	TANY,
	SAREG,		TPTRTO|TANY,
		NSPECIAL,	RDEST,
		"ZI", },

/*
 * DIV/MOD/MUL 
 *
 * FIXME: Rules
 *	Signed: DIVS or MPYS take an address for one argument, the other
 *	is in R0 and the result ends up in R0/R1
 *	Unsigned: DIVS/MPYS take a reg and an arg and use reg/reg+1 for
 *	result
 *
 *	This is a fudge to get us working
 */
{ MUL,	INAREG,
	SAREG,			TWORD|TPOINT,
	SAREG|SOREG|SNAME,	TWORD|TPOINT,
		NSPECIAL,	RLEFT,
		"mpy	AR,AL\n", },

{ MUL,	INBREG,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	mul32\n", },

{ MUL,	INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	mul32i\n.word	UR\n.word	AR\n", },

{ MUL,	INCREG,
	SCREG,		TFLOAT|TDOUBLE,
	SCREG|SOREG|SNAME,	TFLOAT|TDOUBLE,
		0,	RLEFT,
		"mr\n", },

/* signed divide r0/r1 by operand into r0/r1 (r1 = remainder) */

/* FIXME: this needs to fix the sign */
{ DIV,	INAREG,
	SAREG,			TINT|TPOINT,
	SAREG|SNAME|SOREG,	TINT|TPOINT,
		NSPECIAL,	RDEST,
		"clr	r0\ndivs	AR\n", },

/* div can use thing sother than r0/r1 but we don't */
{ DIV,	INAREG,
	SAREG,			TUNSIGNED,
	SAREG|SNAME|SOREG,	TUNSIGNED,
		NSPECIAL,	RDEST,
		"clr	r0\ndiv	AR,r0\n", },

/* FIXME: C rule for divides ordering */
{ DIV,	INBREG,
	SBREG,			TULONG,
	SCON,			TULONG,
		NSPECIAL,		RLEFT,
		"bl	div32i\n.word	UR\n.word	AR\n", },

{ DIV,	INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@divs32i\n.word	UR\n.word	AR\n", },

{ DIV,	INBREG,
	SBREG,			TULONG,
	SBREG,			TULONG,
		NSPECIAL,		RLEFT,
		"bl	div32", },

{ DIV,	INBREG,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@divs32\n", },

{ DIV,	INCREG,
	SCREG,			TDOUBLE|TFLOAT,
	SCREG|SNAME|SOREG,	TDOUBLE|TFLOAT,
		0,	RLEFT,
		"dr	AR\n", },

/* signed divide r0/r1 by operand into r0/r1 (r1 = remainder) */

{ MOD,	INAREG,
	SAREG,			TINT|TPOINT,
	SAREG|SNAME|SOREG,	TINT|TPOINT,
		NSPECIAL,	RDEST,
		"clr	r0\ndivs	AR\n", },

/* div can use things other than r0/r1 but we don't */
{ MOD,	INAREG,
	SAREG,			TUNSIGNED,
	SAREG|SNAME|SOREG,	TUNSIGNED,
		NSPECIAL,	RDEST,
		"clr	r0\ndiv	AR,r0\n", },

/* FIXME: C rule for divides ordering */
{ MOD,	INBREG,
	SBREG,			TULONG,
	SCON,			TULONG,
		NSPECIAL,		RLEFT,
		"bl	mod32i\n.word	UR\n.word	AR\n", },

{ MOD,	INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@mods32i\n.word	UR\n.word	AR\n", },

{ MOD,	INBREG,
	SBREG,			TULONG,
	SBREG,			TULONG,
		NSPECIAL,		RLEFT,
		"bl	mod32", },

{ MOD,	INBREG,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	mods32", },

/*
 * Indirection operators.
 */
{ UMUL,	INBREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TLONG|TULONG,
		NBREG,	RESC1, /* |NBSL - may overwrite index reg */
		"mov	AL,A1\nmov	UL,U1\n", },

{ UMUL,	INAREG,
	SANY,	TPOINT|TWORD,
	SOREG,	TPOINT|TWORD,
		NAREG|NASL,	RESC1,
		"mov	AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TANY,
	SOREG,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"movb	AL,A1\n", },

{ UMUL,	INCREG,
	SOREG|SNAME,	TANY,
	SCREG,	TDOUBLE|TFLOAT,
		NCREG,	RESC1,
		"lr	AL\n", },

{ UMUL,	INCREG,
	SCON,	TANY,
	SOREG,	TFLOAT,
		NCREG,	RESC1,
		"li	AL,A1\nli	UL,U1\n", },

{ UMUL,	INCREG,
	SOREG|SNAME,	TANY,
	SOREG,	TFLOAT,
		NCREG,	RESC1,
		"mov	AL,A1\nmov	UL,U1", },

/*
 * Logical/branching operators
 */
{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME|SCON,	TWORD|TPOINT,
	SZERO,	TANY,
		0, 	RESCC,
		"ci	AL, 0\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME|SCON,	TCHAR|TUCHAR,
	SCON,	TANY,
		0, 	RESCC,
		"cb	@__litb_AR, AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TWORD|TPOINT,
	SAREG|SOREG|SNAME,	TWORD|TPOINT,
		0, 	RESCC,
		"c	AR,AL\n", },

{ OPLOG,	FORCC,
	SAREG|SOREG|SNAME,	TWORD|TPOINT,
	SCON,			TWORD|TPOINT,
		0, 	RESCC,
		"ci	AL,AR\n", },

/* FIXME: need to tell it that this destroys right hand and hope that's
   still usable, or need helper - which ? */
{ OPLOG,	FORCC,
	SNAME|SOREG,	TFLOAT|TDOUBLE,
	SCREG,		TFLOAT|TDOUBLE,
		0, 	RESCC,
		"sr	AL\n", },

{ OPLOG,	FORCC,
	SAREG|SCON,	TCHAR|TUCHAR,
	SAREG|SCON,	TCHAR|TUCHAR,
		0, 	RESCC,
		"cb	AL,AR\n", },

{ OPLOG,	FORCC,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RNULL,
		"ZF", },

{ AND,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SCON|SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RLEFT|RESCC,
		"and	AR,AL\nand	UR,UL\n", },

/* set status bits */
{ AND,	FORCC,
	ARONS|SCON,	TWORD|TPOINT,
	ARONS|SCON,	TWORD|TPOINT,
		0,	RESCC,
		"and	AR,AL\n", },

/* AND with int */
{ AND,	INAREG|FORCC|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SAREG|SOREG|SNAME,	TWORD,
		0,	RLEFT|RESCC,
		"and	AR,AL\n", },

{ AND,	INAREG|FORCC|FOREFF,
	SAREG|SNAME|SOREG,	TWORD,
	SCON,			TWORD,
		0,	RLEFT|RESCC,
		"andi	AL,AR\n", },

/* AND with char */
{ AND,	INAREG|FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	ARONS,			TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"andb	AR,AL\n", },

{ AND,	INAREG|FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON,			TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"andb	@_litb_AR,AL\n", },

{ OR,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RLEFT|RESCC,
		"or	AR,AL\nor	UR,UL\n", },

{ OR,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		0,	RLEFT|RESCC,
		"ori	AL,AR\nori	UL,UR\n", },

/* OR with int */
{ OR,	FOREFF|INAREG|FORCC,
	ARONS,		TWORD,
	ARONS|SCON,	TWORD,
		0,	RLEFT|RESCC,
		"or	AR,AL\n", },

{ OR,	FOREFF|INAREG|FORCC,
	ARONS,		TWORD,
	SCON,		TWORD,
		0,	RLEFT|RESCC,
		"ori	AL,AR\n", },

/* OR with char */
{ OR,	INAREG|FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	ARONS,			TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"orb	AR,AL\n", },

{ OR,	INAREG|FORCC,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
	SCON,			TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"orb	@_litb_AR,AL\n", },

/* No XORI */

/* XOR with int  */
{ ER,	INAREG|FORCC,
	ARONS,	TWORD|TCHAR|TUCHAR,
	SAREG,	TWORD|TCHAR|TUCHAR,
		0,	RLEFT|RESCC,
		"xor	AR,AL\n", },

/* XOR with long (extended insn)  */
{ ER,	INBREG|FORCC,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SBREG,	TLONG|TULONG,
		0,	RLEFT|RESCC,
		"xor	AR,AL\nxor	UR,UL\n", },

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
	SCON,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"ljmp	LL\n", },

{ GOTO, 	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"b	*AL\n", },

/*
 * Convert LTYPE to reg.
 */

/* XXX - avoid OREG index register to be overwritten */
/* Const form needed */
{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SCON,	TLONG|TULONG,
		NBREG,	RESC1,
		"li	AL,A1\nli	UL,U1\n", },

{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SCON|SBREG|SNAME|SOREG,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	AL,A1\nmov	UL,U1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SCON,		TWORD|TPOINT,
		NAREG|NASR,	RESC1,
		"li	A1,AL\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SCON,		TCHAR|TUCHAR,
		NAREG|NASR,	RESC1,
		"li	A1*256,AL\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME,	TWORD|TPOINT,
		NAREG|NASR,	RESC1,
		"mov	AL,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME,	TCHAR,
		NAREG,		RESC1,
		"movb	AL,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME,	TUCHAR,
		NAREG,		RESC1,
		"movb	AL,A1", },

{ OPLTYPE,	INCREG,
	SANY,			TANY,
	SOREG|SNAME,		TDOUBLE|TFLOAT,
		NCREG,		RESC1,
		"lr	AL\n", },

/*
 * Negate a word.
 */
{ UMINUS,	INAREG|FOREFF,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
	SANY,	TANY,
		0,	RLEFT,
		"neg	AL\n", },

{ UMINUS,	INBREG|FOREFF,
	SBREG,			TLONG,
	SBREG,			TANY,
		NSPECIAL,	RLEFT,
		"bl	@neg32\n", },

{ UMINUS,	INCREG|FOREFF,
	SCREG,	TFLOAT|TDOUBLE,
	SANY,	TANY,
		0,	RLEFT,
		"negr\n", },


{ COMPL,	INBREG,
	SBREG,	TLONG|TULONG,
	SANY,	TANY,
		0,	RLEFT,
		"inv	AL\ninv	UL\n", },

{ COMPL,	INAREG,
	SAREG,	TWORD,
	SANY,	TANY,
		0,	RLEFT,
		"inv	AL\n", },

/*
 * Arguments to functions.
 */
{ FUNARG,	FOREFF,
	SBREG|SNAME|SOREG,	TLONG|TULONG,
	SZERO,	TLONG|TULONG,
		0,	RNULL,
		"ZAdect	r6\nclr	*r6\ndect	r6\nclr	*r6\n", },
{ FUNARG,	FOREFF,
	SBREG|SNAME|SOREG,	TLONG|TULONG,
	SANY,	TLONG|TULONG,
		0,	RNULL,
		"ZAdect	r6\nmov	AL,*r6\ndect	r6\nmov UL,*r6\n", },

{ FUNARG,	FOREFF,
	SZERO,	TANY,
	SANY,	TANY,
		0,	RNULL,
		"ZAdect	r6\nclr	*r6\n", },

#if 0
{ FUNARG,	FOREFF,
	SARGSUB,	TWORD|TPOINT,
	SANY,		TWORD|TPOINT,
		0,	RNULL,
		"ZAdect	r6\nmov AL,*r6\n", },
#endif
		
#if 0
{ FUNARG,	FOREFF,
	SARGINC,	TWORD|TPOINT,
	SANY,		TWORD|TPOINT,
		0,	RNULL,
		"ZH", },
#endif
{ FUNARG,	FOREFF,
	SAREG|SNAME|SOREG,	TWORD|TPOINT,
	SANY,	TWORD|TPOINT,
		0,	RNULL,
		"ZAdect	r6\nmov	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TCHAR,
	SANY,		TCHAR,
		NAREG,	RNULL,
		"ZAdect	r6\nmovb	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TUCHAR,
	SANY,		TUCHAR,
		NAREG,	RNULL,
		"ZAdect	r6\nmovb	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SAREG,	TUCHAR|TCHAR,
	SANY,	TUCHAR|TCHAR,
		0,	RNULL,
		"ZAdect	r6\nmovb	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SCREG,	TFLOAT|TDOUBLE,
	SANY,		TANY,
		0,	RNULL,
		"ZAdect	r6\nmov r1,*r6\ndect	r6\nmov r0,*r6\n", },

{ STARG,	FOREFF,
	SAREG,	TPTRTO|TANY,
	SANY,	TSTRUCT,
		NSPECIAL,	0,
		"ZJ", },


# define DF(x) FORREW,SANY,TANY,SANY,TANY,REWRITE,x,""

{ UMUL, DF( UMUL ), },

{ ASSIGN, DF(ASSIGN), },

{ STASG, DF(STASG), },

{ FLD, DF(FLD), },

{ OPLEAF, DF(NAME), },

/* { INIT, DF(INIT), }, */

{ OPUNARY, DF(UMINUS), },

{ OPANY, DF(BITYPE), },

{ FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	FREE,	"help; I'm in trouble\n" },
};

int tablesize = sizeof(table)/sizeof(table[0]);
