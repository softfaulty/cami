# extracted lexical grammar

this is the lexical surface the current language rules actually commit to. it
is an extraction, not a creative writing exercise for tokens. anything not
closed by the language contract is marked with a blocker from
[`implementation-blockers.md`](implementation-blockers.md).

## notation

| label       | meaning                                                                                         |
| ----------- | ----------------------------------------------------------------------------------------------- |
| `specified` | the language rules state the spelling or character rule directly                                |
| `observed`  | normative examples use the spelling, but the language rules do not close the whole token family |
| `blocked`   | a conforming lexer cannot choose the missing rule                                               |

an observed spelling may be tokenized for fixtures. it does not prove that the
same word is reserved everywhere or that no other spelling exists.

## source text

| item                | status    | extracted rule                                            | audit                    |
| ------------------- | --------- | --------------------------------------------------------- | ------------------------ |
| encoding            | specified | source files are UTF-8; invalid UTF-8 is a source error   | `S09-LEXER`              |
| source locations    | specified | tokens retain byte range, line and Unicode-scalar column  | `S09-LEXER`, `S13-TOOLS` |
| line endings        | blocked   | normalization and accepted line-ending set are not stated | `BLK-LEX-003`            |
| Unicode identifiers | blocked   | allowed code points and normalization are not stated      | `BLK-LEX-001`            |

## identifiers and names

the only closed identifier-like production is the manifest package/feature
name:

```ebnf
lower = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" | "j"
      | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" | "s" | "t"
      | "u" | "v" | "w" | "x" | "y" | "z" ;

digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;

package-name = lower, { lower | digit | "_" } ;
```

that grammar belongs to `package.toml`. source identifiers are only described
as “valid identifiers,” so they remain blocked by `BLK-LEX-001`. importing the
manifest regex into source code would be a language decision wearing a fake
moustache.

case is semantically significant in every shown identifier. the language rules do not
state case folding or normalization, so the lexer may not invent either.

## grammar literals

the following words have a grammatical role in normative prose or examples:

```text
as          bool        break       byte        char        class
const       continue    defer       do          else        enum
extern      f32         f64         false       fn          for
i8          i16         i32         i64         i128        if
in
impl        import      int         isize       let         match
module      none        override    package     private     protected
public      return      step        struct      switch      true        u8
u16         u32         u64         u128        uint        union
unsafe      usize       virtual     void        where       while
with
```

this is not a closed reserved-keyword list. `core`, `std`, `context`, `some`,
`ok` and `err` are used as names or constructors and are not declared
keywords. `BLK-LEX-002` blocks keyword reservation.

properties, a GC wrapper and async/await are not part of the extracted v0.1
grammar. prose references do not manufacture tokens.

## literals

### booleans and null

```ebnf
boolean-literal = "true" | "false" ;
null-literal = "null" ;
```

`null` is valid only for raw pointers after type checking. it is still one
lexical token here; the lexer is not promoted to null-safety management.

### integers

normative examples demonstrate decimal and `0x`-prefixed hexadecimal integers,
including underscore separators:

```text
0
1
255
4096
0xFF
0x1_0000_0001
```

the examples establish those spellings, not a complete integer grammar.
binary/octal prefixes, separator placement, suffixes and digit diagnostics are
blocked by `BLK-LEX-004`.

the leading `-` in a negative integer is a unary operator, not proven to be
part of the literal token. that keeps overflow checking where the language places
it: typing and constant evaluation.

### floating point

normative examples demonstrate decimal floating literals such as `2.5`.
fraction requirements, exponent forms, hexadecimal floats and suffixes are not
closed. `BLK-LEX-004` applies.

### characters

normative examples use single quotes:

```text
'C'
'💜'
```

after decoding, a character literal contains exactly one Unicode scalar and
may not contain a surrogate. the escape syntax needed to produce characters
that cannot be written directly is blocked by `BLK-LEX-005`.

### strings

normative examples use double quotes and source strings represent UTF-8 text.
string values may contain NUL and are not implicitly terminated. the following
remain blocked by `BLK-LEX-005`:

- escape spellings
- escaped newline behavior
- raw or multiline forms
- how a source NUL is represented
- the exact diagnostic boundary for a malformed escape

the ABI string `"C"` and attribute strings use the same observed string token.
the language does not define a second string-literal lexer for them, which is nice.
one is enough.

## comments and trivia

normative examples demonstrate `//` line comments. section 9 also requires:

- documentation comments retained for documentation generation
- ordinary comments retained for formatting

the delimiter for documentation comments, the existence and nesting of block
comments, newline consumption and unterminated-comment behavior are not
defined. `BLK-LEX-003` blocks the complete comment grammar.

whitespace separates tokens where concatenation would change tokenization.
the accepted whitespace code points are not closed by the language rules.

## punctuation and operators

the following spellings occur in normative syntax or are named directly:

```text
( )  { }  [ ]  ,  ;  :  .  ...
@    ->   ::    ?
=    +=   +     -     *  /  %
==   !=   <     >     <= >=
&
..   ..=
```

important context:

- `*` is multiplication, pointer declarator or dereference depending on parse
  context.
- `&` is bitwise-and or reference formation/declarator depending on context.
- `->` is raw-pointer member access.
- `.` is module/member access and tuple-field access.
- `::` appears in associated names such as `i32::MAX`.
- `?` is postfix propagation.
- `..` and `..=` are range operators.
- `...` is valid only in a C variadic declaration.
- `@` begins an attribute or builtin spelling such as `@sizeOf`.

the language rules name bitwise and shift behavior but do not provide their complete
spellings, a closed operator table or lexical longest-match rules. parser
precedence is likewise not fully stated. those facts belong to `BLK-LEX-006`
and `BLK-SYN-001`; guessing longest-match from vibes would probably work until
it very much did not.

## lexer acceptance gate

a complete lexer release remains blocked until `BLK-LEX-001` through
`BLK-LEX-006` are normatively resolved. before then, implementation may support
the observed corpus for prototyping, but must not label that corpus the complete
Cami lexical grammar.
