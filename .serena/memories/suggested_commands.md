# Quick Reference Commands

- **Build Optimized Release:** `make`
- **Build Clean Release:** `make clean && make`
- **Build with AddressSanitizer (ASan) and Debug Symbols:** `make DEBUG=1`
- **Run Unit/Behavioral Tests:** `pytest tests/` or `make test`
- **Verbose Tests:** `pytest -v -s` or `make test-v`
- **Run with Debug Logging:** `./ytnova 2>/tmp/ytnova_debug.log`
- **Check for Memory Leaks:** `valgrind --leak-check=full ./ytnova`
