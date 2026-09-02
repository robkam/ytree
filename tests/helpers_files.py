def wait_for_file(tui, path, timeout=2.0):
    return bool(
        tui.wait_for_condition(
            lambda _lines: path if path.exists() else False,
            timeout=timeout,
            description=f"filesystem effect {path}",
        )
    )
