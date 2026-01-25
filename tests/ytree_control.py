import pexpect
import sys
import time
import os
import signal
from ytree_keys import Keys

class YtreeController:
    def __init__(self, binary_path, cwd):
        self.log_file = open("tui_debug.log", "w")
        self.child = pexpect.spawn(
            binary_path,
            cwd=cwd,
            dimensions=(24, 80),
            encoding='utf-8',
            env={'TERM': 'xterm', 'LC_ALL': 'C.UTF-8', 'HOME': cwd},
            timeout=5
        )
        self.child.logfile = self.log_file

    def __del__(self):
        if hasattr(self, 'log_file'):
            self.log_file.close()

    def wait_for_startup(self):
        # Wait for the year to appear in the clock
        self.child.expect(r'20\d{2}')

    def select_file(self, filename):
        """
        Selects a file using Show All + Filter.
        Remains in the Show All window so file commands work immediately.
        """
        # 0. Expand tree to ensure deep files are visible
        # Note: If tree is already expanded, this might toggle it, but for fresh start it's safe.
        # Ideally, we should check state, but unconditional expand (*) works for 'init' state usually.
        time.sleep(0.5)
        self.child.send(Keys.EXPAND_ALL)
        self.wait_for_refresh()

        # 1. Flatten tree to find nested files
        time.sleep(0.5)
        self.child.send(Keys.SHOWALL)
        self.wait_for_refresh()

        # 2. Filter for the specific file
        time.sleep(0.5)
        self.child.send(Keys.FILTER)
        self.child.expect(r"FILTER")
        self.input_text(filename)
        self.wait_for_refresh()

        # 3. Verify file is visible (do NOT press Enter)
        self.child.expect(filename)

    def input_text(self, text):
        """Clears line with Ctrl-U and types text."""
        # Use Ctrl-U (\x15) instead of Ctrl-K (\x0b) because UI_ReadString
        # starts with the cursor at the end of the line.
        self.child.send("\x15")
        time.sleep(0.1)
        self.child.send(text + Keys.ENTER)
        time.sleep(0.2)

    def wait_for_refresh(self):
        """Waits for a screen update (clock tick)."""
        self.child.expect(r'20\d{2}')

    def quit(self):
        """Aggressive quit."""
        try:
            if not self.child.isalive():
                return

            self.child.send(Keys.QUIT)
            time.sleep(0.5)

            if not self.child.isalive():
                return

            self.child.send(Keys.CONFIRM_YES)
            time.sleep(0.5)

            if self.child.isalive():
                try:
                    os.kill(self.child.pid, signal.SIGKILL)
                    os.waitpid(self.child.pid, 0)
                except OSError:
                    pass
        except Exception:
            # Ignore errors during teardown (e.g. process already dead, PtyProcessError)
            pass
        finally:
            try:
                self.child.close(force=True)
            except Exception:
                pass