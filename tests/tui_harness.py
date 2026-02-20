import pexpect
import pyte
import time

class YtreeTUI:
    def __init__(self, executable="./build/ytree", cwd=None):
        env = {
            "COLUMNS": "120",
            "LINES": "36",
            "TERM": "xterm-256color"
        }
        
        # Launch ytree in a headless PTY with specific dimensions
        self.child = pexpect.spawn(
            executable,
            env=env,
            dimensions=(36, 120),
            cwd=cwd,
            encoding='utf-8',
            timeout=5
        )
        
        # Initialize an in-memory terminal screen using pyte
        self.screen = pyte.Screen(120, 36)
        self.stream = pyte.Stream(self.screen)
        
        # Read initial startup output
        self._read_output()
        
    def _read_output(self, timeout=0.1):
        """Read pending output from the PTY and feed it to the virtual screen."""
        try:
            # We use a short sleep to allow the application to process and write to the PTY
            time.sleep(timeout)
            
            # Non-blocking read (read_nonblocking could throw Timeout or EOF)
            while True:
                data = self.child.read_nonblocking(size=4096, timeout=0.1)
                self.stream.feed(data)
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

    def send_keystroke(self, keys):
        """Sends keys to the pexpect process, reads output, and updates screen."""
        self.child.send(keys)
        self._read_output()

    def get_screen_dump(self):
        """Returns the screen display as a list of strings representing the grid."""
        # Ensure latest output is collected
        self._read_output(timeout=0.05)
        return self.screen.display

    def quit(self):
        """Cleanly exit."""
        self.child.close(force=True)

