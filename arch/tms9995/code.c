/*	$Id: code.c,v 1.17 2019/04/25 17:39:23 ragge Exp $	*/
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


# include "pass1.h"

#ifndef LANG_CXX
#define NODE P1ND
#undef NIL
#define NIL NULL
#define	talloc p1alloc
#endif

unsigned m_discard = 0;

/*
 * Print out assembler segment name.
 */
void
setseg(int seg, char *name)
{
	switch (seg) {
	case PROG:
	case RDATA:
		if (m_discard)
			name = ".discard";
		else
			name = ".code";
		break;
	case STRNG:
	/* FIXME */
	case PICDATA:
		name = ".picdata"; break;
	case PICRDATA:
		name = ".picrdata"; break;
	case PICLDATA:
		name = ".picldata"; break;
	case DATA:
	case LDATA: name = ".data"; break;
	case UDATA: break;
	default:
		cerror("setseg");
	}
	printf("\t%s\n", name);
}

void
defalign(int al)
{
	if (al > ALCHAR)
		printf(".even\n");
}

/*
 * Define everything needed to print out some data (or text).
 * This means segment, alignment, visibility, etc.
 */
void
defloc(struct symtab *sp)
{
	static char *loctbl[] = { "text", "data", "data" };
	TWORD t;
	char *n;
	int s;

	t = sp->stype;
	s = ISFTN(t) ? PROG : ISCON(cqual(t, sp->squal)) ? RDATA : DATA;
	if (s != lastloc)
		printf("	.%s\n", loctbl[s]);
	lastloc = s;
	n = getexname(sp);
	if (sp->sclass == EXTDEF)
		printf("	.export %s\n", n);
	if (sp->slevel == 0) {
		printf("%s:\n", n);
	} else {
		printf(LABFMT ":\n", sp->soffset);
	}
}

/*
 * Code for the end of a function. Deals with struct return here.
 * On pdp11 we use a static bounce-buffer for struct return, 
 * which address is returned in r0.
 */
void
efcode(void)
{
	struct symtab *sp;
	NODE *p, *q;

	if (cftnsp->stype != STRTY+FTN && cftnsp->stype != UNIONTY+FTN)
		return;

	/* Handcraft a static buffer */
	sp = getsymtab("007", SSTMT);
	sp->stype = DECREF(cftnsp->stype);
	sp->sdf = cftnsp->sdf;
	sp->sap = cftnsp->sap;
	sp->sclass = STATIC;
	sp->soffset = getlab();
	sp->slevel = 1;
	defzero(sp);

	/* create struct assignment */
	q = nametree(sp);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(UMUL, p, NIL);
	p = buildtree(ASSIGN, q, p);
	ecomp(p);

	/* return address of buffer */
	q = buildtree(ADDROF, nametree(sp), NIL);
	p = block(REG, NIL, NIL, PTR+STRTY, 0, cftnsp->sap);
	p = buildtree(ASSIGN, p, q);
	ecomp(p);
}

/*
 * Helper to insert typecasts on a function argument
 */

static NODE *promote_arg(NODE *r)
{
	unsigned t = UNSIGNED;
	NODE *n;

	if (r->n_type == UCHAR)
		t = UNSIGNED;
	else if (r->n_type == CHAR)
		t = INT;
	else
		return r;

	n = block(SCONV, r, NIL, t, r->n_df, r->n_ap);
	return n;
}

/* Manage the argument register allocation rules in one place */
static unsigned regnum;
static unsigned regstop;

#define REGSTART	R4
#define REGEND		R5

/* Start register argument allocation */
static void reg_arg_init(void)
{
	regnum = REGSTART;
	regstop = 0;
}

/* Allocate a register argument and return the register, or 0 if none */
static unsigned reg_arg_alloc(unsigned type)
{
	int sz = szty(type);

	if (regstop)
		return 0;

	if (regnum + sz > REGEND + 1) {
		regstop = 1;
		return 0;
	}
	/* For now just do integer/char types */
	if (sz == 2) {
		regnum += 2;
		return RP45;
	}
	if (sz > 1) {
		regstop = 1;
		return 0;
	}
	return regnum++;
}

/* End register usage in this argument set */
static void reg_arg_end(void)
{
	regstop = 1;
}

static unsigned int reg_arg_shift(void)
{
	/* Return the number of words to shift the arguments by to allow
	   for those that were registerized */
	return regnum - REGSTART;
}

/*
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **sp, int cnt)
{
	NODE *p, *q;
	int i, n;
	union arglist *usym;

	is_va = 0;

	reg_arg_init();

        /*
         * Detect if this function has ellipses and save in lastchance
         * for pass 2
         */
        usym = cftnsp->sdf->dfun;
        while (usym && usym->type != TNULL) {
                if (usym->type == TELLIPSIS) {
                        is_va = 1;
			reg_arg_end();
                        break;
                }
                ++usym;
        }
        /* For now just handle integer types. Pairs are a bit more painful */

	/* recalculate the arg offset and create TEMP moves */
	for (i = 0; i < cnt; i++) {
		if ((n = reg_arg_alloc(sp[i]->stype)) != 0) {
			p = tempnode(0, sp[i]->stype, sp[i]->sdf, sp[i]->sap);
			q = block(REG, NIL, NIL,
			    sp[i]->stype, sp[i]->sdf, sp[i]->sap);
			q->n_rval = n;	/* R4 / R5 */
			/* FIXME: need to build a subtree for type conv for
			   char/uchar that were passed into.. at least until
			   that bodge can be removed */
			q = promote_arg(q);
			p = buildtree(ASSIGN, p, q);
			sp[i]->soffset = regno(p->n_left);
			sp[i]->sflags |= STNODE;
			ecomp(p);
		} else {
			/* Adjust the stack offset to allow for the previous
			   register variables */
			sp[i]->soffset -= SZINT * reg_arg_shift();
		        /* adjust the offset for bytewide objects. We always push them
			   16bit to keep stack alignment (and also deal with int promotion
			   rules), which means the value is 1 byte further in */
			if (sp[i]->stype == CHAR || sp[i]->stype == UCHAR)
				sp[i]->soffset += SZCHAR;
			if (xtemps) {
				/* put stack args in temps if optimizing */
				p = tempnode(0, sp[i]->stype,
				    sp[i]->sdf, sp[i]->sap);
				p = buildtree(ASSIGN, p, nametree(sp[i]));
				sp[i]->soffset = regno(p->n_left);
				sp[i]->sflags |= STNODE;
				ecomp(p);
			}
		}
	}
}


/* called just before final exit */
/* flag is 1 if errors, 0 if none */
void
ejobcode(int flag)
{
	/* Make sure the object module is even aligned */
	printf("	.code\n");
	printf("	.even\n");
	printf("	.data\n");
	printf("	.even\n");
	printf("	.bss\n");
	printf("	.even\n");
	printf("	.discard\n");
	printf("	.even\n");
}

void
bjobcode(void)
{
	extern char *asspace;
	asspace = ".ds"; /* .ds, not .space */
}

/*
 * Make a register node, helper for funcode.
 */
static NODE *
mkreg(NODE *p, int n)
{
	NODE *r;

	r = block(REG, NIL, NIL, p->n_type, p->n_df, p->n_ap);
	r->n_rval = n;
	return r;
}

/*
 * Move args to registers and emit expressions bottom-up. This needs to
 * match the behaviour (register and classes) expected by bfcode.
 */
static void
fixargs(NODE *p)
{
	NODE *r;
	unsigned int n;

	if (p->n_op == CM) {
		fixargs(p->n_left);
		r = p->n_right;
		/* For now leave the hard stuff with pairs and structs */
		if (r->n_op == STARG) {
			reg_arg_end();
			return;
		}
		n = reg_arg_alloc(r->n_type);
		if (n)
			p->n_right = buildtree(ASSIGN, mkreg(r, n), r);
		else {
			r = promote_arg(r);
			p->n_right = block(FUNARG, r, NIL, r->n_type,
			    r->n_df, r->n_ap);
		}
		return;
	}
	if (p->n_op == STARG) {
		reg_arg_end();
		return;
	}
	n = reg_arg_alloc(p->n_type);
	if (n) {
		r = talloc();
		*r = *p;
		r = buildtree(ASSIGN, mkreg(r, n), promote_arg(r));
		*p = *r;
		p1nfree(r);
	} else {
		r = talloc();
		*r = *p;
		p->n_op = FUNARG;
		p->n_left = r;
		p->n_left = promote_arg(p->n_left);
		p->n_type = p->n_left->n_type;
	}
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 *
 * FIXME: need to identify vararg calls.
 */

NODE *
funcode(NODE *p)
{
	reg_arg_init();
	fixargs(p->n_right);
	return p;
}

/* fix up type of field p */
void
fldty(struct symtab *p)
{
}

/*
 * XXX - fix genswitch.
 */
int
mygenswitch(int num, TWORD type, struct swents **p, int n)
{
	return 0;
}

/*
 * Return "canonical frame address".
 */
NODE *
builtin_cfa(const struct bitable *bt, NODE *a)
{
	uerror(__func__);
	return bcon(0);
}

NODE *
builtin_return_address(const struct bitable *bt, NODE *a)
{
	uerror(__func__);
	return bcon(0);
}

NODE *
builtin_frame_address(const struct bitable *bt, NODE *a)
{
	uerror(__func__);
	return bcon(0);
}
