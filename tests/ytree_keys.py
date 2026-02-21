class Keys:
    """
    Centralized mapping of Logical Actions to Key Characters.
    Based on standard ytree defaults and xterm sequences.
    """
    # Navigation (Application Keypad Mode strings used by ncurses)
    UP = "\033OA"
    DOWN = "\033OB"
    RIGHT = "\033OC"
    LEFT = "\033OD"
    PGUP = "\033[5~"
    PGDN = "\033[6~"
    ENTER = "\r"
    ESC = "\033"
    TAB = "\t"
    CTRL_L = "\x0c"

    # Function Keys (Standard xterm/vt100)
    # Note: If these fail, try "\033OR" for F7 and "\033OS" for F8
    F2 = "\033OQ"
    F7 = "\033[18~"
    F8 = "\033[19~"

    # Core Commands
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