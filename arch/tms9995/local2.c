/*	$Id: local2.c,v 1.20 2019/04/27 20:35:52 ragge Exp $	*/
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

static int spcoff;
static int argsiz(NODE *p);
static void negcon(FILE *fp, int con);

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
		printf(".globl	%s\n", ipp->ipp_name);
	printf("%s:\n", ipp->ipp_name);
#endif
	printf("dect	r6\n");
	printf("mov	r11,*r6\n");
	printf("dect	r6\n");
	printf("mov	r7,*r6\n");

	/* Allow for the frame pointer and r11 save */
	addto = p2maxautooff - 2;
	if (addto & 1)
		addto++;

	/* We juggle these around a bit so that r7 is pointing to the
	   first value not to the saved old r7. This makes our first
	   stacked value accessible via *r7 which saves us a word (and
	   a bunch of cycles) a reference */

	if (addto == 2) {
		printf("dect	r6\n");
		/* Set up frame pointer */
		printf("mov	r6, r7\n");
	} else {
		/* Set up frame pointer */
		printf("mov	r6, r7\n");
		if (addto > 0)
			printf("ai	%d,r6\n", -addto);
		printf("dect	r7\n");
	}

	/* Save old register variables below the frame */
	for (i = 0; i < MAXREGS; i++)
		if (TESTBIT(p2env.p_regs, i))
			printf("dect	r6\nmov	%s,*r6\n",
				rnames[i]);
	spcoff = 0;
}

void
eoftn(struct interpass_prolog *ipp)
{
	int i;
	if (spcoff)
		comperr("spcoff == %d", spcoff);
	if (ipp->ipp_ip.ip_lbl == 0)
		return; /* no code needs to be generated */

	/* our registers should be top of stack */
	for (i = 0; i < MAXREGS; i++) {
		if (TESTBIT(p2env.p_regs, i))
			printf("mov	*r6+, %s\n", rnames[i]);
	}

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

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

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
	expand(p, 0, "c	AL,AR\n");
	if (cb1) cbgen(cb1, s);
	if (cb2) cbgen(cb2, e);
        expand(p, 0, "c	UL,UR\n");
        cbgen(u, e);
        deflab(s);
}

void
zzzcode(NODE *p, int c)
{
	struct attr *ap;
	NODE *l;
	char *s;
	int o;
	int len;

	switch (c) {
	case 'A': /* Adust sp for argument push */
		spcoff += argsiz(p);
		break;
	case 'C': /* subtract stack after call */
		spcoff -= p->n_qual;
		if (p->n_qual == 2)
			printf("inct	r6\n");
		else if (p->n_qual > 2)
			printf("ai	%d,r6\n", (int)p->n_qual);
		break;
	case 'F': /* long comparision */
		twolcomp(p);
		break;
#if 0
	case 'G': /* printout a subnode for post-inc */
		adrput(stdout, p->n_left->n_left);
		break;

	case 'H': /* arg with post-inc */
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
					printf("mov	%s,r1\n", rnames[r]);
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
			printf("dect	r6");
		else
			printf("ai	r6, %d", -o);
		printf("mov	r6,r2\n");
		printf("li	r0, %d\n", o >> 1);
		deflab(o);
		printf("mov	*r1+, *r2+\n");
		printf("dec	r0");
		printf("jne	" LABFMT "\n", o);
		spcoff += argsiz(p);
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

int canaddr(NODE *p)
{
	int o = p->n_op;

	if (o==NAME || o==REG || o==ICON || o==OREG ||
	    (o==UMUL && shumul(p->n_left, SOREG|STARNM)))
		return(1);
	return(0);
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
	printf("$" CONFMT, val);
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
		setlval(p, getlval(p) + size);
		adrput(stdout, p);
		setlval(p, getlval(p) - size);
		break;
	case REG:
		printf("r%c", rnames[p->n_rval][1]);
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
				fprintf(io, "+%d", (int)(getlval(p) & 0177777));
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
			fprintf(io, "(%s)", rnames[p->n_rval]);
		else
			fprintf(io, "*%s", rnames[p->n_rval]);
		return;
	case ICON:
		/* We will need to do some special handling for literals
		   when we get to it */
		conput(io, p);
		return;

	case REG:
		switch (p->n_type) {
		case LONG:
		case ULONG:
			fprintf(io, "r%c", rnames[p->n_rval][2]);
			break;
		default:
			fprintf(io, "%s", rnames[p->n_rval]);
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
	printf("%s	" LABFMT "\n", ccbranches[o-EQ], lab);
}

#define	IS1CON(p) ((p)->n_op == ICON && getlval(p) == 1)

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

static void
fixops(NODE *p, void *arg)
{
	static int fltwritten;
	NODE *r;

	if (!fltwritten && (p->n_type == FLOAT || p->n_type == DOUBLE)) {
		printf(".globl	fltused\n");
		fltwritten = 1;
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

void
rmove(int s, int d, TWORD t)
{
	if (t < LONG || t > BTMASK) {
		printf("mov	%s,%s\n", rnames[s],rnames[d]);
	} else if (t == LONG || t == ULONG) {
		/* avoid trashing double regs */
		if (d > s)
			printf("mov	r%c,r%c\nmov	r%c,r%c\n",
			    rnames[s][2],rnames[d][2],
			    rnames[s][1],rnames[d][1]);
		else
			printf("mov	r%c,r%c\nmov	r%c,r%c\n",
			    rnames[s][1],rnames[d][1],
			    rnames[s][2],rnames[d][2]);
	} else if (t == FLOAT || t == DOUBLE) {
		printf("movf	%s,%s\n", rnames[s],rnames[d]);
	} else
		comperr("bad float rmove: %d %d %x", s, d, t);

}

/*
 * For class c, find worst-case displacement of the number of
 * registers in the array r[] indexed by class.
 */
int
COLORMAP(int c, int *r)
{
	switch (c) {
	case CLASSA:
		return (r[CLASSB] * 2 + r[CLASSA]) < 5;
	case CLASSB:
		if (r[CLASSB] > 1) return 0;
		if (r[CLASSB] == 1 && r[CLASSA] > 0) return 0;
		if (r[CLASSA] > 2) return 0;
		return 1;
	case CLASSC:
		return r[CLASSC] < 4;
	}
	return 0;
}

char *rnames[] = {
	"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
	"r01", "r12", "r23", "r34", "XXX", "XXX", "XXX", "XXX",
	"fr0", "XXX", "XXX", "XXX", "XXX", "XXX", "XXX", "XXX",
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
	 * Mark first arg not to have - before it.
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
	case SINCB: /* Check if subject for post-inc */
		if (p->n_op == ASSIGN && p->n_right->n_op == PLUS &&
		    treecmp(p->n_left, p->n_right->n_left) &&
		    is1con(p->n_right->n_right))
			return SRDIR;
		break;
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
