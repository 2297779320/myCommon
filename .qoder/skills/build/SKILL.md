---
name: build
description: Build the project with `make clean && make` and report results.
---

Run the full build:

```bash
make clean && make
```

**What to check:**
- If build succeeds: report "Build OK" with binary size
- If build fails: extract and display the specific error lines (not full output)
- The build requires Linux environment with gcc, pthread, and sqlite3 dev headers
- `make clean` uses `find . -name "*.o" -delete` which needs GNU find (not Windows CMD)
- Working directory must be the project root (where the Makefile is)

**Common failure points:**
- Missing `-lsqlite3`: install `libsqlite3-dev` (Debian/Ubuntu) or `sqlite-devel` (RHEL)
- Missing SDK headers (`soc_errno.h`, `securec.h`, `uapi_*.h`): these are from a platform SDK that must be in the compiler's include path
- The project is Linux-only — will not compile on Windows natively
