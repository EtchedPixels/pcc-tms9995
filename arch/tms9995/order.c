/*	$Id: order.c,v 1.13 2019/04/25 17:40:33 ragge Exp $	*/
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

#include <string.h>

/*
 *	Feature sets
 */

unsigned m_has_divs = 1;		/* TMS9995 has divs, 9900 does not */

/*
 *	Can we addr this ?
 */
static int canaddr(NODE *p)
{
	int o = p->n_op;

	if (o==NAME || o==REG || o==ICON || o==OREG ||
	    (o==UMUL && shumul(p->n_left, SOREG|STARNM)))
		return(1);
	return(0);
}

/* is it legal to make an OREG or NAME entry which has an
 * offset of off, (from a register of r), if the
 * resulting thing had type t */
int
notoff(TWORD t, int r, CONSZ off, char *cp)
{
	/* R0 is special - it cannot be indexed with an offset.
          Subtle case: we cannot index a multi-word object at
          offset 0 as well generate a lower word reference at + 2 */
        if (r == 0 && off) //(off || szty(t) > 1))
		return 1;
	return(0);  /* YES */
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
	/* Remove any UMULs that are left over */
	if (p->n_op == UMUL && q->n_op == REG && getlval(q) == 0) {
		/* Here be dragons.. this might be *R0 for a 4 byte object */
		/* In that case we need to make up a subtree moving r0
		   into a temp and using that */
//		if (szty(p->n_type) == 1) {
			printf("; killed UMUL for %d\n", q->n_rval);
			p->n_op = OREG;
			p->n_rval = q->n_rval;
			p->n_name = q->n_name;
			p->n_left = NULL;	/* ??? */
			setlval(p, 0);
			tfree(q);
//		} else {
//			printf("HELP");
//		}
	}
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
   so we can generate nice tight helpers. Favour RP01 for now */

static struct rspecial longfunc[] = {
	{NLEFT,  RP01}, {NRIGHT, RP23}, {NRES, RP01}, { 0 }
};

static struct rspecial longfunconearg[] = {
	{NLEFT,  RP01}, { 0 }
};

static struct rspecial longfunconearg_shift[] = {
	{NLEFT,  RP01}, { NEVER, RP12 }, {NRES, RP01}, { 0 }
};

static struct rspecial longfunconeargusesr23[] = {
	{NLEFT,  RP01}, { NEVER, RP01 }, { NEVER, R2 }, {NRES, RP01}, { 0 }
};

static struct rspecial longfunc1[] = {
	{NLEFT,  RP01}, {NRIGHT, R2}, {NRES, RP01}, {NEVER, RP01}, {NEVER, R2}, { 0 }
};

static struct rspecial longintfunc[] = {
	{NLEFT,  RP01}, {NRIGHT, R2}, {NRES, RP01}, { 0 }
};

static struct rspecial convlongfunc[] = {
	{NLEFT,  R0}, {NRES, RP01}, { 0 }
};

static struct rspecial  floatops[] = {
	{ NLEFT, FR0 }, {NEVER, FR0}, { NORIGHT, FR0 }, { NRES, FR0 }, { 0, }
};


struct rspecial *
nspecial(struct optab *q)
{
	switch (q->op) {
	case UMUL:
		if (q->visit == INBREG || q->visit == INCREG) {
			static struct rspecial s[] = {
				{ NOLEFT, R0 }, { 0 }
			};
			return s;
		}
		break;
	case MUL:
		/* FIXME: this is a hack for now. The actual rule is that
		   reg x mam -> reg;reg+1 */
		if (q->visit == INAREG) {
			static struct rspecial s[] = {
			    { NLEFT, R0 }, { NEVER, R0 },{ NRES, R1 }, { 0 } };
			return s;
		} else if (q->visit == INBREG) {
			if (q->rshape == SCON)
				return longfunconearg;
			return longfunc;
		} else if (q->visit == INCREG) {
			return floatops;
		}
		break;

	case DIV:
		/* Hack for now */
		if (q->visit == INAREG) {
			/* Early machines lack mpys divs so use a helper */
			if (m_has_divs == 0) {
				static struct rspecial s[] = {
				    { NRIGHT, R2 }, { NLEFT, R1 }, { NEVER, R0 }, { NEVER, R2 }, { NRES, R1 }, { 0 } };
				return s;
			} else {
				static struct rspecial s[] = {
				    { NORIGHT, R0 }, { NORIGHT, R1 }, { NLEFT, R1 }, { NRES, R0 }, { 0 } };
				    return s;
			}
		} else if (q->visit == INBREG) {
			if (q->rshape == SCON)
				return longfunconeargusesr23;
			return longfunc;
                } else if (q->visit == INCREG) {
			return floatops;
		}
		break;

	case MOD:
		/* Hack for now */
		if (q->visit == INAREG) {
			/* Early machines lack mpys divs so use a helper */
			if (m_has_divs == 0) {
				static struct rspecial s[] = {
				    { NRIGHT, R2 }, { NLEFT, R1 }, { NEVER, R0 }, { NEVER, R2 }, { NRES, R1 }, { 0 } };
				return s;
			} else {
				static struct rspecial s[] = {
				    { NORIGHT, R0 }, { NORIGHT, R1 }, { NEVER, R0 }, { NLEFT, R1 }, { NRES, R1 }, { 0 } };
				return s;
			}
		}
		else if (q->visit == INBREG) { 
			if (q->rshape == SCON)
				return longfunconearg;
			return longfunc;
		}
		break;
	case PLUS:
	case MINUS:
		if (q->rshape == SONE || q->rshape == SCON)
			return longfunconearg;
		if (q->visit == (INBREG|FOREFF))
			return longfunc;
		if (q->visit == (INCREG|FOREFF|FORCC))
			return floatops;
		break;
	case UMINUS:
		if (q->visit == (INBREG|FOREFF))
			return longfunconearg;
		break;
	case LS:
	case RS:
		if (q->lshape == SAREG || q->lshape == (SOREG|SNAME)) {
			/* The shift amount must be in R0 */
			static struct rspecial s[] = {
			    {NRIGHT, R0 }, { NOLEFT, R0 }, { 0 } };
			return s;
		}
		if (q->visit == (INBREG | FOREFF)) {
			if (q->rshape == SCON)
				return longfunconearg_shift;
			return longintfunc;
		}
		break;
	case SCONV:
		/* u8/16 -> float/double */
		if (q->lshape == SAREG && q->visit == INCREG) {
			static struct rspecial s[] = {
			    { NLEFT, R1 }, { NRES, FR0 }, { 0 } };
			return s;
		}
		if (q->lshape == (SNAME|SOREG) && q->visit == INCREG) {
			static struct rspecial s[] = {
			    { NRES, FR0 }, { 0 } };
			return s;
		}
		/* u32 -> float/double heler */
		if (q->lshape == SBREG && q->visit == INCREG) {
			static struct rspecial s[] = {
			    { NRES, FR0 }, { 0 } };
			return s;
		}
		/* float to 32 */
		if (q->lshape == SCREG && q->visit == INBREG) {
			static struct rspecial s[] = {
			    { NLEFT, RP01 }, {NRES, FR0} };
			return s;
		}
		/* float to 16 */
		if (q->lshape == SCREG && q->visit == INBREG) {
			static struct rspecial s[] = {
			    { NLEFT, FR0 }, {NRES, R1} };
			return s;
		}
		/* u8/16 -> u32 */
		if (q->lshape == SAREG) {
			static struct rspecial s[] = {
			  { NEVER, R0 }, { NEVER, R1 },
			  { NLEFT, R1 }, {NORIGHT, RP01}, { NRES, RP01 }, { 0 } };
			return s;
		}
		if (q->lshape & SAREG) {
			static struct rspecial s[] = {
			{ NEVER, R0 }, { NEVER, R1, }, 
			{ NRIGHT, RP01 }, { NRES, RP01 }, { 0 } };
			return s;
		}
	
		break;
	case OPLTYPE:
		if (q->visit == INCREG) {
			static struct rspecial  s[] = {
			    { NLEFT, FR0 }, { NRES, FR0 }, { 0, } };
			return s;
		}
		break;
	case OPLOG:
		if (q->visit == FORCC && q->rshape == SCREG) {
			static struct rspecial  s[] = {
			    { NLEFT, FR0 }, { NORIGHT, FR0 }, { NEVER, FR0 }, { 0, } };
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
	return 0; /* nothing differs on tms9995 */
}

/*
 * set registers in calling conventions live.
 *
 * In our case we may have a couple of live registers. We walk the argument
 * list looking for arguments that are a subtree which assigns to a register.
 * These are our register arguments. Any other arguments will be FUNARG
 * nodes that push data.
 */
int *
livecall(NODE *p)
{
	static int r[8];
	int *s = r;

	*s = -1;
	if (p->n_op == UCALL || p->n_op == UFORTCALL || p->n_op == USTCALL ||
	    p->n_op == FORTCALL)
		return r;
	for (p = p->n_right; p->n_op == CM; p = p->n_left) {
		if (p->n_right->n_op == ASSIGN &&
		    p->n_right->n_left->n_op == REG)
			*s++ = p->n_right->n_left->n_rval;
	}
	if (p->n_op == ASSIGN &&
	    p->n_left->n_op == REG)
		*s++ = p->n_left->n_rval;
	*s = -1;
	return r;
}

/*
 * Signal whether the instruction is acceptable for this target.
 */
int
acceptable(struct optab *op)
{
	/* Fall back to helper calls on the 9900 if we are doing signed
	   division 16bit with divs */
	if (m_has_divs == 0) {
		if ((op->op == DIV || op->op == MOD) && op->visit == INAREG &&
			op->rshape == (SAREG|SNAME|SOREG))
			return 0;
	}
	return 1;
}
