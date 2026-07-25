# compiler pipeline contract

the v0.1 compiler uses this stage order:

```text
manifest and target loading
→ lexer
→ parser
→ source-faithful AST
→ restricted @cfg pruning and module discovery
→ name resolution
→ type checking
→ HIR
→ HIR-to-MIR monomorphization
→ verified MIR
→ LLVM IR
→ LLVM optimization
→ object emission and linking
```

the arrows are ownership boundaries. a later stage may consume facts from an
earlier one; it does not get to reopen a settled question because an LLVM type
looked convenient.

## driver and discovery

**input:** selected targets, manifests, enabled features and source roots.

**output:** active package graph, normalized source files and target
configuration.

the driver parses a source file before following its imports because a
restricted `@cfg` may remove an import. it then rejects module cycles and
orders the acyclic package graph dependency-first. it does not type-check source
or execute general `comptime` while deciding what files exist.

audit: `S07`, `S09`, `S11-FEATURES`, `S11-CACHE`.

## lexer

**input:** one UTF-8 source file.

**output:** tokens with exact byte, line and Unicode-scalar column spans plus
retained formatter/documentation trivia.

the lexer validates token text and source encoding. it does not resolve names,
infer literal types, check overflow or parse declarations.

the complete lexer is blocked by `BLK-LEX-001` through `BLK-LEX-006`. observed
fixtures may exist before those blockers clear; they are not a complete
language claim.

audit: `S09-LEXER`, `S13-TOOLS`.

## parser

**input:** token stream and retained trivia.

**output:** source-faithful AST nodes with stable IDs and spans.

the parser recognizes grammar, precedence and recovery boundaries. it leaves
names unresolved and types potentially absent. member-style calls remain
syntactic member calls; deciding class method versus UFCS is not parser work.

the parser preserves disabled declarations for formatting and documentation.
restricted `@cfg` only removes them from the semantic AST view after parsing.

audit: `S09-PARSER`, `S09-AST`. blockers: `BLK-SYN-001` through
`BLK-SYN-006`.

## name resolution

**input:** active AST and discovered module graph.

**output:** stable declaration IDs, scopes, import/re-export bindings, label
targets, inheritance links and registered trait implementation heads.

resolution owns visibility and namespace rules. it rejects module/inheritance
cycles, inaccessible names, invalid labels, orphan impls and obvious duplicate
impl heads. overlap requiring substituted types continues into type checking.

it does not infer expression types, validate generic bounds or select machine
layouts.

audit: `S07-*`, `S09-RESOLVE`, `S05-COHERENCE`.

## type checking

**input:** resolved AST.

**output:** typed AST facts and explicit semantic requirements for HIR.

type checking owns literal inference, conversions, expression and pattern
types, exhaustiveness, generic inference/bounds, the type facts needed for C
ABI representability and call-category classification. it derives
Copy/Clone/cleanup and borrow-carrying capabilities without constructing LLVM
types.

after types are complete, declaration-level ABI validation consumes those facts
and applies section 12's closed representability table before HIR lowering.

it does not elaborate cleanup edges, instantiate generic machine code, assign
field offsets or emit vtable loads.

audit: `S01-*`, `S05-*`, `S06-*`, `S09-TYPES`, `S12-REP`.

## HIR

**input:** typed resolved AST.

**output:** typed, resolved high-level IR with explicit conversions, calls,
places, source origins and lowered surface forms.

HIR lowers UFCS, ranges, closure captures, tuple destructuring and postfix `?`.
it resolves concrete trait calls, marks class calls direct/virtual, assigns
class layout/vtable shape and runs the specified moved-state and direct-origin
analyses. permitted constant evaluation also operates over typed HIR.

generic layout-dependent facts remain symbolic. HIR does not yet contain one
body per concrete generic instantiation or target ABI argument locations.

audit: `S02-STATE`, `S04-TRACK`, `S08-SUGAR`, `S09-HIR`.

## HIR to MIR

**input:** checked HIR plus reachable concrete type arguments.

**output:** concrete MIR work items.

this boundary monomorphizes. substitution determines concrete layout,
ownership, cleanup and trait targets. identical semantic instantiations share
one identity; unused instantiations emit no work item.

layout-dependent generic `comptime` runs after substitution. unresolved names,
failed bounds and illegal ownership must not survive into MIR.

audit: `S05-NATIVE`, `S05-ARTIFACT`, `S09-MONO`.

## MIR

**input:** concrete HIR work items.

**output:** target-independent CFGs with typed places and explicit operations.

MIR has branches, loads/stores, copies, moves, borrows, checked arithmetic,
bounds checks, panic edges, allocator calls, cleanup entries and all call
forms. the hidden context parameter is explicit on normal Cami functions and
absent on C calls.

one cleanup stack interleaves owners and reached defers on every normal exit.
panic paths call the non-unwinding runtime and have no fake cleanup edges.

the verifier rejects moved reads, active-borrow bypasses, malformed
terminators, type mismatches and impossible return cleanup state. these are
compiler failures, not new user diagnostics.

audit: `S06-DEFER`, `S09-MIR`, `S13-ICE`.

## LLVM IR generation

**input:** verified MIR and selected target layout.

**output:** verified LLVM IR.

generation maps concrete layouts, globals, vtables, calls, checks, panic calls
and debug locations. target ABI details live here, backed by the selected LLVM
`DataLayout`.

LLVM generation does not resolve names, choose trait impls, infer ownership or
decide whether a safety check exists. if it sees an unresolved type, the
compiler is broken. asking LLVM to improvise would only make the breakage wear
a tie.

audit: `S09-LLVM`, `S12-*`, `S13-DEBUG`.

## LLVM optimization

**input:** verified LLVM IR and profile settings.

**output:** optimized, semantically equivalent LLVM IR.

optimization may remove proven-unreachable overflow or bounds checks and may
devirtualize known calls. it may not turn checked operations into undefined
behavior, enable unsafe fast-math by default or mark aliasable references
`noalias`.

audit: `S01-PROFILES`, `S09-OPT`, `S11-PROFILES`.

## object emission and linking

**input:** optimized LLVM IR, target machine, native libraries and target kind.

**output:** the requested object, executable, `.cami-lib`, static library or
shared library in the specified build tree.

the backend emits the selected platform object format. the driver invokes the
platform linker, reports linker failures as tool failures and never treats a
successful mystery artifact as a cache hit.

executable entry/startup remains blocked by `BLK-ENTRY-001` and
`BLK-ENTRY-002`. registry-produced package input remains blocked by
`BLK-REGISTRY-001` and `BLK-REGISTRY-002`.

audit: `S09-LINK`, `S11-TARGETS`, `S11-LAYOUT`, `S11-CACHE`.

## diagnostic ownership

the earliest informed stage emits the user diagnostic:

| stage                  | examples                                                                            |
| ---------------------- | ----------------------------------------------------------------------------------- |
| lexer                  | invalid UTF-8, malformed token text, unterminated literal                           |
| parser                 | missing delimiter, malformed declaration, impossible token order                    |
| resolution             | missing/inaccessible name, import cycle, invalid label                              |
| type checking          | bad conversion, failed bound, non-exhaustive match, bad C signature                 |
| HIR                    | use after move, maybe-moved use, direct borrow conflict, invalid permitted comptime |
| MIR verifier and later | internal compiler error                                                             |

all stages emit the shared structured diagnostic model. none print private
little error strings and hope tooling develops telepathy.
