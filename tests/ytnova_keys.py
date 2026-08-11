class Keys:
    """
    Centralized mapping of Logical Actions to Key Characters.
    Based on standard ytnova defaults and xterm sequences.
    """
    # Navigation (Application Keypad Mode strings used by ncurses)
    UP = "\033OA"
    DOWN = "\033OB"
    RIGHT = "\033OC"
    LEFT = "\033OD"
    PGUP = "\033[5~"
    PGDN = "\033[6~"
    HOME = "\033OH"
    END = "\033OF"
    ENTER = "\r"
    ESC = "\033"
    TAB = "\t"
    CTRL_A = "\x01"
    CTRL_S = "\x13"
    CTRL_T = "\x14"
    CTRL_L = "\x0c"
    CTRL_O = "\x0f"
    CTRL_U = "\x15"
    CTRL_V = "\x16"
    CTRL_W = "\x17"

    # Function Keys (Standard xterm/vt100)
    # Note: If these fail, try "\033OR" for F7 and "\033OS" for F8
    F1 = "\033OP"
    F2 = "\033OQ"
    F3 = "\033OR"
    F4 = "\033OS"
    F5 = "\033[15~"
    F6 = "\033[17~"
    F7 = "\033[18~"
    F8 = "\033[19~"
    F9 = "\033[20~"

    # Core Commands
    ATTRIBUTE = "a"
    COPY = "c"
    MOVE = "m"
    RENAME = "r"
    DELETE = "d"
    MAKE_FILE = "n"
    PATHCOPY = "Y"  # Ensure this is present!

    # View Controls
    FILTER = "f"
    SHOWALL = "s"
    EXPAND_ALL = "*"
    LOG = "l"

    # Global
    QUIT = "q"
    CONFIRM_YES = "y"
    CONFIRM_NO = "n"
