# An Introduction To PCC Targetting

Or "my notes whilst tring to do it"

## Conceptual Model

The first pass of the compiler turns C code blocks into trees. These trees
then receive both architecture independent and architecture specific
optimizations. The code generator then uses a supplied rule table, register
rules and helper functions to turn this into target assembler.

As it processes the rules the compiler also understands how to extrapolate
many missing instruction forms. For example given only a two register add
instruction it will embroider the tree with the neceessary moves in order
to access and store data not in registers. Likewise given an indirect
address based form and no constant form it will place constants in the
program data and reference them by address.

The code generator understands both two and three register instruction
models and knows how to try and optimize for two register machines (that is
where one of the inputs is the output).

### Classes and Registers

This is the heart of the automatic register assignment. The compiler handles
all register allocation using provided rules and tables. All the allocation,
movement and spilling of registers is handled for you.

The compiler assigns things based upon a class. Each base C type in the
system (plus pointer) is mmapped to a class using a rule provided when
targetting the compiler. A given type must map to a specific class, but many
types can map to one class.

For example a processor with eight 32bit registers might well have only a
single class for char, short, integer, pointer, float and maybe even long
types. The different types that live within a class do not need to have the
same bit representation. Conversions are generated between the types and
can do any needed masking or sign extension if necessary. In many cases (for
example a processor with 32bit register operations plus byte and word sized
load/store memory) they may all be blank.

The compiler also distinguishes between pointers and, although rare except
on old machines, can cope with things like special character pointers.

There are two usual reasons to put something into a different class. The first
is for types that have physically separate registers. For example some
processors have entirely separate floating point registers and operators.
The second is when you need to combine registers for larger types. Thus our
simple 32bit processor might use pairs of registers for 64bit types and have
a second class of virtual registers made of pairs of the 32bit class.

The compiler is quite weak at this, and does not have a way to express a
class as "any two members of another" nor to express in-situ conversions
where for example a register pair is turned from 64 to 32bit by simple
aliasing one of the registers. Instead explicit pairs must be described.
That may be arbitrary pairs, or reflect any instruction set rules such as
hardware 64bit operations using even register pairs.

Registers need not be allocated to a specific class. The machine description
also specifies which registers in a given class clash with those in another.
Thus register r0 could be a 32bit register in CLASSA, and half of a register
in a 64bit CLASSB. The compiler will use the overlap table when generating
register assignments.

Instruction asymmetries are normally best handled outside of classes. The
rule tables described later allow the description of input and output rules
for a given instruction. Thus if a specific register is needed as a shift
counter this can be expressed in a rule. Similarly it is possible to specify
the range of register + offset allowed for a pointer dereference based
entirely upon the register and type.

It is possible for some registers to be memory locations if this is handled
by the rules correctly. For some architectures this may be necessary if
there are not enough real registers.

### Stack

The machine has a stack upon which stack frames may be placed. The current
frame is indexed by a frame pointer. Frames are generated in favour of
spills via push and pop. It is possible for a platform to track the stack
pointer and make the frame pointer just a software concept. Note that
elimiation of a frame pointer in this way prevents supporting alloca().

### Call and Return

Function calls and returns are handled by rule tables as is the placement of
arguments and conversion of them into more basic types for access. This
allows register based, stack based and other conventions. Except for
structs functions are assumed to return a value in a register in the class
for that type.

## Machine Definition (macdefs.h)

### Definitions

makecc is a helper to turn multi-byte characters into a single word. This
doesn't normally need touching unless you have a word based machine or
strange character bit lengths.

ARGINIT is the number of *bits* above the frame pointer where the arguments
start. A typical platform has to push the return address and the old frame
pointer above the arguments, so would need perhaps 64bits.

AUTOINIT is the same bias value for local variables.

SZCHAR, SZBOOL and friends give the size (in bits) of these types. They do
not need to be 8bit types as the compiler can support things like 18 or 36
bit machines just fine. Note that the pointer version takes the underlying
type as an argument for the same of platforms that need special character
pointers.

ALxxxx for each type gives the alignment constraint for this type, again in
bits

MIN/MAX_xxxx define the range of values that can be stored in this type.

CHAR_UNSIGNED selects unsigned character by default

BOOL_TYPE gives the type used to store booleans. This should be picked based
upon performance and size.

CONSZ, U_CONSZ, OFFZ hold types large enough to hold the biggest constants,
offsets etc. Usually nowdays this is just defined to use long long, and
unsigned long long for the U_CONSZ unsigned constant type.

CONFMT is the matching print formatting for CONSZ, so typically "%lld"

LABFMT is the printing type for the numeric labels the compiler generates
for internal branching. If you are using _ in front of actual symbol names
then it can be something like "L%d", if not then it needs to be something
that cannot clash with a C language name (eg ".L%d") depending upon the
platform.

STACK_DOWN sets a downward growing stack. If not then the stack is assumed
to grow upwards

FIELDOPS can be defined if the processor has hardware bit field operations.
If not the compiler will handle them itself.

TARGET_ENDIAN can be defined as TARGET_BE or TARGET_LE for big or little
endian

MYINSTRING can be defined to indicate that the default string printing
function in the compiler should not be used. Instead the architecture
provides void instring(struct symtab *s), which prints out the data.

MYALIGN causes the compiler to call defalign provided by the target rather
than the default printing function.

BYTEOFF(x) returns true if the value is byte aligned. It is used solely
for the SWADD shape on some platforms to ensure you dont word add bytes.

wdal(x) is true if x is word aligned.

STOARG(p), STOFARG(p) and STOSTARG(p)  do not appear to be used any more.

FINDMOPS can be defined if the processor has operations on memory and
benefits from things like in situ register allocations (for example if it
has add reg, const type instructions rather than being a 3 register
machine).

printdotfile is called to print the file name of a given module into the
assembly source. It can simply be defined as printdotfile(x) if not needed
or as a call to something if it is.

szty(t) returns the size of t in target machine characters.

### Registers And Classes

The register and class are encoded as a small integer according to the
number of registers in the largest class. Thus a machine with a largest group
of 8 registers will put the register number in the low 3 bits and the class in
the bits above.

The actual register naming doesn't matter, but these values will be used
over the various pieces of arch code so pick wisely. Most targets name them
to reflect the hardware names.

MAXREGS is the total number of registers remembering that they are spaced
out by class. In other words it's one higher than the biggest register
number assigned.

RSTATUS is an array indexed by register giving the class and temporary/permanent
properties of the register. SAREG/SBREG etc indicate the class of the
register whilst TEMPREG indicates it may be used by a rule that asks for
a temporary of this class, and PERMREG identifies that it will be saved
and restored if used by a called function. PERMREG is used for register
variable allocation, so if PERMREG is set functions must save and
restore the relevant registers. 0 indicates an unusable or unused register.

Note that the frame pointer, any program counter and stack pointer are not
counted in the registers that are allocatable. That is they should have a 0
entry in their class if present in a class.

ROVERLAP lists which registers from other classes clash. Thus if the first
two registers overlapped the first floating point register you might have
an entry like { FR0, -1 }, and likewise in the FR0 slot for {R0, R1, -1 }.

Registers that do not clash simply have a { -1 } entry.

PCLASS(p) returns the register class for a given node. Usually this is done
by the type (p->n_type). It returns SAREG, SBREG etc according to the class
to us for this type.

NUMCLASS gives the number of classes actually used.

RETREG(x) returns the register to use for the return value of type x. The
register must be in the correct class.

FPREG is a define which tells the compiler the register name to use for the
frame pointer.

SPREG is a define which tells the compiler the register name to use for the
stack pointer.

TODO COLORMAP/FDFLOAT S*

Registers are allocated low to high within a class. Thus if there are higher
cost or less functional registers you want to minimise use of they should be
highest numbered in the logical numbering of the class.

The minimum size of a class appears to be two. Having one item in the class
causes some operations to allocate a bogus non existent register and then
get confused and die when trying to evaluate things like x + 1 / y in that
class.

## Tables

The tables describe in generic terms a set of operations with constraints
and results. The compiler uses these to work out much of the code
generation.

Tables and much of the compiler are built around the notion of shapes. A
shape describes what/where something is. The main shapes you will see in
rules are

SZERO: a constant zero
SONE: a constant one
SCON: a constant
SNAME: a symbolic name or address possibly with a constant offset. Generally
for globals and statics
SOREG: a register dereference, possibly with an offset
SAREG/SBREG/...: a value loaded into a register of this class.

Architecture specific code can also generate additional types to fit the
matching needs of the platform.

For something to match it must have a shape and type matching the shape
and type masks in the rule.

The table entries are laid of as

````
{	op,	visit,
	left shape, left types
	right shape, right types
	needs, rewrite
		string
}
````

op is the type of operation the rule matches. These are documented below.
left shape and left types describe the shape and types that the left hand
node below this one, right shape and right types for the match of the
right hand node. 

visit indicates the reasons to use this node. INAxRG indicates that it puts
the result into that register class, FOREFF indicates it has a side effect
and FORCC indicates it sets all the condition codes. (There is no easy way
to deal with instructions that set only partial codes other than by saying
the don't generate them and generating extra comparisons).

The types and shapes are mask bases so for example TLONG|TULONG matches a
long or unsigned long.

The needs describes the requirement properties of the rule. Most commonly
these are

NAREG, NBREG, one or more times indicates that a number of additional
registers in this class are needed beyond those implied by the rule.

NxSL: May share left register between input and output
NxSR: May share right register between input and output

NSPECIAL: The nspecial() function will be called and the target code can
provide specific rules for register management. This is used for
instructions that have more complex requirements than class membership, such
as placing a result in a particular register. It can also be useful to force
register allocation when invoking helper functions.

The usual rewrite rules are

RLEFT - the result ends up in the left hand variable. Also implicitly
clobber the left hand value. Allows an implicit move to be inserted
to get the value needed into the left hand side in order to use a two
argument operation like "add reg1 to reg2"
RDEST - used for ASSIGN and STASG
RESCn - the result ends up in the nth allocated register
RESCC - the result ends up in the condition codes ???

and are used for reclamation.

The string itself is parsed and upper case letters are substituted. The
substitutions are:

A	The address of an object (via adrput)
Bx	The byte offset within a word of x
Cx	Constant value
F	this line should be deleted if FOREFF is active
Ix	instruction X
L	output a label field
LC	same for a compiler internal label
O	opcode. The target hopcode() methid provides an opcode
Ux	The upper half of an addreess (via upput)

For those taking a second character the values that may be used are

1..3	the result n
D	the result 0
L	the left hand (so AL is address left node)
R	the right hand


Bit field specific substitutions are

H	the shift of a bit field
M	the mask of a bit field
N	the complement of the mask of a bit field
S	the size of a bit field

Rules are matched in order, with priority given to the earliest match.

#### Debugging Table Entries

mkext does an initial sanity check of the table during the build. It can
also invoked after it is built with an entry number and will print the text
of that table entry. This helps figure out the various "I broke in table
entry 114" type errors.

There are classes of rule designed to make certain tables simpler
particularly when combined with the 'O' expansion (see below). This allows
one set of rules to be written for a group of similar operations.

### PCONV

These are conversions for pointer ypes. In most cases these are no-operations
but they can do work when for example addresses need to be shifted.

### SCONV

Shape conversions. These move things between different types and class. When
an object is moved between classes the rule must include an NxREG, RESC1
pair or the compiler will get its types in a knot and confused. It is not
possible to relialy move an object in situ between two classes using an
overlapping register.

### CALL/UCALL

Function calls. There is one entry for each return type (left side) both for
constant and pointer calls (indirect function calls), and one on the right
for the type of the return.

UCALL matches functions that take no arguments. CALL functions that do take
arguments. USTCALL is used for functions that return large objects
(structures or unions).

### PLUS

Rules for addition of values within a class. The constant (if any) goes on
the right hand side.

### MINUS

As PLUS but for subtraction. The result is left minus right.

### LS

Left shift. This needs to include the fact there may be shifts by an integer
and not just a constant. There is no shift operation for floating point
types.

### RS

Much the same as a left shift but will require different entries for signed
and unsigned shifts.

### ASSIGN

Assign a value to a register or other object. The object being assigned to
is on the left the value to assign it to is on the right. As usual any type
casts have already been done so the ASSIGN does not need to worry about
mismatched types.

### STASG

Assign a structure to another. Somewhat of a special case.
TODO

### MUL

Multiply left by right. Any constant will be on the right. Because the
compiler default behaviour is to promote arguments first there is no
simple table driven way to tell it you have a 16bit x 16bit to 32bit
multiply.

### DIV

Whilst this works the same way as MUL it isn't commutative so either side
could be constant. In addition sign matters for division so the correct
instructions must be used for each type. Where a mix of signed and unsigned
type occurs the rules for this are defined in ISO/IEC 9899:2011. In general
though signed / unsigned etc is done unsigned.

### MOD

This is misnamed. It's not a modulo operator but a remainder operator.
Specifically like C it requires that the sign of a signed remainder
operation is the sign of the dividend. This may need fixing up if the
hardware has a modulo operator instead.

Otherwise this works like divide.

### UMUL

Not a multiply. This is the C dereference '*' operator. The object on the right
should be dereferenced. If you are moving multiple words take care to handle
the case where the index is the same as a target word, or mark the table entry
accordingly to disallow this.

### OPLOG

Generate the condition codes in order to perform branches. The compiler
doesn't really understand cases where only some flags are set by some
operations.

### AND / OR / ER

Perform a logical operation between the left and right node. If a constant is
present it will be on the right hand side. ER is Exclusive Or.

### GOTO

This is used to generate unconditional internal branches. This may be a
constant or non constant value depending upon the code generation.

### OPLTYPE

Convert the right hand into a register of the correct type. This may be a
constant or non constant value. The constant forms should be present because
this what the compiler will fall back on to get a constant into a register
to match the rule for a higher node.

### UMINUS

Negate the left hand value.

### COMPL

Complement (bit invert) the left hand value.

### FUNARG

Add a function argument. These are needed for each type except structs
which are handeld by STARG

### STARG

Pass a struct argument (not pointer to struct)

## Tree Processing

A lot of the functions within the compiler allow the manipulation of the
parse tree that was created in the first pass. The parse tree consists
of objects of type NODE. These are chained by left and right pointers to
form a tree with value as the leaf nodes.

The current node is passed into many of the machine specific functions
to allow the inspection and sometimes rewriting of nodes. For example the
PDP-11 port rewrites some logic operations to reflect the processors use of
set and clear corresponding bits. Rewrites vary in complexity. A small
number of rewrites are required by the target, particularly to rewrite
arguments, statics and globals and if necessary adjust them for PIC support.

Although it is less obvious the same NODE tree is what drives the rule based
tables and the rules test parts of the node.

The leaf nodes used by the compiler (and thus in effect the primitive
classes of object it works with) are

### Leaf Nodes

#### FCON

This works the same way as an ICON with the floating point value in n_dcon.

#### ICON

An integer constant of arbitrary size (up to the CONSZ size). A constant
may also be a memory reference (ie a literal). In that case n_name is the
name.

#### NAME

A symbol name, possibly with an offset. The n_lval field holds the offset if
any. By the time the code generator is called n_name is the symbol name.

#### OREG

An object whose address is found by adding an offset to a register. The
register is held in n_rval and the offset in n_lval.  An OREG need not be an
object that can be directly generated in one instruction. If the compiler
discovers that the reference cannot be taken that way it will rewrite the
reference in the tree to create a temporary holding the calculated
reference.

#### REG

A register. The encoded register number including the class is held in
n_rval.



### Unary Nodes

These hold objects that have a single subtree. They are used both to
represent C functionality with this property and for some internal
properties.

#### ADDROF

The address of the child object. The C '&' operator.

#### COMPL

The complement of the left hand node. Used for the C ~ operator

#### FLD

A field for bitfield operations. n_rval holds the bit offsets packed
together. The left leg is the bit field reference.

#### FORCE

A special operation used to get the return value of a function into the
return register.

#### FUNARG

A function argument.

#### GOTO

Used to generate branches to the left child node.

#### PCONV

A type conversion whose result is a pointer of some form. PCONV is unusual
in that the type of the NODE and the child NODE may differ.

#### PMCONV

Used in pass1.

#### PVCONV

Used in pass1.

#### SCONV

A type conversion to an integer or floating point type. SCONV is unusual
in that the type of the NODE and the child NODE may differ.

#### UCALL

A function call to a function that has no arguments. Functions that have
arguments use the binary node CALL. Functions returning a struct use
USTCALL or STCALL instead.

#### UMINUS

The C unary minus operator (negation)

#### UMUL

A type indirection (the C * operator)

#### USTCALL

A function call to a function with no arguments, but which returns a
structure.

### Binary Nodes

#### AND

The C binary and operator applied to left and right.

#### ASSIGN

An assignment of right to the object on the left.

#### DIV

The left side divided by the right hand side.

#### EQ

An equality comparison.

#### ER

The C binary xor operator applied to left and right.

#### GE

A signed greater-equal comparison.

#### GT

A signed greater-than comparison.

#### LE

A signed less-than comparison.

#### LS

A left of sift of the left hand side by the right.

#### LT

A signed less-than comparison.

#### MINUS

The subtraction of right from left.

#### MOD

The remainder of left divided by right. Not the modulus. This is the C '%'
operator.

#### NE

A not equals comparison of left and right.

#### PLUS

The addition of left and right.

#### OR

The C binary or operator applied to left and right.

#### RS

A right shift of the left side by the right.

#### UGE

An unsigned greater-equal comparison.

#### UGT

An unsigned greater-than comparison.

#### ULE

An unsigned less-equal comparison.

#### ULT

An unsigned less-than comparison.


### TODO
#### CBRANCH
#### CM
#### CCODES
#### XARG
#### XASM
#### STASG
#### STARG
#### RETURN


## Functions Provided By The Target Code

````
int notoff(TWORD T, int r, CONSZ off, char *cp)
````
This function looks at the type, register, offset and decides whether it is
valid to construct an OREG (register offset) to describe it. This allows
for processors with varying range limits, register limits etc to decide
which if any indexes are valid. When the compiler finds such an OREG is not
valid it will pick another solution, possibly involving moving the value to
a temporary and issuing a constant add in order to create an OREG with no
offset.

Take care to allow for worst case offsets if you are using types that
require multiple loads. Several of the existing targets appears to be buggy
in this respect. If for example your largest reach is 255 bytes but you load
a longlong from address offsets and n + 4 you need to take that into
consideration using the passed type parameter.

Note that you cannot use this function to block the use of a specific
register for offsets. The compiler will always turn an unsuitable OREG into
either a UMUL (for offset 0) or an addition into a temporary and then a
UMUL, and that may itself use any register in the class irrespective of
notoff().

````
void offstar(NODE *p, int shape)
````
Turn a UMUL (pointer indirection) referenced node into an OREG. This
allows the target to manipulate the compile tree to make changes it wishes.

````
void myormake(NODE *(p)
````
During canonicalisation of the tree ay UMUL that survives the generic
attempt to convert it into an OREG is passed to myormake so that it can
look for further opportunities to convert it. This occurs before mycanon()
is invoked.

The possible returns are:

* SRNOPE  Cannot match this shape.
* SRDIR   Direct match, may or may not traverse down.
* SRREG   Will match if put in a regster
* SROREG  Use offstar to turn this into an OREG


````
int shumul(NODE *p, int shape)
````
Decide if a node should be converted via offstar from a UMUL into an OREG


````
int setbin NODE *p)
````
Allow rewriting of binary operators.


````
int setasg(NODE *p, int cookie)
````
Allow setup and rewriting of an assignment operator.

````
int setuni(NODE *p, int cookie)
````
Allow setup and rewriting for a unary operator

````
int setorder(NODE *p)
````
Dunno


````
int livecall(NODE *p)
````
Set any registers in the calling convention as live. Returns a -1 terminated
list of registers. For a typical stack based argument passing scheme this
will just be returning a -1 array.

````
int acceptable(struct optab *op)
````
This functions is invoked when a table entry is being considered. If it
returns zero the rule is ignored. It is intended to allow sub architecture
specific filtering of rules.


````
struct rspecial *nspecial(struct optab *q)
````

When a rule has NSPECIAL set this function is invoked with the rule in
question in q. The function is then responsible for handing back an array of
specific rules for this node.

The rules are an array of struct respecial nodes ending with a 0 node. Each
node consists of a rule and a register.

NLEFT indicates the left hand side must be the named register. NOLEFT
indicates the left hand side must not be the named register.

NRIGHT and NORIGHT do the same thing for the right hand side

NRES indicates the named register will be the result. This should be used
with RDEST in the rule.

NEVER indicates a register that will be clobbered internally by this
operation.

NMOVTO indicates a move between classes

Confusingly getting these rules wrong and the compiler stuck generating code
often generates an error suggesting COLORMAP is buggy. Incorrect rules can
also lead to nonsense register assignments without an error message.


````
void setseg(int seg, char *name
````
Used by the compiler whenever it changes segment. This function then outputs
the target specific directives to switch segment accordingly.

````
void defalign(int al)
````
If MYALIGN is defined then this function is called whenever an alignment
requirement is hit with al holding the alignment. It should output whatever
directives are required to align accordingly.

```
void defloc(struct symtab *sp)
```
Define everything required to make some kind of symbol appropriately
visible.

````
void efcode(void)
````

Generate the code for the end of a function. This is typically a no-op
except for structure returns if supported.

````
void bfcode(struct symtab **sp, int cnt)
````

Generate the code for the beginning of a function. This generally consists
of putting all the arguments into temporary nodes so that the compiler
can then build the resulting code. This can generally be cut and pasted for
a typical stack based setup.

````
void ejobcode(int flag)
````
Called after the assembler output is generated just before the final exit.
Allows the compiler to write markers for invalid output or similar to the
output file, or any final alignment requirement or things like ".end"
directives.

````
void bjobjcode(void)
````
Called before any output occurs to the assembler file. This can be used
to configure the default output writing routines, add headers and so on.

````
NODE *funcode(NODE *p)
````
During function call processing this method is called to allow for target
rule rewriting for function arguments, and to deal with the joys of struct
parsing. For most stack based setups this can be a cut and paste.

````
NODE *fldty(struct symtab *p)
````
Allow the target to set up types on fields.

````
int mygenswitch(int num, TWORD type, struct swents **p, int n )
````
TODO

````
NODE *builtin_cfa(const struct bitable *bt, NODE *a)
NODE *builtin_return_address(const struct bitable *bt, NODE *a)
NODE *builtin_frame_address(const struct bitable *bt, NODE *a)
````
Allow the target to support the gcc style builtins. These are optional
features.

````
NODE *clocal(NODE *p)
````
Before each expresssion tree is turned into code it is handed to this
function to allow any rewriting to be done.

There are certain rewrites that are required. In particular NAME objects
for PARAM, AUTO and STATIC need to be rewritten into the relevant reference
types. Typically that involves faking PARAM and AUTO objects up to a
structure reference (of an imaginary struct of all the param/auto stuff) so
that they are turned into OREF offsets of the frame pointer. Static and
EXTERN/EXTDEF objects may also need to be fixed up for things like PIC mode
support or target features such as globals indexed off a register.

FORCE needs to be rewritten to place the return value in the correct place.

Some platforms rewrite certain type conversion patterns and comparison
patterns where they can be done more efficiently by avoiding casting.

Any other target specific pattern rewrites can also be performed as needed.

Much of this can be cloned from other ports.

````
int andable(NODE *p)
````
Returns 1 if the address of a name can be taken.

````
int cisreg(TWORD t)
````
Returns 1 if the type in question can be placed in a register. If it is zero
then this indicates a type that can only by referenced by its address.
Things like struct are already managed internally by the compiler.

````
void spalloc(NODE *t, NODE *p, OFFSZ off)
````

Generate the required code to allocate a block of data on the stack

TODO: who owns spcon in this case

````
void instring(struct symtab *sp)
````
If MYINSTRING is defined then this routine is called to output strings of
character variables (or widechars) into the output file.

````
int ninval(CONSZ off, int fsz, NODE *p)
````
Output p as a constant into the output file. Allow for any platform
specific formatting and conversions.


````
char *exname(char *p)
````
Return the external symbol name for a C symbol. This varies by target but
is usually unchanged or with the addition of a leading underscore. This
routine can return a static buffer safely.

````
TWORD ctype(TWORD type)
````
If a platform does not implement a particular type as a primitive type then
this function allows the type to be rewritten so that there are no excessive
conversions generated and less rules needed.

Typically it is used to rewrite INT and LONG types signed and unsigned into
the same for a 32bit machine or INT and SHORT types likewise for a 16bit
one. Similarly it can be used to rewrite float and double into the same type
where appropriate.

It should not rewrite char into unsigned char. For an unsigned char default
the defines in macdef.h are used

````
void calldec(NODE *p, NODE *q)
````
TODO

````
void extdec(struct symtab *q)
````
TODO

````
void defzero(struct symtab *sp)
````
Output the correct declarations for a symbol that is a zeroed block. That
may involve placing it in common or bss sections. Any internal alignment
and padding rules should also be considered.

````
int mypragma(char *str)
````
Allow the target to do custom pragma processing. Return 0 if the pragma
is not known or there is no support.

````
void fixdef(struct symtab *sp)
````
Allows the fix up of a symbol when it is declared

````
void pass1_lastchance(struct interpass *ip)
````
A hook for any special processing between pass one and two of the compiler
that would historically have been the insertion of an additional program to
be executed as its own pass.

````
void deflab(in label)
````
Output an internal label definition to the assembler output file. This is
usually something like L255:\n but can vary by target.

````
void prologue(struct interpass_prolog *ipp)
````
At the start of each function this is called to output the entry code for
the function including setting up the frame point, and stacking any
other register variables that are used in this function.

````
void eoftn(struct interpass_prolog *ipp)
````
The companion to prologue this function cleans up the stack frame. It needs
to avoid modifying the return value variables.

````
void hopcode(int f, int o)
````
Generates an opcode string for the 'O' expansion of a rule. This sometimes
allows for fewer rules to be written.

````
int tlen(NODE *p)
````
Given a node return the size in target characters of the resulting object.

````
void rmove(int s, int d, TWORD t)
````
Generate the assembler for a register move of type t between source s and
destination d. This is used in various places to handle movement of objects
between registers. The rmove operations do not go via the normal table
mechanism but an equivalen rule is also needed.

````
void zzcode(NODE *p, int c)
````
When a 'Z' symbol is encounter in the expansion of a table entry this target
specific function is called and passed the following letter in c. It allows
for any target specific behaviour to be handled. This varies from needing
fields in particular formats to tracking the stack when stacking for
function calls through to writing out complex code blocks such as struct
assignments and multi-word register moves

````
int rewfld(NODE *p)
````
return 1. Not used on any current target.


````
int flshape(NODE *p)
````
Given a field in p returns the necessary action to access it. This can be
one of SRDIR (direct match), SROREG (place in an OREG), SRREG (place in a
register) or SRNOPE (not possible).

````
int shtemp(NODE *p)
````
Now obsolete, return 0

````
void adrcon(CONSZ val)
````
Print out the address given (used for N and M expansions)

````
void conput(FILE *fp, NODE *p)
````
fp will always be stdout

Output the given constant node to the assembly output

````
void insput(NODE *p)
````
Called when an Ix (IL IR etc) pattern is found in the table entry being
expanded. Not used by an current target.

````
void upput(NODE *p, int size)
````
Output the uppoer word of p to the assembly file. size is always passed as
the size of a long even though this is wrong on a word based machine.

```
void adrput(FILE *io, NODE *p)
````
io will always be stdout

Ouutput p to the assembly output including any offsets and indirections.
This is the main routine that turns the node into the native syntax for
accessing the data to which it refers.

````
void cbgen(int o, int lab)
````
Generate an internal conditional branch to a compiler generated label. This
is another oddity that does not go via the rules but straight to a function.
o is the operator code, lab is the label.


````
void myreader(struct interpass *ipole)
````
TODO

````
void mycanon(NODE *p)
````
After the tree is built the compiler turns it into a canonical form.
Internally it rearranges arthimetic to put pointers on the left hand
side of additions to pointers and it tries to turn dereferences into
an OREG - replacing various constant patterns that can be turned from
an expression into an OREG (constant offset of register) without
creating temporary values. It then calls mycanon with the parse tree to
allow any architecture specific tree changes to suit the target machine.


````
void myoptim(struct interpass *ip)
````
TODO

````
int COLORMAP(int c, int *r)
````
Given the list of registers by class in the array r return true if the
allocation is trivially possible.

This function is used to evaluate which register assignment combinations
will work without running out of registers. If it is not correct then the
compiler can generate some very very strange and broken register
assignments without actually erroring.

````
int gclass(TWORD t)
````
Convert a type into the required class.

void lastcall(NODE *p)
````
This is invoked on function call nodes just before the function call is
generated. Some platforms use this to calculate things like the size of the
argument block being passed.

````
int special(NODE *p, int shape)
````
Given a node and a shape code that is > MAXSPECIAL (ie user defined) look at
the node and decide if it is a match for the given shape. This can be used
to define target specific match rules.

````
void mflags(char *str)
````
TODO

````
int myxasm(struct interpass *ip, NODE *p)
````
TODO

 