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

/*
 * Print out assembler segment name.
 */
void
setseg(int seg, char *name)
{
	switch (seg) {
	case PROG: name = ".code"; break;
	case STRNG:
	/* FIXME */
	case PICDATA:
		name = ".picdata"; break;
	case PICRDATA:
		name = ".picrdata"; break;
	case PICLDATA:
		name = ".picldata"; break;
	case RDATA:
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
 * code for the beginning of a function; a is an array of
 * indices in symtab for the arguments; n is the number
 */
void
bfcode(struct symtab **sp, int cnt)
{
	struct symtab *sp2;
	NODE *n;
	int i;

	/* adjust the offset for bytewide objects. We always push them
	   16bit to keep stack alignment (and also deal with int promotion
	   rules), which means the value is 1 byte further in */
	for (i = 0; i < cnt ; i++) {
		if (sp[i]->stype == CHAR || sp[i]->stype == UCHAR)
			sp[i]->soffset += SZCHAR;
	}

	if (xtemps == 0)
		return;

	/* put arguments in temporaries */
	for (i = 0; i < cnt; i++) {
		if (sp[i]->stype == STRTY || sp[i]->stype == UNIONTY ||
		    cisreg(sp[i]->stype) == 0)
			continue;
		sp2 = sp[i];
		n = tempnode(0, sp[i]->stype, sp[i]->sdf, sp[i]->sap);
		n = buildtree(ASSIGN, n, nametree(sp2));
		sp[i]->soffset = regno(n->n_left);
		sp[i]->sflags |= STNODE;
		ecomp(n);
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
}

void
bjobcode(void)
{
	extern char *asspace;
	asspace = ".ds"; /* .ds, not .space */
}

/*
 * Helper to insert typecasts on a function argument
 */

static NODE *promote_arg(NODE *r)
{
	unsigned t = UNSIGNED;
	NODE *n;

	if (szty(r->n_type) != 1)
		return r;
	if (r->n_type == CHAR)
		t = INT;

	n = block(SCONV, r, NIL, t, r->n_df, r->n_ap);
	return n;
}

/*
 * Called with a function call with arguments as argument.
 * This is done early in buildtree() and only done once.
 * Returns p.
 */

NODE *
funcode(NODE *p)
{
	NODE *r, *l;

	/* Fix function call arguments:
		add FUNARG
		turn any byte sized pushes into int to deal with both
			promotion rules and stack alignment

		The second half is matched by bfcode which adjusts
		the corresponding argument stack offsets to match the
		result of the typecasting
	*/
	for (r = p->n_right; r->n_op == CM; r = r->n_left) {
		if (r->n_right->n_op != STARG) {
			r->n_right = promote_arg(r->n_right);
			r->n_right = block(FUNARG, r->n_right, NIL,
			    r->n_right->n_type, r->n_right->n_df,
			    r->n_right->n_ap);
		}
	}
	if (r->n_op != STARG) {
		l = talloc();
		*l = *r;
		r->n_op = FUNARG;
		r->n_left = promote_arg(l);
		r->n_type = r->n_left->n_type;
	}
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
