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

The test document is deliberately ten objects deep — firm, region, desk, book,
portfolio, position, instrument, pay_leg, accrual — because depth is where the
constant evaluator's budget gets interesting.

## Sample output

    constexpr-marketdata  ·  921 bytes embedded, 10 levels deep, 0 parsed at runtime

    DOCUMENT
    ---   ------------  -----------------  --------------------------
      1   root          schema_version     3
      1   root          as_of              2026-08-16
      ...
     10   accrual       dv01               219,450.75

    DERIVED  (folded to immediates at -O3)
    ---   ------------  -----------------  --------------------------
          coupon        4,132,608.33 EUR
          pv_fixed_leg  4,058,937.46 EUR
          dv01_per_mm   877.80 per MM

## Requirements

- **GCC 16 or newer.** P2996 reflection is gated behind `-freflection`; no other
  compiler ships it as of this writing. CMake selects the newest `g++` it can
  find at 16 or above (including `~/opt/gcc-16/bin`) *before* `project()`, so
  you do not have to pass `-DCMAKE_CXX_COMPILER` on a machine where the system
  compiler is older.
- CMake 3.25+, Ninja, and network access on first configure (simdjson is pulled
  via `FetchContent`).

Verified on GCC 16.0.1 (Ubuntu toolchain PPA snapshot `16-20260315`) under
WSL2 on Ubuntu 22.04, and on the GCC 16.2 release.

### Getting GCC 16

| Distro | How |
| --- | --- |
| Arch | `pacman -S gcc` (a real 16.x release, not a snapshot) |
| Ubuntu 26.04 | `apt install gcc-16 g++-16` from the default repos |
| Ubuntu 22.04 / 24.04 | `add-apt-repository ppa:ubuntu-toolchain-r/test`, then `apt install gcc-16 g++-16` |
| Anything else | Build from source into `~/opt/gcc-16` with `--disable-bootstrap --program-suffix=-16` |

Note that on Ubuntu the PPA upgrades `libstdc++6` and `libgcc-s1` system-wide to
a pre-release snapshot. That is normally harmless, but it is worth knowing if
CUDA or Docker starts behaving oddly afterward.

Sanity-check the compiler before configuring anything:

    printf 'constexpr const char d[] = {\n#embed __FILE__\n, 0 };\nstatic_assert(sizeof(d) > 1);\nint main() {}\n' > /tmp/probe.cpp
    g++-16 -std=c++26 -freflection /tmp/probe.cpp -o /tmp/probe && echo OK

## Build and run

    cmake -B build -G Ninja
    cmake --build build
    ./build/snapshot

Or via the preset, which pins the compiler and flags explicitly:

    cmake --preset default && cmake --build build

Debug build, or a different document:

    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
    cmake -B build -G Ninja -DCTMD_SNAPSHOT=/path/to/other.json

## Prove it folded

    cmake --build build --target check-fold

This fails if any JSON key name survives into the binary, then disassembles
`pv_fixed_leg`. You should see a single load of a literal and a `ret` — no
multiply, no field access, no parser.

## Tests

    ctest --test-dir build --output-on-failure

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

## Build-system notes

Two things here are non-obvious and cost real time to work out.

**simdjson probes for reflection at configure time using the global
`CMAKE_CXX_FLAGS`.** Putting `-freflection` on an `INTERFACE` target is not
enough — simdjson's own `check_cxx_source_compiles` will fail, it will silently
build without the compile-time parser, and your first error will be a confusing
one deep in a header. `CMakeLists.txt` therefore appends `-std=c++26
-freflection` to `CMAKE_CXX_FLAGS` *before* `FetchContent_MakeAvailable`. Watch
for this line at configure time:

    -- simdjson: C++26 static reflection detected, it will be used.

If it says "not available with these compiler settings", stop and fix that
first; nothing downstream will work.

**Set the C compiler too.** simdjson has C sources. Without
`CMAKE_C_COMPILER`, CMake happily pairs GCC 16 for C++ with the distro's much
older `cc`.

CMake caches the compiler on first configure and will not change it in place, so
`rm -rf build` is required after changing any of this.

## Known sharp edges

- **Constexpr budget, not depth, is the wall.** The evaluator is a tree-walking
  interpreter with no SIMD, so throughput is several orders of magnitude below
  runtime simdjson. `-fconstexpr-ops-limit` is already raised in
  `CMakeLists.txt`; a few hundred KB of JSON will still hurt, and you pay it on
  every clean build of every TU that includes the header. On WSL2 a large
  document can also push `cc1plus` into the OOM killer, which surfaces as
  `internal compiler error: Killed` — raise `memory=` in `.wslconfig`.
- **Keys must be C++ identifiers.** `"day-count"` or `"Notional (USD)"` cannot
  become member names. Normalise in your build step, not in C++.
- **Strings don't escape constant evaluation.** `src/main.cpp` funnels every
  string field through a small `sv()` helper rather than assuming a type; keep
  `static_assert`s numeric where you can.
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
