# compiler interface contracts

this document records the current compiler/runtime boundaries. it describes
facts crossing a boundary, not a C++ class hierarchy.
actual headers can stay small. the interface does not become more correct when
every record acquires an abstract factory.

## source files and spans

every loaded source file exposes:

| field | contract |
| --- | --- |
| normalized path | package-relative when the file belongs to a package |
| bytes | original validated UTF-8 source bytes |
| line index | maps a byte offset to one-based source line |
| scalar column index | maps a byte offset to one-based Unicode-scalar column |

a half-open span contains:

```text
file identity
byte start
byte end
line start
column start
line end
column end
```

byte offsets are authoritative for edits. line/column values are presentation
and protocol facts derived from the same source. tabs and multibyte scalars may
change columns without changing the byte contract.

audit: `S09-LEXER`, `S13-TOOLS`.

## stable identities

AST declarations and nodes use stable IDs within one compilation. HIR and MIR
refer to declarations, types, functions, blocks and places by IDs rather than
owning C++ pointers into growable containers.

an ID is meaningful only with its owning compilation/session table. serializing
an arena index and calling it a global identity would be less an interface and
more a delayed accident.

package artifacts use explicit serialized identities and a format version.
they do not serialize host pointers or C++ object layout.

audit: `S09-AST`, `S09-RESOLVE`, `S05-ARTIFACT`.

## diagnostics

every stage emits one shared diagnostic record:

```text
severity
stable diagnostic code
message
primary span, when source exists
zero or more secondary spans with labels
notes
suggestions
```

each suggestion contains:

```text
applicability
file
byte start
byte end
replacement text
```

the renderer owns color and source snippets. compiler stages do not pre-render
terminal art into `message`.

the initial stable code registry is blocked by `BLK-DIAG-001`. the record shape
is not.

audit: `S13-ANATOMY`, `S13-SUGGEST`, `S13-TOOLS`.

## token, AST, HIR and MIR boundaries

### token stream

tokens carry:

- token kind
- exact source span
- source spelling or an interned reference to it
- retained trivia needed by formatter/docs

the token kind set remains incomplete until the lexical blockers clear.

### AST

AST nodes preserve:

- source shape and written names
- attributes and trivia
- stable node IDs
- exact spans
- inactive `@cfg` state without deleting the source node

AST nodes do not contain LLVM types, final declaration IDs or synthesized
cleanup control flow.

### HIR

HIR contains:

- resolved declaration/type IDs
- explicit implicit-conversion nodes
- explicit call kind and target requirement
- typed places and source origins
- lowered range/UFCS/closure/tuple/propagation forms
- moved-state and borrow-origin results
- symbolic generic layout operations where substitution is pending

the checked public subset of HIR may be serialized. private compiler pointers,
diagnostic renderer state and LLVM objects may not.

### MIR

MIR contains:

- typed basic blocks and terminators
- explicit copy, move, borrow, load and store operations
- explicit checked arithmetic and bounds operations
- explicit allocator, panic and cleanup operations
- direct, virtual, Cami-function-pointer and C-function-pointer calls
- the hidden context parameter on ordinary Cami functions

MIR is target-independent in control-flow shape, but concrete work items already
know substituted types. ABI register assignment remains codegen work.

audit: `S09-*`.

## target layout service

one compiler-owned target-layout interface answers:

```text
pointer size and alignment
primitive size and alignment
struct field offsets, size and alignment
union size and alignment
fixed-array stride, size and alignment
explicit-repr enum layout
class-handle size and alignment
concrete class object layout
target C ABI classification required by codegen
```

the implementation is backed by the selected LLVM `DataLayout` and target
machine. semantic code asks this service for target facts; it does not construct
LLVM types just to discover whether a struct field moved.

required invariants:

- struct fields remain in declaration order
- representable structs/unions follow the selected target C ABI
- a class handle is two target words
- `@sizeOf(Class)` returns handle size, not heap-object size
- the base class subobject begins at offset zero
- one hierarchy vtable pointer exists only when virtual dispatch requires it

audit: `S02-STRUCTS`, `S03-LAYOUT`, `S09-LLVM`, `S12-STRUCT`.

## ordinary Cami call ABI

every ordinary Cami function semantically receives one hidden pointer to the
current `Context`. source declarations do not include it.

```text
normal call:
    written arguments
    hidden Context*

extern "C" call:
    written C arguments only
```

ordinary calls forward the same pointer. entering a `with
context.allocator = value` scope copies the current context to stack storage,
replaces the allocator field in the child and forwards the child pointer for
the block's dynamic extent.

the optimizer may remove context traffic when the callee cannot observe it.
the ABI still contains the semantic argument. “LLVM probably deletes it” is
not a source-level cost model.

audit: `S02-CONTEXT`, `S12`.

## Context and allocator handles

`Context` is a small copyable runtime value. its allocator field is a copyable
handle to stable allocator state, not the state itself.

an allocator handle supports the operations needed by `core`:

```text
allocate
fallible allocate
release
identity/origin comparison
```

exact source-level declarations remain under `BLK-LIB-001`. the runtime
contract is already fixed:

- forwarding a context performs no allocation
- copying a context copies the handle but does not extend allocator-state
  lifetime
- every successful allocation records its allocator origin
- release uses the recorded origin, never whichever allocator is current later
- `unsafe with` is required when allocator lifetime is not proven

audit: `S02-CONTEXT`, `S10-COLLECTIONS`.

## owning class handle

the runtime value is exactly:

```text
non-null complete-object pointer
allocator-origin pointer/handle
```

move copies those words and invalidates the source in HIR. there is no runtime
moved flag, reference count or collector field.

destruction:

1. run the most-derived destructor body;
2. destroy that layer's owning fields in reverse declaration order;
3. continue through base layers;
4. release the complete object through the stored allocator origin.

the basic constructor shape is observed. default construction, destructor and
clone-constructor syntax remain blocked by `BLK-SYN-002`; the ownership and
layout interface does not.

audit: `S02-MOVE`, `S03-LAYOUT`.

## borrow-origin summaries

an exported function returning a reference or borrow-carrying value stores a
summary describing which parameters/receiver may be its origin.

the summary domain contains:

```text
parameter origin set
receiver origin
static origin
unknown origin
```

direct and statically resolved trait calls substitute caller origins through
the summary. virtual and indirect calls conservatively include every
borrow-capable argument plus unknown.

the summary is serialized in package metadata and participates in public
interface/cache hashes.

audit: `S04-TRACK`, `S05-ARTIFACT`.

## panic runtime

all panic entry points are non-returning and non-unwinding. they receive enough
static data to report:

```text
stable category
message or checked-operation description
source file, line and column
operation-specific facts already available at the panic site
```

panic reporting must have a bounded allocation-free fallback. enabled traces
inspect at most 64 frames and still terminate through the platform abort path.
no destructor or defer executes after panic.

the callable runtime surface uses an explicit C ABI so generated code does not
depend on the host C++ ABI.

audit: `S06-PANIC`, `S13-PANIC`, `S13-TRACE`.

## `.cami-lib` artifact

a Cami library artifact contains:

```text
format version
compiler/language/manifest compatibility facts
target and profile facts
compiled non-generic code
exported signatures and nominal type metadata
public impl metadata
borrow-origin summaries
checked HIR for public generics
reachable generic definitions needed downstream
```

the format is compiler metadata, not stable C ABI. loaders reject incompatible
versions rather than reading a nearby layout and hoping it has not developed
new fields.

private generics remain inside their package unless required by an exported
generic body.

audit: `S05-ARTIFACT`, `S11-TARGETS`.

## cache keys

stage cache keys include every semantic input relevant to that stage:

```text
source content
imported public interfaces
compiler and serialized-HIR versions
target triple and data layout
profile settings
enabled feature union
relevant manifest and lockfile entries
identified linker inputs
```

corruption produces a miss or diagnostic, never a valid-looking output with a
different answer.

audit: `S05-ARTIFACT`, `S11-CACHE`.

## C ABI boundary

the boundary accepts exactly target-default `"C"`. representability is checked
at declaration time and recursively records the first invalid field path.

generated C-callable definitions:

- omit the hidden context parameter;
- create a fresh root context on entry;
- run normal local cleanup before destroying that context;
- never unwind a panic through C.

incoming executable startup is separate and remains blocked by
`BLK-ENTRY-001`/`BLK-ENTRY-002`.

audit: `S12-*`.

## machine-readable tool stream

in JSON mode, each Cami-tool or test-harness event is one UTF-8 object on one
stderr line with:

```text
schema = 1
reason
package
target
reason-specific fields
```

diagnostic objects include the shared diagnostic facts. progress, artifact and
test objects use distinct reasons. verbose compiler/linker output remains
structured.

after a successful `cami run` handoff, the program owns both inherited streams.
the tool cannot promise uninterrupted JSON while an arbitrary program is
printing into the same pipe, because physics remains regrettably enabled.

audit: `S13-TOOLS`.

## unresolved interfaces

the following are intentionally absent here:

- complete token and AST variant lists while grammar blockers remain
- exact executable entry/startup ABI
- registry transport/auth/archive protocol
- complete source declarations for every `core`/`std` API
- properties, a GC wrapper and async/await

their blocker IDs live in
[`implementation-blockers.md`](implementation-blockers.md). none should become
a private compiler extension merely to make a milestone turn green.
