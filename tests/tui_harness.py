import pexpect
import pyte
import time
import os

class YtreeTUI:
    def __init__(self, executable="./build/ytree", cwd=None, env_extra=None, args=None):
        self.time_scale = self._read_time_scale()
        env = {
            "TERM": "xterm",
            "LC_ALL": "C.UTF-8",
            "HOME": cwd if cwd else "/tmp",
        }
        if env_extra:
            env.update(env_extra)
        
        # Launch ytree in a headless PTY with specific dimensions
        self.child = pexpect.spawn(
            executable,
            args=args or [],
            env=env,
            dimensions=(36, 120),
            cwd=cwd,
            encoding='utf-8',
            timeout=max(5.0 * self.time_scale, 5.0)
        )
        
        # Initialize an in-memory terminal screen using pyte
        self.screen = pyte.Screen(120, 36)
        self.stream = pyte.Stream(self.screen)
        
        # Wait for the main UI tree to be ready (handles startup scan + any error dialogs)
        # The tree pane shows box-drawing like "tq" or "mq" once the dir is scanned.
        if not self.wait_for_content("tq", timeout=8.0) and not self.wait_for_content("mq", timeout=1.0):
            # Fallback: just sleep and drain
            time.sleep(2.0 * self.time_scale)
            self._read_output()

    @staticmethod
    def _read_time_scale():
        raw = os.getenv("YTREE_TUI_TIME_SCALE", "1.0")
        try:
            scale = float(raw)
        except (TypeError, ValueError):
            return 1.0
        return max(1.0, scale)

    def _scaled(self, seconds):
        return seconds * self.time_scale

    def _read_output(self, timeout=0.1):
        """Read pending output from the PTY and feed it to the virtual screen."""
        try:
            # We use a short sleep to allow the application to process and write to the PTY
            time.sleep(self._scaled(timeout))
            
            # Non-blocking read (read_nonblocking could throw Timeout or EOF)
            while True:
                data = self.child.read_nonblocking(size=4096, timeout=self._scaled(0.1))
                self.stream.feed(data)
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

    def send_keystroke(self, keys, wait=0.3):
        """Sends keys to the pexpect process, reads output, and updates screen."""
        self.child.send(keys)
        self._read_output(timeout=wait)

    def get_screen_dump(self):
        """Returns the screen display as a list of strings representing the grid."""
        # Ensure latest output is collected
        self._read_output(timeout=0.05)
        return self.screen.display

    def wait_for_content(self, target, timeout=5.0):
        """Wait until the target string appearing anywhere on the screen."""
        effective_timeout = self._scaled(timeout)
        start_time = time.time()
        while time.time() - start_time < effective_timeout:
            screen = self.get_screen_dump()
            for line in screen:
                if target in line:
                    return True
            time.sleep(0.1)
        return False

    def quit(self):
        """Cleanly exit."""
        self.child.close(force=True)
