/* dfa.h - declarations for GNU deterministic regexp compiler
   Copyright (C) 1988, 1998, 2007, 2009-2010 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc.,
   51 Franklin Street - Fifth Floor, Boston, MA  02110-1301, USA */

/* Written June, 1988 by Mike Haertel */

/* FIXME:
   2.  We should not export so much of the DFA internals.
   In addition to clobbering modularity, we eat up valuable
   name space. */

/* Number of bits in an unsigned char. */
#ifndef CHARBITS
#define CHARBITS 8
#endif

/* First integer value that is greater than any character code. */
#define NOTCHAR (1 << CHARBITS)

/* INTBITS need not be exact, just a lower bound. */
#ifndef INTBITS
#define INTBITS (CHARBITS * sizeof (int))
#endif

/* Number of ints required to hold a bit for every character. */
#define CHARCLASS_INTS ((NOTCHAR + INTBITS - 1) / INTBITS)

/* Sets of unsigned characters are stored as bit vectors in arrays of ints. */
typedef int charclass[CHARCLASS_INTS];

/* The regexp is parsed into an array of tokens in postfix form.  Some tokens
   are operators and others are terminal symbols.  Most (but not all) of these
   codes are returned by the lexical analyzer. */

typedef enum
{
  END = -1,			/* END is a terminal symbol that matches the
				   end of input; any value of END or less in
				   the parse tree is such a symbol.  Accepting
				   states of the DFA are those that would have
				   a transition on END. */

  /* Ordinary character values are terminal symbols that match themselves. */

  EMPTY = NOTCHAR,		/* EMPTY is a terminal symbol that matches
				   the empty string. */

  BACKREF,			/* BACKREF is generated by \<digit>; it
				   it not completely handled.  If the scanner
				   detects a transition on backref, it returns
				   a kind of "semi-success" indicating that
				   the match will have to be verified with
				   a backtracking matcher. */

  BEGLINE,			/* BEGLINE is a terminal symbol that matches
				   the empty string if it is at the beginning
				   of a line. */

  ENDLINE,			/* ENDLINE is a terminal symbol that matches
				   the empty string if it is at the end of
				   a line. */

  BEGWORD,			/* BEGWORD is a terminal symbol that matches
				   the empty string if it is at the beginning
				   of a word. */

  ENDWORD,			/* ENDWORD is a terminal symbol that matches
				   the empty string if it is at the end of
				   a word. */

  LIMWORD,			/* LIMWORD is a terminal symbol that matches
				   the empty string if it is at the beginning
				   or the end of a word. */

  NOTLIMWORD,			/* NOTLIMWORD is a terminal symbol that
				   matches the empty string if it is not at
				   the beginning or end of a word. */

  QMARK,			/* QMARK is an operator of one argument that
				   matches zero or one occurences of its
				   argument. */

  STAR,				/* STAR is an operator of one argument that
				   matches the Kleene closure (zero or more
				   occurrences) of its argument. */

  PLUS,				/* PLUS is an operator of one argument that
				   matches the positive closure (one or more
				   occurrences) of its argument. */

  REPMN,			/* REPMN is a lexical token corresponding
				   to the {m,n} construct.  REPMN never
				   appears in the compiled token vector. */

  CAT,				/* CAT is an operator of two arguments that
				   matches the concatenation of its
				   arguments.  CAT is never returned by the
				   lexical analyzer. */

  OR,				/* OR is an operator of two arguments that
				   matches either of its arguments. */

  ORTOP,			/* OR at the toplevel in the parse tree.
				   This is used for a boyer-moore heuristic. */

  LPAREN,			/* LPAREN never appears in the parse tree,
				   it is only a lexeme. */

  RPAREN,			/* RPAREN never appears in the parse tree. */

#ifdef MBS_SUPPORT
  ANYCHAR,                     /* ANYCHAR is a terminal symbol that matches
                                  any multibyte (or single byte) characters.
			          It is used only if MB_CUR_MAX > 1.  */

  MBCSET,			/* MBCSET is similar to CSET, but for
				   multibyte characters.  */

  WCHAR,			/* Only returned by lex.  wctok contains
				   the wide character representation.  */
#endif /* MBS_SUPPORT */

  CSET				/* CSET and (and any value greater) is a
				   terminal symbol that matches any of a
				   class of characters. */
} token;

/* Sets are stored in an array in the compiled dfa; the index of the
   array corresponding to a given set token is given by SET_INDEX(t). */
#define SET_INDEX(t) ((t) - CSET)

/* Sometimes characters can only be matched depending on the surrounding
   context.  Such context decisions depend on what the previous character
   was, and the value of the current (lookahead) character.  Context
   dependent constraints are encoded as 8 bit integers.  Each bit that
   is set indicates that the constraint succeeds in the corresponding
   context.

   bit 7 - previous and current are newlines
   bit 6 - previous was newline, current isn't
   bit 5 - previous wasn't newline, current is
   bit 4 - neither previous nor current is a newline
   bit 3 - previous and current are word-constituents
   bit 2 - previous was word-constituent, current isn't
   bit 1 - previous wasn't word-constituent, current is
   bit 0 - neither previous nor current is word-constituent

   Word-constituent characters are those that satisfy isalnum().

   The macro SUCCEEDS_IN_CONTEXT determines whether a a given constraint
   succeeds in a particular context.  Prevn is true if the previous character
   was a newline, currn is true if the lookahead character is a newline.
   Prevl and currl similarly depend upon whether the previous and current
   characters are word-constituent letters. */
#define MATCHES_NEWLINE_CONTEXT(constraint, prevn, currn) \
  ((constraint) & 1 << (((prevn) ? 2 : 0) + ((currn) ? 1 : 0) + 4))
#define MATCHES_LETTER_CONTEXT(constraint, prevl, currl) \
  ((constraint) & 1 << (((prevl) ? 2 : 0) + ((currl) ? 1 : 0)))
#define SUCCEEDS_IN_CONTEXT(constraint, prevn, currn, prevl, currl) \
  (MATCHES_NEWLINE_CONTEXT(constraint, prevn, currn)		     \
   && MATCHES_LETTER_CONTEXT(constraint, prevl, currl))

/* The following macros give information about what a constraint depends on. */
#define PREV_NEWLINE_DEPENDENT(constraint) \
  (((constraint) & 0xc0) >> 2 != ((constraint) & 0x30))
#define PREV_LETTER_DEPENDENT(constraint) \
  (((constraint) & 0x0c) >> 2 != ((constraint) & 0x03))

/* Tokens that match the empty string subject to some constraint actually
   work by applying that constraint to determine what may follow them,
   taking into account what has gone before.  The following values are
   the constraints corresponding to the special tokens previously defined. */
#define NO_CONSTRAINT 0xff
#define BEGLINE_CONSTRAINT 0xcf
#define ENDLINE_CONSTRAINT 0xaf
#define BEGWORD_CONSTRAINT 0xf2
#define ENDWORD_CONSTRAINT 0xf4
#define LIMWORD_CONSTRAINT 0xf6
#define NOTLIMWORD_CONSTRAINT 0xf9

/* States of the recognizer correspond to sets of positions in the parse
   tree, together with the constraints under which they may be matched.
   So a position is encoded as an index into the parse tree together with
   a constraint. */
typedef struct
{
  unsigned index;		/* Index into the parse array. */
  unsigned constraint;		/* Constraint for matching this position. */
} position;

/* Sets of positions are stored as arrays. */
typedef struct
{
  position *elems;		/* Elements of this position set. */
  int nelem;			/* Number of elements in this set. */
} position_set;

/* A state of the dfa consists of a set of positions, some flags,
   and the token value of the lowest-numbered position of the state that
   contains an END token. */
typedef struct
{
  int hash;			/* Hash of the positions of this state. */
  position_set elems;		/* Positions this state could match. */
  char newline;			/* True if previous state matched newline. */
  char letter;			/* True if previous state matched a letter. */
  char backref;			/* True if this state matches a \<digit>. */
  unsigned char constraint;	/* Constraint for this state to accept. */
  int first_end;		/* Token value of the first END in elems. */
#ifdef MBS_SUPPORT
  position_set mbps;           /* Positions which can match multibyte
                                  characters.  e.g. period.
				  These staff are used only if
				  MB_CUR_MAX > 1.  */
#endif
} dfa_state;

/* Element of a list of strings, at least one of which is known to
   appear in any R.E. matching the DFA. */
struct dfamust
{
  int exact;
  char *must;
  struct dfamust *next;
};

#ifdef MBS_SUPPORT
/* A bracket operator.
   e.g. [a-c], [[:alpha:]], etc.  */
struct mb_char_classes
{
  int invert;
  wchar_t *chars;		/* Normal characters.  */
  int nchars;
  wctype_t *ch_classes;		/* Character classes.  */
  int nch_classes;
  wchar_t *range_sts;		/* Range characters (start of the range).  */
  wchar_t *range_ends;		/* Range characters (end of the range).  */
  int nranges;
  char **equivs;		/* Equivalent classes.  */
  int nequivs;
  char **coll_elems;
  int ncoll_elems;		/* Collating elements.  */
};
#endif

/* A compiled regular expression. */
struct dfa
{
  /* Fields filled by the scanner. */
  charclass *charclasses;	/* Array of character sets for CSET tokens. */
  int cindex;			/* Index for adding new charclasses. */
  int calloc;			/* Number of charclasses currently allocated. */

  /* Fields filled by the parser. */
  token *tokens;		/* Postfix parse array. */
  int tindex;			/* Index for adding new tokens. */
  int talloc;			/* Number of tokens currently allocated. */
  int depth;			/* Depth required of an evaluation stack
				   used for depth-first traversal of the
				   parse tree. */
  int nleaves;			/* Number of leaves on the parse tree. */
  int nregexps;			/* Count of parallel regexps being built
				   with dfaparse(). */
#ifdef MBS_SUPPORT
  /* The following are used only if MB_CUR_MAX > 1.  */

  /* The value of multibyte_prop[i] is defined by following rule.
       if tokens[i] < NOTCHAR
         bit 0 : tokens[i] is the first byte of a character, including
                 single-byte characters.
         bit 1 : tokens[i] is the last byte of a character, including
                 single-byte characters.

       if tokens[i] = MBCSET
         ("the index of mbcsets correspnd to this operator" << 2) + 3

     e.g.
     tokens
        = 'single_byte_a', 'multi_byte_A', single_byte_b'
        = 'sb_a', 'mb_A(1st byte)', 'mb_A(2nd byte)', 'mb_A(3rd byte)', 'sb_b'
     multibyte_prop
        = 3     , 1               ,  0              ,  2              , 3
  */
  int nmultibyte_prop;
  int *multibyte_prop;

  /* Array of the bracket expression in the DFA.  */
  struct mb_char_classes *mbcsets;
  int nmbcsets;
  int mbcsets_alloc;
#endif

  /* Fields filled by the state builder. */
  dfa_state *states;		/* States of the dfa. */
  int sindex;			/* Index for adding new states. */
  int salloc;			/* Number of states currently allocated. */

  /* Fields filled by the parse tree->NFA conversion. */
  position_set *follows;	/* Array of follow sets, indexed by position
				   index.  The follow of a position is the set
				   of positions containing characters that
				   could conceivably follow a character
				   matching the given position in a string
				   matching the regexp.  Allocated to the
				   maximum possible position index. */
  int searchflag;		/* True if we are supposed to build a searching
				   as opposed to an exact matcher.  A searching
				   matcher finds the first and shortest string
				   matching a regexp anywhere in the buffer,
				   whereas an exact matcher finds the longest
				   string matching, but anchored to the
				   beginning of the buffer. */

  /* Fields filled by dfaexec. */
  int tralloc;			/* Number of transition tables that have
				   slots so far. */
  int trcount;			/* Number of transition tables that have
				   actually been built. */
  int **trans;			/* Transition tables for states that can
				   never accept.  If the transitions for a
				   state have not yet been computed, or the
				   state could possibly accept, its entry in
				   this table is NULL. */
  int **realtrans;		/* Trans always points to realtrans + 1; this
				   is so trans[-1] can contain NULL. */
  int **fails;			/* Transition tables after failing to accept
				   on a state that potentially could do so. */
  int *success;			/* Table of acceptance conditions used in
				   dfaexec and computed in build_state. */
  int *newlines;		/* Transitions on newlines.  The entry for a
				   newline in any transition table is always
				   -1 so we can count lines without wasting
				   too many cycles.  The transition for a
				   newline is stored separately and handled
				   as a special case. */
  struct dfamust *musts;	/* List of strings, at least one of which
				   is known to appear in any r.e. matching
				   the dfa. */

#ifdef GAWK
  int broken;			/* True if using a feature where there
				   are bugs and gawk should use regex. */
#endif
};

/* Some macros for user access to dfa internals. */

/* ACCEPTING returns true if s could possibly be an accepting state of r. */
#define ACCEPTING(s, r) ((r).states[s].constraint)

/* ACCEPTS_IN_CONTEXT returns true if the given state accepts in the
   specified context. */
#define ACCEPTS_IN_CONTEXT(prevn, currn, prevl, currl, state, dfa) \
  SUCCEEDS_IN_CONTEXT((dfa).states[state].constraint,		   \
		       prevn, currn, prevl, currl)

/* FIRST_MATCHING_REGEXP returns the index number of the first of parallel
   regexps that a given state could accept.  Parallel regexps are numbered
   starting at 1. */
#define FIRST_MATCHING_REGEXP(state, dfa) (-(dfa).states[state].first_end)

/* Entry points. */

/* dfasyntax() takes three arguments; the first sets the syntax bits described
   earlier in this file, the second sets the case-folding flag, and the
   third specifies the line terminator. */
extern void dfasyntax (reg_syntax_t, int, unsigned char);

/* Compile the given string of the given length into the given struct dfa.
   Final argument is a flag specifying whether to build a searching or an
   exact matcher. */
extern void dfacomp (char const *, size_t, struct dfa *, int);

/* Search through a buffer looking for a match to the given struct dfa.
   Find the first occurrence of a string matching the regexp in the
   buffer, and the shortest possible version thereof.  Return a pointer to
   the first character after the match, or NULL if none is found.  BEGIN
   points to the beginning of the buffer, and END points to the first byte
   after its end.  Note however that we store a sentinel byte (usually
   newline) in *END, so the actual buffer must be one byte longer.
   When NEWLINE is nonzero, newlines may appear in the matching string.
   If COUNT is non-NULL, increment *COUNT once for each newline processed.
   Finally, if BACKREF is non-NULL set *BACKREF to indicate whether we
   encountered a back-reference (1) or not (0).  The caller may use this
   to decide whether to fall back on a backtracking matcher. */
extern char *dfaexec (struct dfa *d, char const *begin, char *end,
		      int newline, int *count, int *backref);

/* Free the storage held by the components of a struct dfa. */
extern void dfafree (struct dfa *);

/* Entry points for people who know what they're doing. */

/* Initialize the components of a struct dfa. */
extern void dfainit (struct dfa *);

/* Incrementally parse a string of given length into a struct dfa. */
extern void dfaparse (char const *, size_t, struct dfa *);

/* Analyze a parsed regexp; second argument tells whether to build a searching
   or an exact matcher. */
extern void dfaanalyze (struct dfa *, int);

/* Compute, for each possible character, the transitions out of a given
   state, storing them in an array of integers. */
extern void dfastate (int, struct dfa *, int []);

/* Error handling. */

/* dfaerror() is called by the regexp routines whenever an error occurs.  It
   takes a single argument, a NUL-terminated string describing the error.
   The user must supply a dfaerror.  */
extern void dfaerror (const char *);
