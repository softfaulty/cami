# v0.1 conformance seed

this file assigns stable IDs and expected test classes to the current language
rules and examples. it is a plan for the conformance corpus, not evidence that
tests already exist.

the finished fixture paths should use these IDs:

```text
tests/conformance/<section>/<id>-<short-name>.cami
tests/conformance/<section>/<id>-<short-name>.expected
```

## outcome classes

| class           | meaning                                                                                   |
| --------------- | ----------------------------------------------------------------------------------------- |
| `parse-pass`    | syntax must parse; later stages may still reject it                                       |
| `compile-pass`  | type/HIR validation must succeed                                                          |
| `compile-fail`  | compilation must fail with a stable diagnostic family                                     |
| `runtime-pass`  | compiled program must produce the stated value/output                                     |
| `runtime-panic` | compiled program must terminate through the panic runtime                                 |
| `layout`        | size, alignment, offset, calling convention or symbol behavior                            |
| `tool`          | manifest, CLI, formatter, linter, docs or diagnostic protocol behavior                    |
| `documentation` | non-source example constrains names/layout but is not executable                          |
| `split`         | one displayed block contains more than one independently testable outcome                 |
| `blocked`       | the current language contract lacks grammar or protocol required to construct the fixture |

## rule-family coverage

| audit range | primary test classes                     | planned fixture prefix |
| ----------- | ---------------------------------------- | ---------------------- |
| `S01-*`     | compile, runtime panic, layout           | `primitive/`           |
| `S02-*`     | compile, runtime cleanup, layout         | `ownership/`           |
| `S03-*`     | compile, runtime dispatch, layout/IR     | `classes/`             |
| `S04-*`     | compile, unsafe runtime, HIR origin      | `pointers/`            |
| `S05-*`     | compile, artifact, IR                    | `generics/`            |
| `S06-*`     | compile, runtime cleanup/panic           | `errors/`              |
| `S07-*`     | compile, module graph, visibility        | `modules/`             |
| `S08-*`     | parse, compile, runtime lowering         | `syntax/`              |
| `S09-*`     | IR verifier and stage ownership          | `pipeline/`            |
| `S10-*`     | core/std API, ownership, OS integration  | `library/`             |
| `S11-*`     | manifest, resolver, CLI, cache           | `tooling/`             |
| `S12-*`     | compile, C harness, layout               | `abi/`                 |
| `S13-*`     | snapshots, JSON protocol, runtime traces | `diagnostics/`         |

## normative example index

IDs follow the current example order. an edit does not silently renumber them;
the audit must explicitly migrate the ID. bureaucracy is
occasionally useful when the alternative is every golden test forgetting who
it is.

| id       | rule              | expected class            | fixture focus                                                                   |
| -------- | ----------------- | ------------------------- | ------------------------------------------------------------------------------- |
| `EX-001` | `S01-TYPES`       | compile-pass              | contextual integer literal inference                                            |
| `EX-002` | `S01-CHAR`        | compile-pass/layout       | scalar `char` and raw byte distinction                                          |
| `EX-003` | `S01-OVERFLOW`    | runtime-panic             | `u8` compound-assignment overflow                                               |
| `EX-004` | `S01-CASTS`       | compile-pass/runtime-pass | same-type arithmetic with fitting literal                                       |
| `EX-005` | `S01-CASTS`       | runtime-pass              | explicit integer `as` truncation                                                |
| `EX-006` | `S02-MOVE`        | split                     | class move succeeds; later source use fails                                     |
| `EX-007` | `S02-MOVE`        | split                     | replacement destroys destination; source becomes moved                          |
| `EX-008` | `S02-MOVE`        | runtime-pass              | explicit class clone                                                            |
| `EX-009` | `S02-STATE`       | compile-fail              | conditional move produces maybe-moved state                                     |
| `EX-010` | `S02-STRUCTS`     | runtime-pass              | plain struct assignment copies                                                  |
| `EX-011` | `S02-STRUCTS`     | split                     | aggregate with owning field moves as a whole                                    |
| `EX-012` | `S02-ITER`        | compile-pass              | ordinary collection iteration borrows                                           |
| `EX-013` | `S02-ITER`        | runtime-pass              | drain iteration transfers yielded owners                                        |
| `EX-014` | `S02-CONTEXT`     | compile-pass              | ordinary construction observes hidden context                                   |
| `EX-015` | `S02-CONTEXT`     | runtime-pass              | scoped allocator override has dynamic extent                                    |
| `EX-016` | `S03-STATIC`      | documentation             | virtual/override declaration shape uses non-source placeholders                 |
| `EX-017` | `S03-STATIC`      | blocked                   | dispatch result is specified; default construction needed by the fixture is not |
| `EX-018` | `S03-TRAITS`      | runtime-pass/IR           | direct and UFCS trait calls resolve identically                                 |
| `EX-019` | `S04`             | compile-pass              | mutable/const reference behavior over the observed constructor form             |
| `EX-020` | `S04`             | compile-pass              | mutable reference copy and field mutation                                       |
| `EX-021` | `S04-RAW`         | split                     | safe null/equality outside unsafe; raw access/arithmetic inside                 |
| `EX-022` | `S04-RAW`         | compile-pass              | reference/raw-pointer round trip is explicit unsafe                             |
| `EX-023` | `S04-RAW`         | compile-pass              | null exists only for raw pointers                                               |
| `EX-024` | `S04-TRACK`       | split                     | active borrow blocks owner move; move succeeds after scope                      |
| `EX-025` | `S05`             | compile-pass/layout       | distinct generic struct instantiations                                          |
| `EX-026` | `S05`             | compile-pass              | generic function with trait bound                                               |
| `EX-027` | `S05`             | compile-pass              | explicit function type arguments                                                |
| `EX-028` | `S05-BOUNDS`      | parse-pass                | function/type `where` clauses with multiple trait bounds                        |
| `EX-029` | `S05-DEFAULTS`    | compile-pass              | trailing generic type default                                                   |
| `EX-030` | `S05-CONST`       | compile-fail              | user const generic is excluded                                                  |
| `EX-031` | `S06`             | compile-pass              | explicit `Result` and `Option` values                                           |
| `EX-032` | `S06`             | compile-fail              | no implicit Result unwrap                                                       |
| `EX-033` | `S06-PROPAGATE`   | compile-pass/runtime-pass | exact-error Result propagation                                                  |
| `EX-034` | `S06-PROPAGATE`   | compile-pass/runtime-pass | explicit error mapping before propagation                                       |
| `EX-035` | `S06-DEFER`       | runtime-pass              | reached defer executes during normal cleanup                                    |
| `EX-036` | `S06-OPTION`      | compile-pass/runtime-pass | Option propagation                                                              |
| `EX-037` | `S06-MATCH`       | compile-pass/runtime-pass | exhaustive Result match                                                         |
| `EX-038` | `S07`             | documentation             | file paths determine module paths                                               |
| `EX-039` | `S07-IMPORTS`     | compile-pass              | module, alias and selective imports                                             |
| `EX-040` | `S07-IMPORTS`     | compile-pass              | selective public re-export                                                      |
| `EX-041` | `S07-VIS`         | split                     | protected access rules contain pass/fail cases                                  |
| `EX-042` | `S07-LEAKS`       | compile-fail              | public signature leaks package-visible type                                     |
| `EX-043` | `S07-UNSAFE`      | compile-fail              | unsafe cannot bypass private access                                             |
| `EX-044` | `S08-RANGE`       | runtime-pass              | exclusive and inclusive ranges                                                  |
| `EX-045` | `S08-RANGE`       | runtime-pass              | positive range step                                                             |
| `EX-046` | `S08-RANGE`       | runtime-pass              | borrow iteration versus drain iteration                                         |
| `EX-047` | `S08-LABELS`      | runtime-pass              | labeled nested break and continue                                               |
| `EX-048` | `S08-FN`          | parse-pass                | readable function-pointer type                                                  |
| `EX-049` | `S08-FN`          | runtime-pass              | function values in fields, parameters and variables                             |
| `EX-050` | `S08-CLOSURE`     | runtime-pass              | explicitly typed noncapturing closure                                           |
| `EX-051` | `S08-CLOSURE`     | compile-pass              | closure types inferred from expected callback                                   |
| `EX-052` | `S08-CLOSURE`     | runtime-pass              | by-value capture                                                                |
| `EX-053` | `S08-CLOSURE`     | runtime-pass              | explicit reference capture mutates capture                                      |
| `EX-054` | `S08-CLOSURE`     | compile-pass/IR           | `Callable` generic bound lowers statically                                      |
| `EX-055` | `S08-TUPLE`       | runtime-pass/layout       | tuple return and destructuring                                                  |
| `EX-056` | `S08-INDEX`       | runtime-pass              | checked borrowed indexing                                                       |
| `EX-057` | `S08-INDEX`       | compile-pass              | explicit unsafe unchecked access                                                |
| `EX-058` | `S08-EXPR`        | runtime-pass              | value-producing `if`                                                            |
| `EX-059` | `S08-EXPR`        | runtime-pass              | value-producing exhaustive match                                                |
| `EX-060` | `S08-CONST`       | compile-pass              | explicitly typed constants                                                      |
| `EX-061` | `S08-CONST`       | compile-pass              | inferred constant type                                                          |
| `EX-062` | `S09`             | documentation             | exact compiler stage order                                                      |
| `EX-063` | `S10`             | documentation             | exact prelude names                                                             |
| `EX-064` | `S10-COLLECTIONS` | runtime-pass              | collection allocation origin survives context change                            |
| `EX-065` | `S10-COLLECTIONS` | split                     | borrow blocks relocation; mutation succeeds after borrow ends                   |
| `EX-066` | `S10-ARRAY`       | documentation             | minimum Array API contract                                                      |
| `EX-067` | `S10-LIST`        | documentation             | minimum List API contract                                                       |
| `EX-068` | `S10-MAPS`        | documentation             | HashMap and Set API contracts                                                   |
| `EX-069` | `S10-MAPS`        | documentation             | map/set iteration and mutation contract                                         |
| `EX-070` | `S10-STRINGS`     | documentation             | String, StringView and CString API contracts                                    |
| `EX-071` | `S10-FS`          | compile-pass              | explicit File open options                                                      |
| `EX-072` | `S10-FS`          | documentation             | File API contract                                                               |
| `EX-073` | `S10-FS`          | documentation             | filesystem and directory API contract                                           |
| `EX-074` | `S10-NET`         | compile-pass              | socket address parsing returns Result                                           |
| `EX-075` | `S10-NET`         | documentation             | synchronous TCP/UDP API contract                                                |
| `EX-076` | `S10-TIME`        | runtime-pass              | wall-clock and monotonic time use                                               |
| `EX-077` | `S11-SCHEMA`      | tool                      | complete valid manifest                                                         |
| `EX-078` | `S11-DEPS`        | tool                      | registry and path dependency forms                                              |
| `EX-079` | `S11-FEATURES`    | tool                      | additive feature declarations                                                   |
| `EX-080` | `S11-FEATURES`    | tool/compile-pass         | restricted `@cfg` pruning                                                       |
| `EX-081` | `S11-LAYOUT`      | documentation/tool        | conventional package tree                                                       |
| `EX-082` | `S11-LAYOUT`      | documentation/tool        | exact generated artifact tree                                                   |
| `EX-083` | `S11-CLI`         | documentation/tool        | complete v0.1 CLI spelling                                                      |
| `EX-084` | `S12`             | parse-pass                | exact ABI string                                                                |
| `EX-085` | `S12-DECL`        | compile-pass/ABI          | foreign declaration block                                                       |
| `EX-086` | `S12-DECL`        | compile-pass/ABI          | public foreign declaration                                                      |
| `EX-087` | `S12-DECL`        | compile-pass/ABI          | declaration link name                                                           |
| `EX-088` | `S12-DEF`         | compile-pass/ABI          | exported C-callable definition                                                  |
| `EX-089` | `S12-DEF`         | compile-pass/ABI          | exported definition link name                                                   |
| `EX-090` | `S12-DEF`         | compile-pass/ABI          | C-shaped failure translation                                                    |
| `EX-091` | `S12-UNSAFE`      | compile-pass/ABI          | foreign call inside unsafe                                                      |
| `EX-092` | `S12-FNPTR`       | parse-pass                | distinct extern function type                                                   |
| `EX-093` | `S12-FNPTR`       | compile-pass/ABI          | C-callable definition converts to function value                                |
| `EX-094` | `S12-FNPTR`       | compile-pass/ABI          | noncapturing extern closure                                                     |
| `EX-095` | `S12-REP`         | compile-fail              | String rejected by value at declaration                                         |
| `EX-096` | `S12-STRUCT`      | layout/ABI                | representable structs cross C                                                   |
| `EX-097` | `S12-STRUCT`      | compile-fail              | recursive field path rejects bad boundary struct                                |
| `EX-098` | `S12-STRUCT`      | layout/ABI                | explicit-repr fieldless enum                                                    |
| `EX-099` | `S12-OWN`         | compile-pass/ABI          | explicit CString pointer conversion                                             |
| `EX-100` | `S12-VARARGS`     | compile-pass/ABI          | C variadic declaration with fixed parameter                                     |
| `EX-101` | `S12-VARARGS`     | runtime-pass/ABI          | unsafe variadic call and default promotions                                     |
| `EX-102` | `S13-PANIC`       | runtime-panic/snapshot    | minimum panic report                                                            |
| `EX-103` | `S13-TRACE`       | runtime-panic/snapshot    | bounded symbolic trace                                                          |
| `EX-104` | `S13-ANATOMY`     | compile-fail/snapshot     | labeled compiler diagnostic                                                     |
| `EX-105` | `S13-LINTS`       | tool                      | scoped lint attributes                                                          |
| `EX-106` | `S13-TOOLS`       | tool/snapshot             | human diagnostic rendering                                                      |
| `EX-107` | `S13-TOOLS`       | tool/snapshot             | JSON diagnostic envelope                                                        |
| `EX-108` | `S13-TOOLS`       | tool/snapshot             | JSON progress/artifact events                                                   |

## completion rule

commit `129` completes this seed by attaching every audit ID and example ID to
an actual pass, fail, runtime, layout or tool test. `blocked` entries remain
visible until their blocker has normative resolution; deleting the row is not
resolution. it is just losing the receipt.
