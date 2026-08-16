# constexpr-marketdata

Compile-time parsing of nested financial JSON with C++26 static reflection,
`#embed`, and simdjson — schema validated by concepts, zero runtime parse.

## What this actually does

A conventional program ships a JSON config, opens it at startup, parses it, and
discovers a typo as a runtime error in someone else's session. This project
moves all of that to the compiler.

1. `#embed` pulls `data/market_snapshot.json` into the translation unit as a
   `constexpr` byte array. There is no file I/O anywhere in the program.
2. `simdjson::compile_time::parse_json` parses those bytes during constant
   evaluation and *synthesises the struct type from the keys it finds* — you
   never declare the shape by hand.
3. Concepts in `include/ctmd/schema.hpp` assert the shape you actually expected,
   so a valid-but-wrong document (a `notional` that arrived as `250000000.5`)
   fails the build at the concept rather than at some call site three files
   away.
4. Anything computed from the result is itself a constant. Under `-O3` the
   parsing, the field accesses, and the arithmetic all disappear; only the
   answers remain as immediates.

The test document is deliberately ten objects deep — firm → region → desk →
book → portfolio → position → instrument → pay_leg → accrual — because depth is
where the constant evaluator's budget gets interesting.

## Requirements

- **GCC 16 or newer.** P2996 reflection is gated behind `-freflection`; no other
  compiler ships it as of this writing. CMake hard-errors on anything else.
- CMake 3.25+, Ninja, and network access on first configure (simdjson is pulled
  via `FetchContent`).

## Build and run

```sh
cmake -B build -G Ninja
cmake --build build
./build/snapshot
```

Debug build, or a different document:

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake -B build -G Ninja -DCTMD_SNAPSHOT=/path/to/other.json
```

## Prove it folded

```sh
cmake --build build --target check-fold
```

This fails if any JSON key name survives into the binary, then disassembles
`pv_fixed_leg`. You should see a single load of a literal and a `ret` — no
multiply, no field access, no parser.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

`depth` runs the per-layer `static_assert`s (it passing at all means they held
at compile time). `neg_malformed` and `neg_wrong_schema` are inverted: they
invoke the build system on documents that *must* be rejected, and fail if
compilation succeeds.

## Layout

| Path | What lives there |
| --- | --- |
| `data/market_snapshot.json` | The ten-level document baked into the binary |
| `data/invalid/` | Documents that must not compile |
| `include/ctmd/snapshot.hpp` | `#embed` + `parse_json` + the `constexpr` object |
| `include/ctmd/schema.hpp` | Concepts describing the expected shape |
| `cmake/ReflectionSupport.cmake` | Compiler probe for `-freflection` and `#embed` |
| `tools/check_fold.sh` | Disassembly check that nothing survived |

## Known sharp edges

- **Constexpr budget, not depth, is the wall.** The evaluator is a tree-walking
  interpreter with no SIMD, so throughput is several orders of magnitude below
  runtime simdjson. `-fconstexpr-ops-limit` is already raised in
  `CMakeLists.txt`; a few hundred KB of JSON will still hurt, and you pay it on
  every clean build of every TU that includes the header.
- **Keys must be C++ identifiers.** `"day-count"` or `"Notional (USD)"` cannot
  become member names. Normalise in your build step, not in C++.
- **Strings don't escape constant evaluation.** Compare per character or via
  `std::string_view` inside a `constexpr` function; keep `static_assert`s
  numeric where you can.
- **Arrays bake their length in.** Adding one element to a `"cashflows"` array
  is a recompile of everything that touches it.
- **Rebuild coupling is the real trade-off.** This is the right shape for
  day-count conventions, holiday centre codes, index definitions, and product
  taxonomies. It is the wrong shape for anything carrying a daily fixing.

## What this is not

It is not a faster parser. Runtime simdjson answers "how fast can I parse bytes
I just received"; this answers "can I avoid receiving them at all." The honest
comparison is against hand-writing the same table as a `constexpr` array in a
header — identical codegen, but here the source of truth stays JSON, shared
with whatever Python or Java tooling also consumes it.

## Credit

Based on Daniel Lemire, *Parsing JSON at compile time with C++26 static
reflection* (June 2026). The simdjson compile-time implementation is by Daniel
Lemire and Francisco Geiman Thiesen.
