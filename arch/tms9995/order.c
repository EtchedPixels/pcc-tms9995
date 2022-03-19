/*	$Id: order.c,v 1.13 2019/04/25 17:40:33 ragge Exp $	*/
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

#include <string.h>

int canaddr(NODE *);

/* is it legal to make an OREG or NAME entry which has an
 * offset of off, (from a register of r), if the
 * resulting thing had type t */
int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	/* R0 is special - it cannot be indexed with an offset */
	if (r == 0 && off)
		return 1;
	return(0);  /* YES */
}

/* FIXME: does not appear functional */  
static int
inctree(NODE *p)
{
	if (p->n_op == MINUS && p->n_left->n_op == ASSIGN && 
	    p->n_left->n_right->n_op == PLUS &&
	    treecmp(p->n_left->n_left, p->n_left->n_right->n_left) &&
	    p->n_right->n_op == ICON && getlval(p->n_right) == 1 &&
	    p->n_left->n_right->n_right->n_op == ICON &&
	    getlval(p->n_left->n_right->n_right) == 1) {
		/* post-increment by 1; (r0)+ */
		if (isreg(p->n_left->n_left)) /* Ignore if index not in reg */
			return 1;
	}
	return 0;
}

/*
 * Turn a UMUL-referenced node into OREG.
 * Be careful about register classes, this is a place where classes change.
 */
void
offstar(NODE *p, int shape)
{
	if (x2debug)
		printf("offstar(%p)\n", p);

	if (isreg(p))
		return; /* Is already OREG */

	if (p->n_op == UMUL) {
		if (p->n_type < PTR+LONG || p->n_type > PTR+ULONGLONG)
			p = p->n_left; /* double indexed umul */
	}
	if (inctree(p)) /* Do post-inc conversion */
		return;
#if 0
	/* This breaks for some reason I don't yet understand and leaves us
	   with an unresolved UMUL in the table processing */
	if( p->n_op == PLUS || p->n_op == MINUS ){
		if (p->n_right->n_op == ICON) {
			if (isreg(p->n_left) == 0)
				(void)geninsn(p->n_left, INAREG);
			/* Converted in ormake() */
			return;
		}
	}
#endif
	(void)geninsn(p, INAREG);
}

/*
 * Do the actual conversion of offstar-found OREGs into real OREGs.
 */
void
myormake(NODE *p)
{
	NODE *q = p->n_left;

	if (x2debug) {
		printf("myormake(%p)\n", p);
		fwalk(p, e2print, 0);
	}
	if (inctree(q)) {
		if (q->n_left->n_left->n_op == TEMP)
			return;
		p->n_op = OREG;
		setlval(p, 0); /* Add support for index offset */
		p->n_rval = R2PACK(regno(q->n_left->n_left), 0, 1);
		tfree(q);
		return;
	}
	if (q->n_op != OREG)
		return;
	if (TBLIDX(q->n_su))
		return; /* got sub-conversion, cannot convert */
	if (R2TEST(regno(q)))
		return; /* cannot convert anymore */
	if (p->n_type >= LONG && p->n_type <= ULONGLONG)
		return;
	p->n_op = OREG;
	setlval(p, getlval(q));
	p->n_rval = R2PACK(q->n_rval, 0, 0);
	if (x2debug) {
		printf("myormake end(%p)\n", p);
		fwalk(p, e2print, 0);
	}
	nfree(q);
}

/*
 * Shape matches for UMUL.  Cooperates with offstar().
 */
int
shumul(NODE *p, int shape)
{
	if (x2debug)
		printf("shumul(%p)\n", p);
	if (shape & SOREG)
		return SROREG;	/* Calls offstar */
	if (p->n_op == NAME && (shape & STARNM))
		return SRDIR;
	if (shape & STARREG)
		return SROREG;
	return SRNOPE;
}

/*
 * Rewrite operations on binary operators (like +, -, etc...).
 * Called as a result of table lookup.
 */
int
setbin(NODE *p)
{

	if (x2debug)
		printf("setbin(%p)\n", p);
	return 0;

}

/* setup for assignment operator */
int
setasg(NODE *p, int cookie)
{
	if (x2debug)
		printf("setasg(%p)\n", p);
	return(0);
}

/* setup for unary operator */
int
setuni(NODE *p, int cookie)
{
	return(0);
}

/*
 * Special handling of some instruction register allocation.
 */

/* General rules to pin shared 32bit helpers to a specific register group
   so we can generate nice tight helpers. Favour R01 for now */

static struct rspecial longfunc[] = {
	{NLEFT,  R23}, {NRIGHT, R01}, {NRES, R01}, { 0 }
};

static struct rspecial longfuncincdec[] = {
	{NLEFT,  R01}, {NRES, R01}, { 0 }
};

static struct rspecial longfunc1[] = {
	{NLEFT,  R01}, {NRIGHT, R2}, {NRES, R01}, {NEVER, R01}, {NEVER, R2}, { 0 } 
};

static struct rspecial longintfunc[] = {
	{NLEFT,  R01}, {NRIGHT, R2}, {NRES, R01}, { 0 }
};

static struct rspecial convlongfunc[] = {
	{NLEFT,  R0}, {NRES, R01}, { 0 }
};


struct rspecial *
nspecial(struct optab *q)
{
	switch (q->op) {
	case MUL:
		/* FIXME: this is a hack for now. The actual rule is that
		   reg x mam -> reg;reg+1 */
		if (q->visit == INAREG) {
			static struct rspecial s[] = {
			    { NLEFT, R0 }, { NEVER, R0 },{ NRES, R1 }, { 0 } };
			return s;
		} else if (q->visit == INBREG) {
			if (q->rshape == SCON)
				return longfunc1;
			return longfunc;
		}
		break;

	case DIV:
		/* Hack for now */
		if (q->visit == INAREG) {
			static struct rspecial s[] = {
			    { NORIGHT, R0 }, { NORIGHT, R1 }, { NLEFT, R1 }, { NRES, R0 }, { 0 } };
			return s;
		}
		else if (q->visit == INBREG) {
			if (q->rshape == SCON)
				return longfunc1;
			return longfunc;
		}
		break;

	case MOD:
		/* Hack for now */
		if (q->visit == INAREG) {
			static struct rspecial s[] = {
			    { NORIGHT, R0 }, { NORIGHT, R1 }, { NEVER, R0 }, { NLEFT, R1 }, { NRES, R1 }, { 0 } };
			return s;
		}
		else if (q->visit == INBREG) { 
			if (q->rshape == SCON)
				return longfuncincdec;
			return longfunc;
		}
		break;
	case PLUS:
	case MINUS:
		if (q->rshape == SONE || q->rshape == SCON)
			return longfuncincdec;
		if (q->visit == (INBREG|FOREFF))
			return longfunc;
		break;
	case UMINUS:
		if (q->visit == (INBREG|FOREFF))
			return longfunc1;
		break;
	case LS:
	case RS:
		if (q->lshape == SAREG || q->lshape == (SOREG|SNAME)) {
			/* The shift amount must be in R0 */
			static struct rspecial s[] = {
			    { NOLEFT, R0 }, { NEVER, R0 }, {NRIGHT, R0 }, { 0 } };
			return s;
		}
		if (q->visit == (INBREG | FOREFF)) {
			if (q->rshape == SCON)
				return longfunc1;
			return longintfunc;
		}
		break;
	case SCONV:
		/* u8 -> float/double */
		if (q->lshape == SAREG && q->visit == INCREG) {
			static struct rspecial s[] = {
			    { NLEFT, R0 }, { NRES, FR0 }, { 0 } };
			return s;
		}
		/* u32 -> float/double heler */
		if (q->lshape == SBREG && q->visit == INCREG) {
			static struct rspecial s[] = {
			    { NLEFT, R01 }, { NRES, FR0 }, { 0 } };
			return s;
		}
		/* u8/16 -> u32 */
		if (q->lshape == SAREG) {
			static struct rspecial s[] = {
			    { NEVER, R0 }, { NLEFT, R1 }, { NRES, R01 }, { 0 } };
			return s;
		}
		break;
	case STASG: {
			/* R0 = tmp counter. R1 source, R2 dest */
			static struct rspecial s[] = {
			    { NEVER, R0 }, { NEVER, R1 }, { NEVER, R2 },
			    { NRIGHT, R2 }, { NOLEFT, R2 }, { NOLEFT, R0 },
			    { 0 }
			};
			return s;
		}
		break;
	case STARG: {
			static struct rspecial s[] = {
			    { NEVER, R0 }, { NLEFT, R1 }, 
			    { NEVER, R2 }, { 0 }
			};
			return s;
		}
		break;
	} 
	comperr("nspecial entry %d", q - table);
	return 0; /* XXX gcc */
}

/*
 * Set evaluation order of a binary node if it differs from default.
 */
int
setorder(NODE *p)
{
	return 0; /* nothing differs on x86 */
}

/*
 * set registers in calling conventions live.
 */
int *
livecall(NODE *p)
{
	static int r[] = { -1 };

	return r;
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	return 1;
}
