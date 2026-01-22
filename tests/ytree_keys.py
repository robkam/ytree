class Keys:
    """
    Centralized mapping of Logical Actions to Key Characters.
    Based on standard ytree defaults.
    """
    # Navigation
    UP = "\033[A"
    DOWN = "\033[B"
    RIGHT = "\033[C"
    LEFT = "\033[D"
    ENTER = "\r"
    ESC = "\033"
    TAB = "\t"

    # Core Commands
    COPY = "c"
    MOVE = "m"
    RENAME = "r"
    DELETE = "d"

    # Capital 'Y' is PathCopy
    PATHCOPY = "Y"

    # Tagged Operations (often Ctrl-Keys)
    TAGGED_PATHCOPY = "\x19"  # Ctrl-Y
    TAGGED_COPY = "\x0b"      # Ctrl-K (default config)

    # Mode/View Commands
    FILTER = "f"
    SHOWALL = "s"
    TAG = "t"
    UNTAG = "u"
    LOG = "l"
    EXPAND_ALL = "*"

    # Global
    QUIT = "q"
    CONFIRM_YES = "y"
    CONFIRM_NO = "n"