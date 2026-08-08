# extracted lexical grammar

this is the lexical surface the current language rules commit to. it remains an
extraction rather than a second specification with suspiciously independent
opinions.

## source text

source files are UTF-8. outside quoted literals, whitespace is exactly:

```ebnf
whitespace = " " | "\t" | "\n" | "\r\n" ;
```

LF and CRLF each end one line. a lone CR is invalid. tokens and trivia retain
their byte range plus one-based line and Unicode-scalar column. tabs are one
source scalar and render at four-column stops.

audit: `S09-LEXER`, `S13-TOOLS`.

## identifiers

v0.1 identifiers are ASCII and case-sensitive:

```ebnf
identifier-start = "A"…"Z" | "a"…"z" | "_" ;
identifier-continue = identifier-start | "0"…"9" ;
identifier = identifier-start, { identifier-continue } ;
```

non-ASCII text remains valid in literals and comments. identifiers perform no
Unicode normalization or case folding because they contain no non-ASCII code
points. simple, stable, considerably less haunted.

audit: `S09-LEXER`.

## reserved keywords

the closed v0.1 keyword set is:

```text
as          bool        break       byte        char        class
comptime    const       continue    defer       do          else
enum        extern      f32         f64         false       fn
for         i8          i16         i32         i64         i128
if          impl        import      in          int         isize
let         match       module      none        null        override
package     private     protected   public      return      step
struct      switch      trait       true        u8          u16
u32         u64         u128        uint        union       unsafe
usize       virtual     void        where       while       with
```

`core`, `std`, `context`, `some`, `ok` and `err` are ordinary identifiers.
reserving `comptime` and `trait` does not resolve their remaining syntactic
blockers; it only means they cannot cosplay as variable names.

audit: `S01-PRIMITIVES`, `S02-OWNERSHIP`, `S03-DISPATCH`, `S04-POINTERS`,
`S05-GENERICS`, `S06-ERRORS`, `S07`, `S08`, `S09-LEXER`, `S12-ABI`.

## comments and retained trivia

```ebnf
line-comment = "//", { any UTF-8 scalar except CR or LF } ;
doc-line-comment = "///", { any UTF-8 scalar except CR or LF } ;
block-comment = "/*", { any UTF-8 scalar sequence not containing "*/" }, "*/" ;
doc-block-comment = "/**", { any UTF-8 scalar sequence not containing "*/" }, "*/" ;
```

`///` wins over `//`, and `/**` wins over `/*`. `////` is a documentation line
comment whose body begins with `/`; `/**/` is an empty documentation block
comment. block comments do not nest. an unterminated block comment is a lexical
error through end of file. block-comment line endings still use LF or CRLF; a
lone CR does not become valid merely because a comment tried to hide it.

the lexer retains whitespace and all four comment forms in source order.
ordinary comments belong to formatting; documentation comments are additionally
available to documentation generation.

audit: `S09-LEXER`, `S13-TOOLS`.

## punctuation and operators

the closed spelling set is:

```text
( ) { } [ ] , ; : .
... ..= .. -> :: => ? @
+ - * / % ++ --
& | ^ ~ ! && ||
= += -= *= /= %= &= |= ^= <<= >>=
== != < > <= >= << >>
```

the lexer consumes the longest valid spelling at each byte. comment openers are
recognized before `/`. quoted-literal delimiters belong to their literal tokens
and are not punctuation.

this table closes token boundaries, not precedence or semantics. the parser and
type checker still decide what a spelling means in context.

audit: `S01-OVERFLOW`, `S04-POINTERS`, `S05-GENERICS`, `S06-ERRORS`, `S08`,
`S09-LEXER`, `S12-ABI`.

## literals

`true`, `false`, `none` and `null` are keyword-shaped literal spellings.
numeric and quoted literals remain separate token families.

normative examples establish decimal and `0x` integers with separators plus
decimal floating literals, but the complete number grammar remains blocked by
`BLK-LEX-004`.

character literals use single quotes and produce one Unicode scalar. strings use
double quotes and produce UTF-8 text. their escape, raw/multiline and malformed
forms remain blocked by `BLK-LEX-005`.

the leading `-` is always punctuation rather than part of a numeric literal.
overflow and literal typing remain later-stage work.

## lexer acceptance gate

identifiers, keywords, whitespace, comments and punctuation are closed and may
ship in commit `013`. `BLK-LEX-004` and `BLK-LEX-005` continue to block the
complete literal lexer in commits `014` and `015`; this commit does not solve
numbers or escapes through optimism.
