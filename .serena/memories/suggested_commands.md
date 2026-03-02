# Quick Reference Commands

- **Build Optimized Release:** `make`
- **Build Clean Release:** `make clean && make`
- **Build with AddressSanitizer (ASan) and Debug Symbols:** `make DEBUG=1`
- **Run Unit/Behavioral Tests:** `pytest tests/` or `make test`
- **Verbose Tests:** `pytest -v -s` or `make test-v`
- **Run with Debug Logging:** `./ytree 2>/tmp/ytree_debug.log`
- **Check for Memory Leaks:** `valgrind --leak-check=full ./ytree`
