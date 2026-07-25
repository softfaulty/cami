# extracted syntactic grammar

this file collects syntax that the current language rules state or show
normatively. it does not fill missing productions. holes point to
[`implementation-blockers.md`](implementation-blockers.md), because a parser
is a fairly expensive place to keep an undocumented opinion.

## notation

- quoted text is an exact grammar literal.
- `name`, `expression`, `pattern` and similar lowercase names are nonterminals.
- `[x]` means optional.
- `{x}` means zero or more.
- `x | y` means either form.
- `BLOCKED(id)` means the current language rules do not close that production.

lexical details come from [`lexical-grammar.md`](lexical-grammar.md).

## files and modules

```ebnf
source-file = { import-declaration }, { top-level-declaration } ;

top-level-declaration
    = function-declaration
    | function-definition
    | struct-declaration
    | union-declaration
    | class-declaration
    | fieldless-enum
    | impl-declaration
    | constant-declaration
    | extern-block
    | extern-definition
    | BLOCKED(BLK-SYN-001)
    | BLOCKED(BLK-SYN-003)
    | BLOCKED(BLK-SYN-004)
    | BLOCKED(BLK-SYN-006) ;

import-declaration
    = [ "public" ], "import", module-path, ";"
    | [ "public" ], "import", module-path, "as", name, ";"
    | [ "public" ], "import", module-path, ".{", import-items, "}", ";" ;

import-items = name, { ",", name } ;
module-path = name, { ".", name } ;
```

imports precede ordinary declarations. wildcard imports, whole-module
re-exports and source `module` declarations are not v0.1 syntax. module names
come from package name and file path.

## visibility and attributes

```ebnf
visibility = "private" | "module" | "package" | "protected" | "public" ;

attribute = "@", name, [ "(", attribute-arguments, ")" ] ;
attribute-arguments = BLOCKED(BLK-SYN-005) ;
```

individual specified forms include:

```text
@cfg(feature = "name")
@cfg(target_os = "name")
@cfg(target_arch = "name")
@export
@linkName("symbol")
@repr(i32)
@test
@allow("lint_name")
@warn("lint_name")
@deny("lint_name")
@deprecated("message")
```

the grammar does not generalize those examples into arbitrary expressions.
attachment sites and repetition remain blocked by `BLK-SYN-005`.

## types

```ebnf
primitive-type
    = "i8" | "i16" | "i32" | "i64" | "i128"
    | "u8" | "u16" | "u32" | "u64" | "u128"
    | "isize" | "usize" | "f32" | "f64"
    | "bool" | "char" | "void"
    | "int" | "uint" | "byte" ;

primitive-integer-type
    = "i8" | "i16" | "i32" | "i64" | "i128"
    | "u8" | "u16" | "u32" | "u64" | "u128"
    | "isize" | "usize" | "int" | "uint" | "byte" ;

nominal-type = module-path, [ generic-arguments ] ;
generic-arguments = "<", type, { ",", type }, ">" ;

raw-pointer-type = "*", [ "const" ], type ;
reference-type = "&", [ "const" ], type ;

function-type = "fn", "(", [ type, { ",", type } ], ")", "->", type ;
extern-function-type = "extern", "\"C\"", function-type ;

tuple-type = "(", type, ",", type, { ",", type }, ")" ;

type
    = primitive-type
    | nominal-type
    | raw-pointer-type
    | reference-type
    | function-type
    | extern-function-type
    | tuple-type ;
```

fixed arrays use C-shaped declarators such as `i32 values[16]`. the language does
not define a standalone fixed-array type spelling independent of a declarator.
general declarator composition remains part of `BLK-SYN-001`.

`Option<T>` is the only optional spelling. there is no `T?` type.

## generic parameters and bounds

```ebnf
generic-parameter = name, [ "=", type ] ;
generic-parameters
    = "<", generic-parameter, { ",", generic-parameter }, ">" ;

trait-list = nominal-type, { "+", nominal-type } ;
constraint = name, ":", trait-list ;
where-clause = "where", constraint, { ",", constraint } ;
```

type defaults are trailing and apply only to generic structs/classes. generic
functions have no default type parameters. const generic parameters are not
v0.1 syntax.

## functions and methods

the language uses return-type-first C-shaped declarations:

```ebnf
parameter = type, name ;
parameter-list = parameter, { ",", parameter } ;

function-head
    = [ visibility ], type, name, [ generic-parameters ],
      "(", [ parameter-list ], ")", [ where-clause ] ;

function-declaration = function-head, ";" ;
function-definition = function-head, block ;
const-method-definition = function-head, "const", block ;

virtual-method-head
    = [ visibility ], "virtual", type, name,
      "(", [ parameter-list ], ")" ;

override-method-head
    = [ visibility ], type, name,
      "(", [ parameter-list ], ")", "override" ;

virtual-method-definition = virtual-method-head, block ;
override-method-definition = override-method-head, block ;
```

`const` receiver qualification appears after a method's parameter list in
examples:

```text
i32 getCount() const
```

the complete interaction and order between `const`, `override`, visibility and
other modifiers is not closed. `BLK-SYN-001` applies.

constructor examples establish this basic definition:

```ebnf
constructor-definition = name, "(", [ parameter-list ], ")", block ;
```

default construction, modifier order, destructors and clone constructors remain
blocked by `BLK-SYN-002`.

## structs, unions and classes

```ebnf
field-declaration = [ visibility ], type, name, { "[", constant-expression, "]" }, ";" ;

struct-declaration
    = [ visibility ], "struct", name, [ generic-parameters ],
      [ where-clause ], "{", { type-member }, "}" ;

union-declaration
    = [ visibility ], "union", name,
      "{", { field-declaration }, "}" ;

class-declaration
    = [ visibility ], "class", name, [ generic-parameters ],
      [ ":", nominal-type ], [ where-clause ],
      "{", { type-member }, "}" ;

type-member
    = field-declaration
    | function-definition
    | const-method-definition
    | virtual-method-definition
    | override-method-definition
    | constructor-definition
    | BLOCKED(BLK-SYN-002) ;
```

single inheritance uses the colon only for a base class. trait implementations
never appear in that list.

## traits and implementations

the one closed outer form is:

```ebnf
impl-declaration
    = "impl", nominal-type, "for", nominal-type,
      [ where-clause ], "{", impl-body, "}" ;

impl-body = BLOCKED(BLK-SYN-003) ;
trait-declaration = BLOCKED(BLK-SYN-003) ;
```

UFCS member spelling is an expression rewrite after resolution, not a second
declaration form.

## enums and patterns

the language rules close the explicit-representation fieldless example:

```ebnf
repr-attribute = "@repr", "(", primitive-integer-type, ")" ;

fieldless-enum
    = [ repr-attribute ], [ visibility ], "enum", name,
      "{", enum-value, { ",", enum-value }, [ "," ], "}" ;

enum-value = name, [ "=", constant-expression ] ;
```

payload enums, their constructors and the complete pattern grammar remain
blocked by `BLK-SYN-004`. the behavior of `Option`, `Result` and exhaustive
matching is normative; their missing declaration grammar is still missing.

the observed match shell is:

```ebnf
match-expression
    = "match", "(", expression, ")", "{",
      match-arm, { match-arm }, "}" ;

match-arm
    = pattern, "=>", expression, ","
    | pattern, "=>", block, [ "," ] ;

pattern = "_" | BLOCKED(BLK-SYN-004) ;
```

## statements

```ebnf
block = "{", { statement }, [ trailing-expression ], "}" ;

variable-declaration
    = type, name, [ "=", expression ], ";"
    | "let", name, [ ":", type ], "=", expression, ";"
    | "let", "(", name, ",", name, { ",", name }, ")",
      "=", expression, ";" ;

declaration-statement = variable-declaration | constant-declaration ;

statement
    = block
    | declaration-statement
    | expression, ";"
    | "if", "(", expression, ")", statement, [ "else", statement ]
    | "switch", BLOCKED(BLK-SYN-001)
    | classic-for-statement
    | range-for-statement
    | "while", "(", expression, ")", statement
    | "do", statement, "while", "(", expression, ")", ";"
    | "break", [ name ], ";"
    | "continue", [ name ], ";"
    | "return", [ expression ], ";"
    | "defer", block
    | "unsafe", block
    | allocator-with-statement ;

classic-for-statement = "for", "(", BLOCKED(BLK-SYN-001), ")", statement ;

range-for-statement
    = "for", name, "in", expression, [ "step", expression ], statement ;

allocator-with-statement
    = [ "unsafe" ], "with", "context.allocator", "=", expression, block ;

labeled-loop = name, ":", ( classic-for-statement | range-for-statement
                          | "while", "(", expression, ")", statement
                          | "do", statement, "while", "(", expression, ")", ";" ) ;
```

loop labels use the C-shaped form `name: loop`. arbitrary labeled blocks are
not supported. the compact optional-label notation above is descriptive; the
normative examples show `break outer;` and `continue outer;`.

## expressions

the language defines these expression families:

```ebnf
expression
    = literal
    | name-expression
    | call-expression
    | member-expression
    | index-expression
    | tuple-expression
    | closure-expression
    | if-expression
    | match-expression
    | range-expression
    | cast-expression
    | postfix-propagation
    | unary-expression
    | binary-expression
    | assignment-expression
    | parenthesized-expression ;

call-expression = expression, "(", [ expression, { ",", expression } ], ")" ;
member-expression = expression, ".", name ;
index-expression = expression, "[", expression, "]" ;
cast-expression = expression, "as", type ;
postfix-propagation = expression, "?" ;

range-expression
    = expression, "..", expression
    | expression, "..=", expression ;

tuple-expression = "(", expression, ",", expression, { ",", expression }, ")" ;
```

operator precedence and associativity are not comprehensively stated.
`BLK-SYN-001` blocks a complete expression grammar.

### closures

```ebnf
closure-expression
    = [ capture-list ], "fn",
      "(", [ closure-parameter-list ], ")",
      [ "->", type ], block ;

capture-list = "[", [ capture, { ",", capture } ], "]" ;
capture = name | "&", name ;
closure-parameter-list
    = closure-parameter, { ",", closure-parameter } ;
closure-parameter = [ type ], name ;
```

parameter/return omission is valid only when an expected callback type supplies
them. expression-bodied closure shorthand is not v0.1 syntax.

### expression control flow

```ebnf
if-expression
    = "if", "(", expression, ")", value-block,
      "else", ( value-block | if-expression ) ;

value-block = "{", { statement }, trailing-expression, "}" ;
trailing-expression = expression ;
```

a trailing value has no semicolon. statement `if` remains valid without
`else`.

## constants

observed constant declarations are:

```ebnf
constant-declaration
    = [ visibility ], "const", [ type ], name, "=", expression, ";" ;
```

at least one of `type` or an inferable initializer type is required by typing.
general `comptime` syntax and the marker for permitted functions remain blocked
by `BLK-COMPTIME-001`.

## C ABI forms

```ebnf
extern-block
    = "extern", "\"C\"", "{",
      { [ attribute ], function-declaration },
      "}" ;

extern-definition
    = { attribute }, "extern", "\"C\"", function-definition ;

extern-closure
    = "extern", "\"C\"", "fn",
      "(", [ parameter-list ], ")", "->", type, block ;

variadic-parameter-list
    = parameter, { ",", parameter }, ",", "..." ;
```

`"C"` is the only accepted ABI string. C variadic definitions are not valid.

## explicitly absent

the extracted grammar contains no productions for:

- properties
- a GC wrapper
- async/await
- trait objects
- const generics
- multiple inheritance
- foreign globals
- non-C calling conventions
- C variadic definitions

undefined surface is not “reserved for compatibility” unless the normative
language rules say so. currently they do not.

## parser acceptance gate

`BLK-SYN-001` through `BLK-SYN-006`, plus the lexical blockers, prevent a claim
of complete grammar support. the fragments above are still useful: they define
the parse corpus that is already supported by normative text and identify the
precise places where implementation must stop instead of improvising.
