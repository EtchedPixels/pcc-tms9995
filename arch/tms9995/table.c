/*	$Id: table.c,v 1.17 2019/04/27 20:35:52 ragge Exp $	*/
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
		0,	RLEFT,
		"", },


/* Unsigned char to char : no work needed */
{ SCONV,	INAREG,
	SAREG,	TUCHAR,
	SAREG,	TCHAR,
		0,	RLEFT,
		"", },

/* Char to unsigned char : no work needed */
{ SCONV,	INAREG,
	SAREG,	TCHAR,
	SAREG,	TUCHAR,
		0,	RLEFT,
		"", },

/* Char to int or unsigned : move byte down and sign extend */
{ SCONV,	INAREG,
	SAREG,	TCHAR,
	SAREG,	TWORD,
		NAREG|NASL,	RESC1,
		"sra	AL,8\n", },

/* Char to int or unsigned : from memory */
{ SCONV,	INAREG,
	SOREG|SNAME,		TCHAR,
	SAREG,			TWORD,
		NAREG|NASL,	RESC1,
		"movb	AL,A1\nsra	A1,8\n", },

/* Char or unsigned char to int or uint: constant */
{ SCONV,	INAREG,
	SCON,			TCHAR|TUCHAR,
	SAREG,			TWORD,
		NAREG|NASL,	RESC1,
		"li	A1,CL\n", },

/* Unsigned char to int or uint: register */
{ SCONV,	INAREG,
	SAREG,	TUCHAR,
	SAREG,	TWORD,
		NAREG|NASL,	RESC1,
		"srl	AL,8\n", },

/* Unsigned char to int or uint: memory */
{ SCONV,	INAREG,
	SOREG|SNAME,	TUCHAR,
	SAREG,	TINT|TUNSIGNED,
		NAREG,	RESC1,
		"clr	A1\nmovb	AL,A1\nswpb	A1\n", },

/* char to ulong: register - no sign extend to expands somewhat */
{ SCONV,	INBREG,
	SAREG,	TCHAR,
	SANY,	TULONG|TLONG,
		NBREG,	RESC1,
		"mov	AL,Z1clr	U1\nsra	Z1,8\nci	A1,0\nZBjge	ZE\ndec	U1\nZD", },

/* Unsigned char to unsigned long: register */
{ SCONV,	INBREG,
	SAREG,	TUCHAR,
	SANY,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"mov	AL,Z1\nsrl	Z1,8\nclr	U1\n", },

/* Unsigned char to float/double */
/* Constant is forced into R0, masked and converted */
{ SCONV,	INCREG,
	SAREG,	TCHAR|TUCHAR,
	SANY,	TFLOAT,
		NSPECIAL|NCREG,	RESC1,
		"andi	AL,0xff\ncir	AL\n", },

/* Int or unsigned to char or unsigned: register */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"swpb	AL\n", },

/* Int or unsigned to int or unsigned: register */
{ SCONV,	INAREG,
	SAREG,	TWORD,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* Pointer to char: register */
{ SCONV,	INAREG,
	SAREG,	TPOINT,
	SAREG,	TCHAR,
		NAREG|NASL,	RESC1,
		"swpb	AL\n", },

/* Pointer to int or uint: no work */
{ SCONV,	INAREG,
	SAREG,	TPOINT,
	SANY,	TWORD,
		0,	RLEFT,
		"", },

/* Int to long/ulong:  Expands due to lack of sign extend */
{ SCONV,	INBREG,
	SAREG,	TINT|TPOINT,
	SANY,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	A1,Z1\nclr	U1\nci	Z1,0\nZBjge	ZE\ndec	U1\nZD", },

/* int to float: nice and easy except for the register swap */
{ SCONV,	INCREG,
	SAREG,	TINT,
	SANY,	TFLOAT,
		NSPECIAL|NCREG,	RESC1,
		"bl	@cir_AL\n", },

/* int to float: object in memory: actually easier */
{ SCONV,	INCREG,
	SNAME|SOREG,	TINT,
	SANY,	TFLOAT,
		NSPECIAL|NCREG,	RESC1,
		"cir	AL\n", },

/* The compiler isn't too smart at folding matching registers into pairs */
/* unsigned int to long or ulong: just clear the upper */
{ SCONV,	INBREG,
	SAREG,	TUNSIGNED,
	SANY,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	AL,Z1\nclr	U1; sconvu16u32 AL,A1\n", },

/* unsigned int to long or ulong: generic move and conversion for unsigned to long/ulong */
{ SCONV,	INBREG,
	SOREG|SNAME,	TUNSIGNED,
	SANY,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	AL,Z1\nclr	U1\n", },

/* unsigned int to float : we don't have a 16bit op for it. use the 32bit one */
{ SCONV,	INCREG,
	SAREG,		TUNSIGNED,
	SANY,		TFLOAT,
		NSPECIAL,	RLEFT,
		"clr	r0\ncer AL\n", },

/* long or unsigned long to char or uchar: lower byte swapped*/
{ SCONV,	INAREG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
	SAREG,			TCHAR|TUCHAR,
		NAREG,		RESC1,
		"mov	ZL,A1; SCONV breg areg AL, A1\nswpb	A1\n", },

/* long or unsigned long to int or uint: no work required */
/* Alas the compiler isn't quite smart enough for this to happen in situ */
{ SCONV,	INAREG,
	SBREG,		TLONG|TULONG,
	SAREG,		TWORD,
		NAREG|NASL,	RESC1,	/*XX */
		"mov	ZL,A1; sconv breg areg AL, A1\n", },

/* (u)long -> (u)long, nothing */
{ SCONV,	INBREG,
	SBREG,	TLONG|TULONG,
	SANY,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"", },

/* long -> float/double */
{ SCONV,	INCREG,
	SNAME|SOREG,	TLONG,
	SANY,		TFLOAT,
		NSPECIAL,	RLEFT,
		"cer AL\n", },

/* ulong -> float/double */
{ SCONV,	INCREG,
	SBREG,		TULONG,
	SANY,		TFLOAT,
		NSPECIAL|NCSL,	RLEFT,
		"bl	@u32fp\n", },


/* float to long or ulong : helper converts and flips in situ */
/* CRE needs the words flipping */
{ SCONV,	INBREG,
	SCREG,	TFLOAT,
	SANY,	TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@cre_flip\n" },

/* float to int : special rule plonks it in the right place */
{ SCONV,	INAREG,
	SCREG,	TFLOAT,
	SANY,	TINT,
		NSPECIAL,	RLEFT,
		"cri\n", },

/* float/double -> float/double */
{ SCONV,	INCREG,
	SCREG,	TFLOAT,
	SANY,	TANY,
		0,	RLEFT,
		"", },

/*
 * Subroutine calls.
 */
{ CALL,		INAREG,
	SCON,	TANY,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INAREG,
	SNAME|SCON|SOREG,	TANY,
	SAREG,	TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"bl	AL\nZC", },

{ CALL,		INAREG,
	SAREG|SOREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	*AL\nZC", },

{ UCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	*AL\nZC", },

{ CALL,		INBREG,
	SNAME|SCON|SOREG,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	AL\nZC", },

{ UCALL,	INBREG,
	SNAME|SCON|SOREG,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	AL\nZC", },

{ CALL,		INBREG,
	SAREG,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	*AL\nZC", },

{ UCALL,	INBREG,
	SAREG,	TANY,
	SBREG,	TLONG|TULONG,
		NBREG|NBSL,	RESC1,
		"bl	*AL\nZC", },

{ CALL,		INCREG,
	SNAME|SCON|SOREG,	TANY,
	SCREG,	TFLOAT,
		NCREG|NCSL,	RESC1,
		"fbl	AL\nZC", },

{ UCALL,	INCREG,
	SNAME|SCON|SOREG,	TANY,
	SCREG,	TFLOAT,
		NCREG|NCSL,	RESC1,
		"fbl	AL\nZC", },

{ CALL,		INCREG,
	SAREG,	TANY,
	SCREG,	TFLOAT,
		NCREG|NCSL,	RESC1,
		"bl	*AL\nZC", },

{ UCALL,	INCREG,
	SAREG,	TANY,
	SCREG,	TFLOAT,
		NCREG|NCSL,	RESC1,
		"bl	*AL\nZC", },

{ CALL,		FOREFF,
	SNAME|SCON|SOREG,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ UCALL,	FOREFF,
	SNAME|SCON|SOREG,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ CALL,		FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	*AL\nZC", },

{ UCALL,	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	*AL\nZC", },

{ STCALL,	INAREG,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"bl	AL\nZC", },

{ USTCALL,	INAREG,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,
		"bl	AL\nZC", },

{ STCALL,	FOREFF,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ USTCALL,	FOREFF,
	SCON|SOREG|SNAME,	TANY,
	SANY,	TANY,
		0,	0,
		"bl	AL\nZC", },

{ STCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	*AL\nZC", },

{ USTCALL,	INAREG,
	SAREG,	TANY,
	SANY,	TANY,
		NAREG|NASL,	RESC1,	/* should be 0 */
		"bl	*AL\nZC", },

/*
 * The next rules handle all binop-style operators.
 */

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

/* No ADC */
{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SONE,			TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@inc32\n", },

{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		0,	RLEFT,
		"ZBa	UR,UL\na	ZR,ZL\njnc	ZE\ninc	UL\nZD", },

{ PLUS,		INBREG|FOREFF,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,	RLEFT,
		/* Words reversed for speed */
		"bl	@add32i\n.word	CR\n.word	ZQ\n", },

/* Integer to pointer addition */
{ PLUS,		INAREG,
	SAREG,	TPOINT|TWORD,
	SAREG,	TINT|TUNSIGNED,
		0,	RLEFT,
		"a	AR,AL\n", },

/* Add to reg left and reclaim reg */
{ PLUS,		INAREG|FOREFF|FORCC,
	SAREG,		TWORD|TPOINT,
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
		"a	@__litb_CR,AL\n", },

/* floating point */

{ PLUS,		INCREG|FOREFF|FORCC,
	SCREG,			TFLOAT,
	SCREG|SNAME|SOREG,	TFLOAT,
		NSPECIAL,	RDEST,
		"ar	AR ; plus into AL\n", },

{ MINUS,		INBREG|FOREFF,
	SBREG,		TLONG|TULONG,
	SONE,		TLONG|TULONG,
		NSPECIAL,	RLEFT,
		"bl	@dec32\n", },

{ MINUS,		INBREG|FOREFF,
	SBREG,		TLONG|TULONG,
	SCON,		TLONG|TULONG,
		NSPECIAL,	RDEST,
		/* Words reversed for speed */
		"bl	@sub32i\n.word	CR\n.word	ZQ\n", },

{ MINUS,		INBREG|FOREFF,
	SBREG,		TLONG|TULONG,
	SBREG,		TLONG|TULONG,
		NSPECIAL,	RDEST,
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

/* Sub one left but use only for side effects */
{ MINUS,	FOREFF,
	SAREG,		TCHAR|TUCHAR,
	SONE,			TANY,
		0,	RLEFT,
		"ai	AL, -0x100\n", },

/* Sub from anything left but use only for side effects. Need to review
   because of the SUB 0 funny */
{ MINUS,		FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
		0,	RLEFT,
		"sb	AR,AL\n", },

{ MINUS,		FOREFF,
	SAREG|SNAME|SOREG,	TCHAR|TUCHAR,
	SCON,			TCHAR|TUCHAR,
		0,	RLEFT,
		"swpb	AL\nsi	AR,AL\nswpb	AL\n", },

/* floating point */
{ MINUS,	INCREG|FOREFF|FORCC,
	SCREG,			TFLOAT,
	SCREG|SNAME|SOREG,	TFLOAT,
		NSPECIAL,	RDEST|RESCC,
		"sr	AR; subtract from AL\n", },

/*
 * The next rules handle all shift operators.
 *
 * r0 is the only permitted 'amount', and constants
 * must be 1-15. Need to check compiler never tries to output silly values
 */
 
{ LS,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SAREG,	TINT|TUNSIGNED,
		NSPECIAL,	RLEFT,
		"bl	@ls32\n", },


/* Shift of 8 or 16bit type by constant*/

/* Shift of a register by a constant, works for 8 and 16bit */
{ LS,	INAREG|FOREFF,
	SAREG,	TWORD|TCHAR|TUCHAR,
	SCON,	TWORD,
	0,	RLEFT,
		"sla	AL,CR\n", },
		
/* Shift of a register by a register. We must shift by R0, so we cannot
   keep the data in R0 */
{ LS,	INAREG|FOREFF,
	SAREG,	TINT|TCHAR|TUNSIGNED|TUCHAR,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"sla	AL,AR\n", },

/* Constant shift of a word in memory */
{ LS,	INAREG|FOREFF,
	SOREG|SNAME, TWORD,
	SCON,	TWORD,
	0,	RLEFT,
		"sla	AL,CR\n", },

/* Register shift of a word in memory */
{ LS,	INAREG|FOREFF,
	SOREG|SNAME,	TWORD,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"sla	AL,AR\n", },

/* Same again - right unsigned 8 or 16bit shifts */

/* Shift of a register by a constant, works for 8 and 16bit */
{ RS,	INAREG|FOREFF,
	SAREG,	TUCHAR|TUNSIGNED,
	SCON,	TWORD,
	0,	RLEFT,
		"srl	AL,CR\n", },	/* XXX */
		
{ RS,	INAREG|FOREFF,
	SAREG,	TUCHAR|TUNSIGNED,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"srl	AL,AR\n", },

/* And signed */

{ RS,	INAREG|FOREFF,
	SAREG,	TCHAR|TINT,
	SCON,	TWORD,
	0,	RLEFT,
		"sra	AL,CR\n", },
		
{ RS,	INAREG|FOREFF,
	SAREG,	TCHAR|TINT,
	SAREG,	TWORD,
	NSPECIAL,	RLEFT,
		"sra	AL,AR\n", },

/* And 32bit */

{ RS,	INBREG|FOREFF,
	SBREG,	TLONG,
	SCON,	TWORD,
		NSPECIAL,	RLEFT,
		"bl	@rss32i\n.word	ZQ\n", },

{ RS,	INBREG|FOREFF,
	SBREG,	TLONG,
	SAREG,	TWORD,
		NSPECIAL,	RLEFT,
		"bl	@rss32\n", },

{ RS,	INBREG|FOREFF,
	SBREG,	TULONG,
	SCON,	TWORD,
		NSPECIAL,	RLEFT,
		"bl	@rsu32i\n.word	ZQ\n", },

{ RS,	INBREG|FOREFF,
	SBREG,	TULONG,
	SAREG,	TWORD,
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
		"clr	AL ; assign AR to AL\n", },

/* Clear word at reg */
{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TWORD|TPOINT,
	SZERO,		TANY,
		0,	RDEST,
		"clr	AL ; assign AR to AL\n", },

/* Clear byte in reg */
{ ASSIGN,	FOREFF|INAREG,
	SAREG,	TCHAR|TUCHAR,
	SZERO,	TANY,
		0,	RDEST,
		"clrb	AL ; assign AR to AL\n", },

/* The next is class B regs */

/* Clear long at address or in reg */
{ ASSIGN,	FOREFF|INBREG,
	SNAME|SOREG|SBREG,	TLONG|TULONG,
	SZERO,			TANY,
		0,	RDEST,
		"clr	ZL\nclr	UL\n", },

/* Must have multiple rules for long otherwise regs may be trashed */
{ ASSIGN,	FOREFF|INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		0,	RDEST,
		"ZN", },

{ ASSIGN,	FOREFF|INBREG,
	SBREG,			TLONG|TULONG,
	SNAME|SOREG,		TLONG|TULONG,
		0,	RDEST,
		"mov	ZR,ZL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF|INBREG,
	SNAME|SOREG,	TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		0,	RDEST,
		"mov	ZR,ZL\nmov	UR,UL\n", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLONG|TULONG,
	SCON,		TLONG|TULONG,
		0,	0,
		"ZN", },

{ ASSIGN,	FOREFF,
	SNAME|SOREG,	TLONG|TULONG,
	SNAME|SOREG,	TLONG|TULONG,
		0,	0,
		"mov	ZR,ZL\nmov	UR,UL\n", },

{ ASSIGN,	INBREG|FOREFF,
	SBREG,	TLONG|TULONG,
	SBREG,	TLONG|TULONG,
		0,	RDEST,
		"ZG", },

{ ASSIGN,	FOREFF|INAREG|FORCC,
	SAREG,			TWORD|TPOINT,
	SCON,			TWORD|TPOINT,
		0,	RDEST|RESCC,
		"li	AL,CR\n", },

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
		"li	AL,CR\n", },

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
		"li	AL, CR * 256\n", },

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
		"li	AL, CR*256\n", },

/* Floating point */

{ ASSIGN,	FOREFF|INCREG,
	SCREG|SNAME|SOREG,	TFLOAT,
	SCREG|SNAME|SOREG,	TFLOAT,
	0,	RDEST,
		"; mov AR,AL\nZH", },

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
	SCON,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@mul32i\n.word	UR\n.word	ZQ\n", },

{ MUL,	INBREG,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@mul32\n", },

{ MUL,	INCREG,
	SCREG,			TFLOAT,
	SCREG|OREG|SNAME,	TFLOAT,
		NSPECIAL,	RDEST,
		"mr	AR; AL *= AR\n", },

/* signed divide r0/r1 by operand into r0/r1 (r1 = remainder) */
/* We only have a 32 by 16 divide so sign extend first */
{ DIV,	INAREG,
	SAREG,			TINT|TPOINT,
	SAREG|SNAME|SOREG,	TINT|TPOINT,
		NSPECIAL,	RDEST,
		"clr	r0\nci	r1,0\nZBjge	ZE\ndec	r0\nZD\ndivs	AR\n", },

/* div can use things other than r0/r1 but we don't */
{ DIV,	INAREG,
	SAREG,			TUNSIGNED,
	SAREG|SNAME|SOREG,	TUNSIGNED,
		NSPECIAL,	RDEST,
		"clr	r0\ndiv	AR,r0\n", },

{ DIV,	INBREG,
	SBREG,			TLONG,
	SCON,			TLONG,
		NSPECIAL,		RLEFT,
		"bl	@divs32i\n.word	ZQ\n.word	CR\n", },

{ DIV,	INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@divs32i\n.word	ZQ\n.word	CR\n", },

{ DIV,	INBREG,
	SBREG,			TLONG,
	SBREG,			TLONG,
		NSPECIAL,		RLEFT,
		"bl	@divs32\n", },

{ DIV,	INBREG,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,		RDEST,
		"bl	@div32\n", },

{ DIV,	INCREG,
	SCREG,			TFLOAT,
	SCREG|SNAME|SOREG,		TFLOAT,
		NSPECIAL,	RDEST,
		"dr	AR; AL /= AR\n", },

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

{ MOD,	INBREG,
	SBREG,			TLONG,
	SCON,			TLONG,
		NSPECIAL,		RLEFT,
		"bl	@mods32i\n.word	ZQ\n.word	CR\n", },

{ MOD,	INBREG,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@mod32i\n.word	ZQ\n.word	CR\n", },

{ MOD,	INBREG,
	SBREG,			TLONG,
	SBREG,			TLONG,
		NSPECIAL,		RLEFT,
		"bl	@mods32\n", },

{ MOD,	INBREG,
	SBREG,			TLONG|TULONG,
	SBREG,			TLONG|TULONG,
		NSPECIAL,		RLEFT,
		"bl	@mod32\n", },

/*
 * Indirection operators.
 */
{ UMUL,	INBREG,
	SANY,	TPOINT|TWORD,
	SOREG|SNAME,	TLONG|TULONG,
		NBREG,	RESC1, /* |NBSL - may overwrite index reg */
		"mov	ZL,Z1\nmov	UL,U1\n", },

{ UMUL,	INAREG,
	SANY,	TPOINT|TWORD,
	SOREG|SNAME,	TPOINT|TWORD,
		NAREG|NASL,	RESC1,
		"mov	AL,A1\n", },

{ UMUL,	INAREG,
	SANY,	TPOINT | TWORD,
	SOREG|SNAME,	TCHAR|TUCHAR,
		NAREG|NASL,	RESC1,
		"movb	AL,A1\n", },

/* Use a pair of moves. For FR0 we could use LR but it's likely to be
   emulated anyway */
 { UMUL,	INCREG,
 	SANY,	TPOINT | TWORD,
 	SOREG|SNAME,	TFLOAT,
 		NCREG|NCSL,	RESC1,		/* ?? NCSL */
		"mov	ZL,Z1\nmov	UL,U1; umul AL into A1\n", },

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
		"cb	@__litb_CR, AL\n", },

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

/* No compare so we subtract and destroy */
{ OPLOG,	FORCC,
	SCREG|SNAME|SOREG,	TFLOAT,
	SCREG,		TFLOAT,
		NSPECIAL, 	RESCC,
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
		"and	ZR,ZL\nand	UR,UL\n", },

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
		"andb	@_litb_CR,AL\n", },

{ OR,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SBREG|SOREG|SNAME,	TLONG|TULONG,
		0,	RLEFT|RESCC,
		"or	ZR,ZL\nor	UR,UL\n", },

{ OR,	INBREG|FORCC,
	SBREG,			TLONG|TULONG,
	SCON,			TLONG|TULONG,
		0,	RLEFT|RESCC,
		"ori	ZL,ZR\nori	UL,UR\n", },

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
		"orb	@_litb_CR,AL\n", },

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
		"xor	ZR,ZL\nxor	UR,UL\n", },

/*
 * Jumps.
 */
{ GOTO, 	FOREFF,
	SCON|SNAME|SOREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"b	@LL\n", },

{ GOTO, 	FOREFF,
	SAREG,	TANY,
	SANY,	TANY,
		0,	RNOP,
		"b	*AL\n", },

/*
 * Convert LTYPE to reg.
 */

/* XXX - avoid OREG index register to be overwritten */

/* We have to keep separate const forms because of li v mov */
{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SCON,	TLONG|TULONG,
		NBREG,	RESC1,
		"ZO", },

{ OPLTYPE,	INBREG,
	SANY,	TANY,
	SBREG|SNAME|SOREG,	TLONG|TULONG,
		NBREG,	RESC1,
		"mov	ZL,Z1\nmov	UL,U1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SCON,		TWORD|TPOINT|TCHAR|TUCHAR,
		NAREG,	RESC1,
		"clr	A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SCON,		TWORD|TPOINT,
		NAREG,	RESC1,
		"li	A1,CL\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SCON,		TCHAR|TUCHAR,
		NAREG,	RESC1,
		"li	A1, CL * 256\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME,	TWORD|TPOINT,
		NAREG|NASR,	RESC1,
		"mov	AL,A1\n", },

{ OPLTYPE,	INAREG,
	SANY,	TANY,
	SAREG|SOREG|SNAME,	TCHAR|TUCHAR,
		NAREG,		RESC1,
		"movb	AL,A1\n", },

{ OPLTYPE,	INCREG,
	SANY,			TANY,
	SCREG|SOREG|SNAME,	TFLOAT,
		NCREG,		RESC1,
		"; opltype AL into A1\nZM", },

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
	SCREG,	TFLOAT,
	SANY,	TANY,
		NSPECIAL,	RLEFT,
		"negr ; fp negate AL\n", },


{ COMPL,	INBREG,
	SBREG,	TLONG|TULONG,
	SANY,	TANY,
		0,	RLEFT,
		"inv	ZL\ninv	UL\n", },

{ COMPL,	INAREG,
	SAREG,	TWORD,
	SANY,	TANY,
		0,	RLEFT,
		"inv	AL\n", },

/*
 * Arguments to functions.
 */
{ FUNARG,	FOREFF,
	SZERO,	TLONG|TULONG,
	SANY,	TANY,
		0,	RNULL,
		"ZSdect	r6\nclr	*r6\ndect	r6\nclr	*r6\n", },

{ FUNARG,	FOREFF,
	SBREG|SNAME|SOREG,	TLONG|TULONG,
	SANY,	TLONG|TULONG,
		0,	RNULL,
		"ZSdect	r6\nmov	ZL,*r6\ndect	r6\nmov UL,*r6\n", },

{ FUNARG,	FOREFF,
	SZERO,	TANY,
	SANY,	TANY,
		0,	RNULL,
		"ZSdect	r6\nclr	*r6\n", },

#if 0
{ FUNARG,	FOREFF,
	SARGSUB,	TWORD|TPOINT,
	SANY,		TWORD|TPOINT,
		0,	RNULL,
		"ZSdect	r6\nmov AL,*r6\n", },
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
		"ZSdect	r6\nmov	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TCHAR,
	SANY,		TCHAR,
		NAREG,	RNULL,
		"ZSdect	r6\nmovb	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SNAME|SOREG,	TUCHAR,
	SANY,		TUCHAR,
		NAREG,	RNULL,
		"ZSdect	r6\nmovb	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SAREG,	TUCHAR|TCHAR,
	SANY,	TUCHAR|TCHAR,
		0,	RNULL,
		"ZSdect	r6\nmovb	AL,*r6\n", },

{ FUNARG,	FOREFF,
	SCREG,	TFLOAT,
	SANY,		TANY,
		0,	RNULL,
		"ZSdect	r6\nmov ZL,*r6\ndect	r6\nmov UL,*r6\n", },

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
