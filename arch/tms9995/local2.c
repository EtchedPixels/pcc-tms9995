/*	$Id: local2.c,v 1.20 2019/04/27 20:35:52 ragge Exp $	*/
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
# include <ctype.h>
# include <string.h>

static void lpput(NODE *p);

static int spcoff;
static int argsiz(NODE *p);
static void negcon(FILE *fp, int con);

static const char *rpname[]  = {
	"rp01",
	"rp12",
	"rp23",
	"rp34",
	"rp45",
	"rp56",
	"rp67",
	"rp78",
	"rp89",
	"rp910",
	"rp1011",
	"rp1112",
	"rp1213",
	"rp1314",
	"rp1415",
	"rp150"
};

static const char *rname[] = {
	"r0",
	"r1",
	"r2",
	"r3",
	"r4",
	"r5",
	"r6",
	"r7",
	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"r15",
	"xxx"
};

static const char *fr_name[] = {
	"fr0",
	"@fr1",
	"@fr2",
	"@fr3"
};

static const char *fr_name_l[] = {
	"fr0",
	"@fr1+2",
	"@fr2+2",
	"@fr3+2"
};

static const char *fr_name_pic[] = {
	"xxxfr0",
	"@fr1(r15)",
	"@fr2(r15)",
	"@fr3(r15)",
	"XXX"
};

static const char *fr_name_pic_l[] = {
	"xxxfr0",
	"@fr1+2(r15)",
	"@fr2+2(r15)",
	"@fr3+2(r15)",
	"XXX"
};

#define REGBITS(n)	(n & 0x0F)

/* Print the name of a register */
static const char *regname(int n)
{
	int r = REGBITS(n);
	switch(GCLASS(n)) {
	case CLASSA:
		return rname[r];
	case CLASSB:
		/* This is only valid as an internal working name for debug */
		return rpname[r];
	case CLASSC:
		return kflag ? fr_name_pic[r]: fr_name[r];
	default:
		return "XXX";
	}
}

/* We keep registers in the low/high order. This would be nice to fix but
   the compiler core appears to have some internal assumptions about this
   and 'sameness' of differing register classes. It's only annoying for a
   few hardware ops */

/* Print the low part name of a register */
static const char *regname_l(int n)
{
	int r = REGBITS(n);
	switch(GCLASS(n)) {
	case CLASSB:
		return rname[r + 1];
	case CLASSA:
		return rname[r];
	case CLASSC:
		/* fr0 is an alias of r0/r1 */
		if (r)
			return kflag ? fr_name_pic_l[r]: fr_name_l[r];
		return "r1";
	default:
		return "XXX";
	}
}

/* Print the high part name of a register */
static const char *regname_h(int n)
{
	int r = REGBITS(n);
	switch(GCLASS(n)) {
	/* Not valid for class A - there is no high half but we sometimes
	   generate them when doing SCONVs and want it to mean the same b reg */
	case CLASSA:
		return "badh";
//		return rname[r - 1];
	case CLASSB:
		return rname[r];
	case CLASSC:
		if (r > 0)
			return kflag ? fr_name_pic[r]: fr_name[r];
		return "r0";
	default:
		return "xxx";
	}
}

void
deflab(int label)
{
	printf(LABFMT ":\n", label);
}

/*
 *	It would be nicer to put the frame pointer at the base of the frame
 *	with all values indexed upwards but that doesn't seem to be handled
 *	by the compiler core so do it the generic way
 */
void
prologue(struct interpass_prolog *ipp)
{
	int addto;
	int i;

#ifdef LANG_F77
	if (ipp->ipp_vis)
		printf(".export	%s\n", ipp->ipp_name);
	printf("%s:\n", ipp->ipp_name);
#endif
	printf("dect	r13\n");
	printf("mov	r11,*r13\n");
	printf("dect	r13\n");
	printf("mov	r12,*r13\n");

	/* Allow for the frame pointer and r11 save */
	addto = p2maxautooff - 2;
	if (addto & 1)
		addto++;

	/* We juggle these around a bit so that r7 is pointing to the
	   first value not to the saved old r7. This makes our first
	   stacked value accessible via *r7 which saves us a word (and
	   a bunch of cycles) a reference */

	if (addto == 2) {
		printf("dect	r13\n");
		/* Set up frame pointer */
		printf("mov	r13, r12\n");
	} else {
		/* Set up frame pointer */
		printf("mov	r13, r12\n");
		if (addto > 0)
			printf("ai	r13,%d\n", -addto);
		printf("dect	r12\n");
	}

	/* Save old register variables below the frame */
	for (i = 0; i < 16; i++)
		if (TESTBIT(p2env.p_regs, i))
			printf("dect	r13\nmov	%s,*r13\n",
				regname(i));

	/* Might be better to have an attribute for pic library entry funcs ? */
	if (kflag)
		printf("dect	r13\nmov	r15,*r13\n");
	spcoff = 0;
}

/* TODO - if we did an alloca() who owned the cleanup ?? */
void
eoftn(struct interpass_prolog *ipp)
{
	int i;
	if (spcoff)
		comperr("spcoff == %d", spcoff);
	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */

	/* our registers should be top of stack */
	for (i = 15; i >= 0; i--)
		if (TESTBIT(p2env.p_regs, i))
			printf("mov	*r13+, %s\n", regname(i));
	if (kflag)
		printf("mov	*r13+, r15\n");
	if (kflag == 2)
		printf("b	@cret(r14)\n");
	else
		printf("b	@cret\n");
}

/*
 * add/sub/...
 *
 * Param given:
 */
void
hopcode(int f, int o)
{
	char *str;

	switch (o) {
	case PLUS:
		str = "a";
		break;
	case MINUS:
		str = "s";
		break;
	case AND:
		str = "and";
		break;
	case OR:
		str = "or";
		break;
	case ER:
		str = "xor";
		break;
	default:
		comperr("hopcode2: %d", o);
		str = 0; /* XXX gcc */
	}
	printf("%s%c", str, f);
}

/*
 * Return type size in bytes.  Used by R2REGS, arg 2 to offset().
 */
int tlen(NODE *p)
{
	switch(p->n_type) {
		case CHAR:
		case UCHAR:
			return(1);

		case SHORT:
		case USHORT:
			return(SZSHORT/SZCHAR);

		case FLOAT:
		case DOUBLE:
			return(SZFLOAT/SZCHAR);

		case INT:
		case UNSIGNED:
		case LONG:
		case ULONG:
			return(SZINT/SZCHAR);

		case LONGLONG:
		case ULONGLONG:
			return SZLONGLONG/SZCHAR;

		default:
			if (!ISPTR(p->n_type))
				comperr("tlen type %d not pointer");
			return SZPOINT(p->n_type)/SZCHAR;
		}
}

static void
uconput(FILE *fp, NODE *p)
{
	int val = getlval(p);

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%d", val & 0xFFFF);
		} else if (p->n_type == LONG || p->n_type == ULONG) {
			val >>= 16;
			negcon(fp, val & 0xFFFF);
		} else
			negcon(fp, val);
		return;

	default:
		comperr("illegal conput, p %p", p);
	}
}


/*
 * Emit code to compare two long numbers.
 */
static void
twolcomp(NODE *p)
{
	int u;
	int s = getlab2();
	int e = p->n_label;
	int cb1, cb2;

	switch (u = p->n_op) {
	case NE:
		cb1 = 0;
		cb2 = NE;
		break;
	case EQ:
		cb1 = NE;
		cb2 = 0;
		break;
	case LE:
	case LT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case ULE:
	case ULT:
		cb1 = GT;
		cb2 = LT;
		break;
	case GE:
	case GT:
		u += (ULE-LE);
		/* FALLTHROUGH */
	case UGE:
	case UGT:
		cb1 = LT;
		cb2 = GT;
		break;

	default:
		cb1 = cb2 = 0; /* XXX gcc */
	}
	if (p->n_op >= ULE)
		cb1 += 4, cb2 += 4;
	if (p->n_right->n_op == ICON)
		expand(p, 0, "ci	AL,CR\n");
	else
		expand(p, 0, "c	AL,AR\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
	if (p->n_right->n_op == ICON) {
		expand(p, 0, "ci	UL,");
		uconput(stdout, p->n_right);
		printf("\n");
	} else
	        expand(p, 0, "c	UL,UR\n");
        cbgen(u, e);
        deflab(s);
}

/* Move floating point between two registers */
static void fpmove_r(int s, int d)
{
	char *or = kflag ? "(r15)" : "";
	printf(";fpmove %s, %s\n", regname(s), regname(d));
	if (s == d) {
		return;
	}
	if (d == FR0)
		printf("lr	@fr%d%s\n", s & 0x0F, or);
	else if (s == FR0)
		printf("str	@fr%d%s\n", d & 0x0F, or);
	else {
		s &= 0x0F;
		d &= 0x0F;
		printf("mov	@fr%d%s, @fr%d%s\n",
			s, or, d, or);
		printf("mov	@fr%d+2%s, @fr%d+2%s\n",
			s, or, d, or);
	}
}

/* R into L */
static void fpmove(NODE *p)
{
	int r = regno(p->n_right);
	int l = regno(p->n_left);

	/* Is the source a register ? */
	if (p->n_right->n_op == REG) {
		/* reg to reg allowing for virtual registers */
		if (p->n_left->n_op == REG) {
			fpmove_r(r, l);
			return;
		}
		if (r == FR0) {
			/* real fp */
			expand(p, 0, "str	AL\n");
			return;
		}
	}
	if (p->n_left->n_op == REG && l == FR0) {
		/* real fp dest */
		expand(p, 0, "lr	AR\n");
		return;
	}
	expand(p, 0, "mov	ZR,ZL\nmov	UR,UL; fpmove AR,AL\n");
}

/* R into P  where P is always going to be register allocated */
static void ofpmove(NODE *p)
{
	int r = regno(p->n_right);
	int l = 0;

	if (resc[1].n_op == FREE)
		comperr("ofpmove: free node");
	else
		l = regno(&resc[1]);

	/* Destination is the real fp registger */
	if (l == FR0) {
		/* real fp dest */
		expand(p, 0, "lr	AR\n");
		return;
	}
	/* Register to register (fake or real) */
	if (p->n_right->n_op == REG) {
		fpmove_r(r, l);
		return;
	}
	/* Two memory objects */
	expand(p, 0, "mov	ZR,Z1\nmov	UR,U1; ofpmove AR,A1\n");
}

/* rmove is used directly for temporary register moves in the compiler as
   well as by rule based code */
void
rmove(int s, int d, TWORD t)
{
	if (t == FLOAT) {
		fpmove_r(s, d);
		return;
	}
	printf(";rmove %d %d\n", s, d);
	if (t < LONG || t > BTMASK) {
		printf("mov	%s,%s\n",
			regname(s), regname(d));
	} else if (t == LONG || t == ULONG || t == FLOAT || t == DOUBLE) {
		/* avoid trashing double regs */
		if (d > s)
			printf("mov	%s,%s\nmov	%s,%s\n",
			regname_l(s), regname_l(d),
			regname_h(s), regname_h(d));
		else
			printf("mov	%s,%s\nmov	%s,%s\n",
			regname_h(s), regname_h(d),
			regname_l(s), regname_l(d));
	} else
		comperr("bad rmove: %d %d %x", s, d, t);

}

/* 32bit load from r (const) into l (reg). Minimize the number of 4 byte
   li operations in favour of two byte clr or mov */
static void load32(NODE *p)
{
	int l = p->n_left->n_reg;

	unsigned int r = getlval(p->n_right) ;
	unsigned int rlow = (r & 0xFFFF);
	unsigned int rhigh  = (r >> 16) & 0xFFFF;

	printf(";load32 r%d = %d\n", l, r);
	if (rlow == 0)
		printf("clr	%s\n", regname_l(l));
	else
		printf("li	%s, %d\n",
			regname_l(l), rlow);
	if (rhigh == 0)
		printf("clr	%s\n", regname_h(l));
	else if (rlow == rhigh)
		printf("mov	%s, %s\n", regname_l(l), regname_h(l));
	else
		printf("li	%s, %d\n", regname_h(l), rhigh);
}


/* 32bit load from l (const) into 1 (reg). Minimize the number of 4 byte
   li operations in favour of two byte clr or mov */

static void opload32(NODE *p)
{
	unsigned int r = getlval(p);
	unsigned int rlow = (r & 0xFFFF);
	unsigned int rhigh  = (r >> 16) & 0xFFFF;
	int l = 0;

	printf(";opload32 r%d = %d\n", r, l);
	if (resc[1].n_op == FREE)
		comperr("ofpmove: free node");
	else
		l = regno(&resc[1]);

	if (rlow == 0)
		printf("clr	%s\n", regname_l(l));
	else
		printf("li	%s, %d\n",
			regname_l(l), rlow);
	if (rhigh == 0)
		printf("clr	%s\n", regname_h(l));
	else if (rlow == rhigh)
		printf("mov	%s, %s\n", regname_l(l), regname_h(l));
	else
		printf("li	%s, %d\n", regname_h(l), rhigh);
}

static int zzlab;

void zzzcode(NODE *p, int c)
{
	struct attr *ap;
	NODE *l;
	int o;
	int len;

	switch (c) {
	case 'L':
	case 'R':
	case '1':
		lpput(getlr(p, c));
		break;
	case 'B': /* Assign a label (no 1f etc in assembler) */
		zzlab = getlab2();
		break;
	case 'C': /* subtract stack after call */
		spcoff -= p->n_qual;
		if (p->n_qual == 2)
			printf("inct	r13\n");
		else if (p->n_qual > 2)
			printf("ai	r13,%d\n", (int)p->n_qual);
		break;
	case 'D':
		/* Define the label from ZB */
		deflab(zzlab);
		break;
	case 'E':
		/* Print the ZB label */
		printf("@"LABFMT, zzlab);
		break;
	case 'F': /* long comparision */
		twolcomp(p);
		break;
	case 'G':
		rmove(regno(p->n_right), regno(p->n_left), p->n_type);
		break;
	case 'H': /* Load and store of fp registers real or fake */
		fpmove(p);
		break;
#if 0
	case 'G': /* printout a subnode for post-inc */
		adrput(stdout, p->n_left->n_left);
		break;

		expand(p->n_left->n_left, FOREFF, "mov	AL,ZA(sp)\n");
		expand(p->n_left->n_left, FOREFF, "inc	AL\n");
		break;
#endif
	/* TODO : unroll for smaller structs */
	case 'I': /* struct assign. Right in R1, left R2, counter R0. */
		ap = attr_find(p->n_ap, ATTR_P2STRUCT);
		l = p->n_left;
		len = ap->iarg(0);
		
		if (l->n_op == OREG) {
			int r = l->n_rval;
			if (R2TEST(r)) {
				l->n_rval = R2UPK1(r);
				expand(p, FOREFF, "mov	AL,r1\n");
				l->n_rval = r;
			} else {
				if (r != 1)
					printf("mov	%s,r1\n", regname(r));
				if (getlval(l))
					printf("ai	r1, %d\n", (int)getlval(l));
			}
		} else
			printf("li	r1, @%s\n", l->n_name);
		o = getlab2();
		printf("li	r0, %d\n", (len + 1) >> 1);
		deflab(o);
		printf("mov	*r2+, *r1+\n");
		printf("dec	r0\njne	" LABFMT "\n", o);
		break;
	/* TODO : unroll for smaller structs */
	case 'J': /* struct argument */
		ap = attr_find(p->n_ap, ATTR_P2STRUCT);
		o = (ap->iarg(0) + 1) & ~1;
		if (o == 2)
			printf("dect	r13");
		else
			printf("ai	r13, %d", -o);
		printf("mov	r13,r2\n");
		printf("li	r0, %d\n", o >> 1);
		deflab(o);
		printf("mov	*r1+, *r2+\n");
		printf("dec	r0");
		printf("jne	" LABFMT "\n", o);
		spcoff += argsiz(p);
		break;
	/* L see above */
	case 'M': /* Load of an fp reg via OPLTYPE */
		ofpmove(p);
		break;
	case 'N':
		/* Load 32bit immediate with architectural shortcuts */
		load32(p);
		break;
	case 'O':
		/* Load 32bit immediate with architectural shortcuts */
		opload32(p);
		break;
	case 'P':
		/* Insert a move if the 16 and 32bit registers don't happen
		   to align */
		if (resc[1].n_op == FREE)
			comperr("ZP: free node");
		if (regno(p->n_left) != regno(&resc[1]))
			expand(p, 0, "mov	AL,Z1; move AL to A1\n");
		break;
	case 'Q':
		/* Upper word right as constant without '@' */
		uconput(stdout, p->n_right);
		break;
	/* R see above */
	case 'S': /* Adust sp for argument push */
		spcoff += argsiz(p);
		break;
	case 'T': /* Constant - unsigned 8bit */
		printf("%u", (unsigned)getlval(p->n_right) & 0xFF);
		break;
	case 'U': /* Assignment 32bit maybe involving an OREG */
		/* Special handling because of R0 */
		/* Assign L = R */
		printf(";ZU\n");
		if (p->n_left->n_op == OREG && p->n_left->n_rval == R0)
			expand(p, 0, "mov	ZR, *r0+\nmov	UR,*r0\ndect	r0\n");
		else if (p->n_right->n_op == OREG && p->n_right->n_rval == R0)
			expand(p, 0, "mov	*r0+,ZL\nmov	*r0, UL\ndect	r0\n");
		else
			expand(p, 0, "mov	ZR,ZL\nmov	UR,UL\n");
		break;
	case 'V': /* Same thing for UMUL Z1 = ZR */
		printf(";ZV\n");
		if (p->n_op == OREG && p->n_rval == R0)
			expand(p, 0, "mov	*r0+,Z1\nmov	*r0,U1\ndect	r0\n");
		else
			expand(p, 0, "mov	ZR,Z1\nmov	UR,U1\n");
		break;
	case 'W': /* And for stack push */
		printf(";ZW\n");
		if (p->n_left->n_op == OREG && p->n_left->n_rval == R0)
			expand(p, 0, "ZSdect	r13\ninct	r0\nmov	*r0,*r13\ndect	r13\ndect	r0\nmov *r0,*r13\n");
		else
			expand(p, 0, "ZSdect	r13\nmov	ZL,*r13\ndect	r13\nmov	UL,*r13\n");
		break;
	case 'X':
		printf(";ZX");
		if (p->n_left->n_op == OREG && p->n_left->n_rval == R0)
			expand(p, 0, "clr	*r0+\nclr 	*r0\ndect	r0\n");
		else
			expand(p, 0, "clr	ZL\nclr	UL\n");
		break;
	case 'Z': /* Force debug */
		fwalk(p, e2print, 0);
		fflush(stdout);
		break;
	default:
		comperr("zzzcode %c", c);
	}
}

/*ARGSUSED*/
int
rewfld(NODE *p)
{
	return(1);
}

/*
 * Does the bitfield shape match?
 */
int
flshape(NODE *p)
{
	int o = p->n_op;

	if (o == OREG || o == REG || o == NAME)
		return SRDIR; /* Direct match */
	if (o == UMUL && shumul(p->n_left, SOREG))
		return SROREG; /* Convert into oreg */
	return SRREG; /* put it into a register */
}

/* INTEMP shapes must not contain any temporary registers */
/* XXX should this go away now? */
int
shtemp(NODE *p)
{
	return 0;
}

static void
negcon(FILE *fp, int con)
{
	if (con < 0)
		fprintf(fp, "-"), con = -con;
	fprintf(fp, "%d", con & 0xFFFF);
}

void
adrcon(CONSZ val)
{
	printf(CONFMT, val);
}

void
conput(FILE *fp, NODE *p)
{
	int val = getlval(p);

	switch (p->n_op) {
	case ICON:
		if (p->n_name[0] != '\0') {
			fprintf(fp, "%s", p->n_name);
			if (val)
				fprintf(fp, "+%d", val & 0xFFFF);
		} else if (p->n_type == LONG || p->n_type == ULONG)
			negcon(fp, val & 0xFFFF);
		else
			negcon(fp, val);
		return;

	default:
		comperr("illegal conput, p %p", p);
	}
}


/*ARGSUSED*/
void
insput(NODE *p)
{
	comperr("insput");
}

/*
 * Write out the upper address, like the upper register of a 2-register
 * reference, or the next memory location.
 */
void
upput(NODE *p, int size)
{
	size /= SZINT;
	switch (p->n_op) {
	case NAME:
	case OREG:
		adrput(stdout, p);
		break;
	case REG:
		printf("%s", regname_h(p->n_rval));
		break;
	case ICON:
		/* On TMS9995 upper value is high 16 bits */
		negcon(stdout, (getlval(p) >> 16) & 0xFFFF);
		break;
	default:
		comperr("upput bad op %d size %d", p->n_op, size);
	}
}

/*
 * Write out the lower address. As we are big endian this matters for
 * non word sized objects
 */
void lpput(NODE *p)
{
	switch (p->n_op) {
	case NAME:
	case OREG:
		setlval(p, getlval(p) + 2);
		adrput(stdout, p);
		setlval(p, getlval(p) - 2);
		break;
	case REG:
		printf("%s", regname_l(p->n_rval));
		break;
	case ICON:
		/* On TMS9995 lower value is low 16 bits */
		negcon(stdout, getlval(p) & 0xFFFF);
		break;
	default:
		comperr("lpput bad op %d", p->n_op);
	}
}

/*
 * output an address, with offsets, from p
 */
void
adrput(FILE *io, NODE *p)
{
	int lv;
	int at = 0;

	if (p->n_op == FLD)
		p = p->n_left;

	switch (p->n_op) {
	case NAME:
		/* A constant name possibly with an offset */
		fprintf(io, "@");
		if (p->n_name[0] != '\0') {
			fputs(p->n_name, io);
			if (getlval(p) != 0)
				fprintf(io, "+%d", (int)getlval(p) & 0xFFFF);
		} else
			negcon(io, getlval(p));
		return;

	case OREG:
		lv = getlval(p);
		/* A named object maybe with offset */
		if (p->n_name[0]) {
			fprintf(io, "@%s%s", p->n_name, lv ? "+" : "");
			at = 1;
		}
		if (lv) {
			if (!at)
				fprintf(io, "@");
			fprintf(io, "%d", lv);
			at = 1;
		}
#if 0		
		r = p->n_rval;
		if (R2TEST(r)) {
			if (lv)
				fprintf(io, "(%s)", rnames[R2UPK1(r)]);
			else
				fprintf(io, "*%s", rnames[R2UPK1(r)]);
		} else
#endif		
		if (at)
			fprintf(io, "(%s)", regname(p->n_rval));
		else
			fprintf(io, "*%s", regname(p->n_rval));
		return;
	case ICON:
		/* We will need to do some special handling for literals
		   when we get to it */
                fprintf(io, "@");
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case LONG:
		case ULONG:
			fprintf(io, "%s", regname_l(p->n_rval));
			break;
		default:
			fprintf(io, "%s", regname(p->n_rval));
		}
		return;

	case UMUL:
		if (tshape(p, STARNM)) {
			fprintf(io, "*");
			adrput(io, p->n_left);
			break;
		}
	default:
		comperr("illegal address, op %d, node %p", p->n_op, p);
		return;

	}
}

/*
 *	The real processor only allows for short range branches and
 *	also lacks signed <= and >= tests. The assembler pseudo ops
 *	however support all 10 we need and generates either the op if
 *	in range, or the reversed operation and a B @addr. This is also
 *	why the imaginary lte/gte exists. These are always generated
 *	inverted (ie ljlte foo is actually jgt 2; b @foo)
 */
static char *
ccbranches[] = {
	"ljeq",		/* jumpe */
	"ljne",		/* jumpn */
	/* Signed ST1 / ST 2*/
	"ljlte",		/* jumple */
	"ljlt",		/* jumpl */
	"ljgte",		/* jumpge */
	"ljgt",		/* jumpg */
	/* Unsigned ST0 / ST2 */
	"ljle"	,	/* jumple (jlequ) */
	"ljl",		/* jumpl (jlssu) */
	"ljhe",		/* jumpge (jgequ) */
	"ljh",		/* jumpg (jgtru) */
};


/*   printf conditional and unconditional branches */
void
cbgen(int o, int lab)
{
	if (o < EQ || o > UGT)
		comperr("bad conditional branch: %s", opst[o]);
	printf("%s	@" LABFMT "\n", ccbranches[o-EQ], lab);
}

#define	IS1CON(p) ((p)->n_op == ICON && getlval(p) == 1)

#if 0
/*
 * Move postfix operators to the next statement, unless they are 
 * within a function call or a branch.
 */
static void
cvtree(NODE *p, struct interpass *ip2)
{
	struct interpass *ip;
	NODE *q;

	if (callop(p->n_op) || p->n_op == CBRANCH)
		return;

	if ((p->n_op == PLUS || p->n_op == MINUS) &&
	    IS1CON(p->n_right) && (q = p->n_left)->n_op == ASSIGN &&
	    treecmp(q->n_left, q->n_right->n_left) &&
	    IS1CON(q->n_right->n_right)) {
		if ((p->n_op == PLUS && q->n_right->n_op == MINUS) ||
		    (p->n_op == MINUS && q->n_right->n_op == PLUS)) {
			nfree(p->n_right);
			*p = *q->n_left;
			if (optype(p->n_op) != LTYPE)
				p->n_left = tcopy(p->n_left);
			ip = ipnode(q);
			DLIST_INSERT_AFTER(ip2, ip, qelem);
			return;
		}
	}
	if (optype(p->n_op) == BITYPE)
		cvtree(p->n_right, ip2);
	if (optype(p->n_op) != LTYPE)
		cvtree(p->n_left, ip2);
}
#endif


static void
fixops(NODE *p, void *arg)
{
	static int fltwritten;

	if (!fltwritten && (p->n_type == FLOAT || p->n_type == DOUBLE)) {
		printf(".globl	fltused\n");
		fltwritten = 1;
	}
	switch (p->n_op) {
	/* Like the PDP-11 we have a BIC style  not and AND operation but even
	   more confusingly in our case we have a real AND for constants */
	case AND:
		/* Rewrite the non constant right hand side by
		   adding a complement operator or if one existed removing it */
		if (p->n_right->n_op != ICON) {
			if (p->n_right->n_op == COMPL) {
				NODE *q = p->n_right->n_left;
				nfree(p->n_right);
				p->n_right = q;
			} else
				p->n_right = mkunode(COMPL, p->n_right, 0, p->n_type);
		}
		break;
	}
}

void
myreader(struct interpass *ipole)
{
	struct interpass *ip;

#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader before\n");
		printip(ipole);
	}
#endif
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type != IP_NODE)
			continue;
		walkf(ip->ip_node, fixops, 0);
		canon(ip->ip_node); /* call it early */
	}
#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader middle\n");
		printip(ipole);
	}
#endif
#if 0
	DLIST_FOREACH(ip, ipole, qelem) {
		if (ip->type == IP_NODE)
			cvtree(ip->ip_node, ip);
	}
#endif
#ifdef PCC_DEBUG
	if (x2debug) {
		printf("myreader after\n");
		printip(ipole);
	}
#endif
}

/*
 * Remove SCONVs where the left node is an OREG with a smaller type.
 */
static void
delsconv(NODE *p, void *arg)
{
}

void
mycanon(NODE *p)
{
	walkf(p, delsconv, 0);
}

void
myoptim(struct interpass *ip)
{
}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	int ca = r[CLASSA];
	int cb = r[CLASSB];
	int cc = r[CLASSC];

	switch (c) {
	case CLASSA:
		/* Each class B register can cost us two class A */
		ca += 2 * cb;
		/* Class C can cost us two class A max */
		if (cc)
			ca += 2;
		return  ca < 10;	/* We have 10 class A */
	case CLASSB:
		/* Each class A can take out two class B (high and low) */
		cb += 2 * ca;
		/* Class C can take out another 1 (we have no R15,0) */
		if (cc)
			cb ++;
		return cb < 10;	/* We have 10 */
	case CLASSC:
		/* We can lose one class C register if any A or B are
		   allocated */
		if (ca || cb)
			cc++;
		return cc < 4;
	}
	return 0;
}

char *rnames[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"rp01", "rp12", "rp23", "rp34", "rp45", "rp56", "rp67", "rp78",
	"rp89", "rp910", "rp1011", "rp1112", "rp1213", "rp1314", "rp1415", "rp150",
	"fr0", "fr1", "fr2", "XXX", "XXX", "XXX", "XXX", "XXX",
	"XXX", "XXX", "XXX", "XXX", "XXX", "XXX", "XXX", "XXX",
	"ll0", "ll1", "ll2", "ll3"
};
 
/*
 * Return a class suitable for a specific type.
 */
int
gclass(TWORD t)
{
	if (t < LONG || t > BTMASK)
		return CLASSA;
	if (t == LONG || t == ULONG)
		return CLASSB;
	if (t == FLOAT || t == DOUBLE || t == LDOUBLE)
		return CLASSC;
	comperr("gclass");
	return CLASSD;
}

static int
argsiz(NODE *p)  
{
	TWORD t = p->n_type;

	if (t == LONG || t == ULONG || t == FLOAT)
		return 4;
	if (t == DOUBLE)
		return 8;
	if (t == STRTY || t == UNIONTY)
		return attr_find(p->n_ap, ATTR_P2STRUCT)->iarg(0);
	return 2;
}

/*
 * Argument specialties.
 */
void
lastcall(NODE *p)
{
	NODE *op = p;
	int size = 0;

	/*
	 * Calculate arg sizes.
	 */
	p->n_qual = 0;
	if (p->n_op != CALL && p->n_op != FORTCALL && p->n_op != STCALL)
		return;
	for (p = p->n_right; p->n_op == CM; p = p->n_left) {
		p->n_right->n_qual = 0;
		size += argsiz(p->n_right);
	}
	p->n_qual = 0;
	size += argsiz(p);
	p = op->n_right;

	if (p->n_op == CM)
		p = p->n_right;
#if 0 /* XXX fixme */
	if (p->n_type == FLOAT || p->n_type == DOUBLE ||
	    p->n_type == STRTY || p->n_type == UNIONTY)
		op->n_flags |= NLOCAL1;	/* Does not use stack slot */
	else
		op->n_flags &= ~NLOCAL1;
#endif
	op->n_qual = size; /* XXX */
}

static int
is1con(NODE *p)
{
	if (p->n_op == ICON && getlval(p) == 1)
		return 1;
	return 0;
}

/*
 * Special shapes.
 */
int
special(NODE *p, int shape)
{
	int o = p->n_op;

	switch (shape) {
	/* Matches for -2 and +2 in some cases.
	   for reasons I don't understand it does not always fire when
	   it ought to be provably 2 */
	case STWO:
		if (o == ICON && p->n_name[0] == 0 && getlval(p) == 2)
			return SRDIR;
		break;
	case SMTWO:
		if (o == ICON && p->n_name[0] == 0 && getlval(p) == -2)
			return SRDIR;
		break;
#if 0
	case SINCB: /* Check if subject for post-inc */
		if (p->n_op == ASSIGN && p->n_right->n_op == PLUS &&
		    treecmp(p->n_left, p->n_right->n_left) &&
		    is1con(p->n_right->n_right))
			return SRDIR;
		break;
#endif
	}
	return SRNOPE;
}

/*
 * Target-dependent command-line options.
 */
void
mflags(char *str)
{
}

/*
 * Do something target-dependent for xasm arguments.
 */
int
myxasm(struct interpass *ip, NODE *p)
{
	return 0;
}

int
fldexpand(NODE *p, int cookie, char **cp)
{
	return 0;
}
