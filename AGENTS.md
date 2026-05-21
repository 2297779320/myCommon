# AGENTS.md

This file provides guidance to the AI agent when working with code in this repository.

## Build

```bash
make          # builds everything to ./SampleApp
make clean    # removes all .o and the binary
```

Requires: `gcc`, `pthread`, `sqlite3` (linked via `-lsqlite3`). The makefile uses `find . -name "*.o" -delete` for clean (GNU find).

## Architecture

- **Framework V2** is the active message framework. Use `framework_v2_*` APIs, NOT the V1 `framework_*` APIs.
- Modules are registered via `framework_v2_register_module()` with a `T_MsgProcEntryV2` message table.
- Module lifecycle: `Init` → `Run` (message loop) → `Destroy`.
- Use macros `MODULE_GET_PRIVATE`, `MODULE_SET_PRIVATE`, `MODULE_GET_ID`, `MODULE_GET_FRAMEWORK` to access module internals.
- Two STBP client modes: **Global** (shared, internal reporting) and **UserAgent** (independent, external routing).

## Coding Conventions

- **Type system**: Uses custom typedefs — `INT32`, `UINT8`, `BOOL`, `U32`, `E_StateCode`. Do NOT use `int`/`bool`/`uint32_t` directly; use the project types.
- **NULL check style**: `if (NULL == ptr)` (Yoda condition). Follow this pattern.
- **Boolean**: `BOOL` with `TRUE`/`FALSE` macros (not `true`/`false`).
- **Error handling**: Use `E_StateCode` enum. Check with `STATE_OK(eCode)` macro.
- **Safe free**: Use `LJMEM_FREE(p)` which sets pointer to NULL after free.
- **Safe alloc**: Use `LJ_SAFE_MALLOC(ptr, size)` which zero-initializes.
- **String copy**: Use `SAFESTRCPY(dest, src, dest_size)` — handles truncation.
- **Logging**: Use `dbprintf()` for debug, `SysErr()` for errors, `syswarn()` for warnings.
- **Guard macro**: `return_if_fail(p)`, `return_value_if_fail(p, value)`, `break_if_fail(p)`.
- **Include guards**: `#ifndef XXX_H` / `#define XXX_H` / `#endif`.
- **C++ compat**: Wrap headers with `#ifdef __cplusplus extern "C" { ... #endif`.

## Gotchas

- `defs.h` is the central dependency — it pulls in system headers, SDK headers (`soc_errno`, `securec`, `uapi_*`), and defines all basic types. Almost every module depends on it directly or indirectly.
- `BOOL` is `#define`'d as `int` in `defs.h`, which `#undef`'s the `typedef` in `ctdef.h`. Include `defs.h` (or anything that includes it) rather than `ctdef.h` directly.
- `U64`/`S64` are **structs** with `LSW`/`MSW` fields, not native 64-bit types. This is intentional for DSP compatibility.
- **Linux only**: Will not compile on Windows. `make clean` uses `find . -name "*.o" -delete` which requires GNU find.
- **SDK headers required**: `soc_errno.h`, `securec.h`, `uapi_avplay.h`, `uapi_sound.h`, etc. must be in the compiler's include path — these are from a platform SDK, not in this repo.
- No test framework exists in this repo. There are no automated tests. Use `/build` to verify compilation.
- No lint/format config existed before setup. A `.clang-format` and format-on-edit hook are now configured.
- Config files live in `SampleApp/configs/topic_data/<module>/` as JSON.
- The `SampleApp` binary output is excluded from git via `.gitignore` (`*.exe`, `*.out`).

## Tools

- `/build` — run `make clean && make` and report results
- `/review` — check changed files against project coding conventions
