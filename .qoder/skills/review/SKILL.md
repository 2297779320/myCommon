---
name: review
description: Review changed files against project coding conventions from AGENTS.md.
---

Review the user's changed files (use `git diff` or `git diff --cached`) and check for convention violations:

**Type system violations:**
- Using `int` instead of `INT32`, `unsigned char` instead of `UINT8`, etc.
- Using `bool`/`true`/`false` instead of `BOOL`/`TRUE`/`FALSE`
- Using native `uint32_t` instead of `U32` or `msg_type_t`/`module_id_t`

**Style violations:**
- NULL check not using Yoda style (`if (ptr == NULL)` should be `if (NULL == ptr)`)
- Not using `LJMEM_FREE(p)` for freeing (direct `free(p)` is wrong)
- Not using `LJSAFE_MALLOC` or `malloc` without zero-init
- Not using `SAFESTRCPY` for string copies (raw `strcpy`/`strncpy`)
- Not using guard macros (`return_if_fail`, `return_value_if_fail`)
- Missing include guards in new headers

**Architecture violations:**
- Using V1 framework APIs (`framework_create`, `framework_register_module`) instead of V2 (`framework_v2_*`)
- Including `ctdef.h` directly instead of `defs.h`
- Using `U64`/`S64` as native 64-bit integers (they are structs with LSW/MSW)

**Logging:**
- Using `printf` for errors instead of `SysErr()` or `syserr()`
- Not using `dbprintf()` for debug output

Report each violation with file:line, the problematic code, and the correct form.
