import re


def detect_stats_split_x(lines):
    if len(lines) < 2:
        return None
    border = lines[1]
    marker = "wqqqqqqq FILTER"
    idx = border.find(marker)
    if idx != -1:
        return idx
    return None


def current_file_from_stats(lines, split_x=None):
    for idx, line in enumerate(lines):
        segment = line[split_x:] if split_x is not None else line
        if "CURRENT FILE" not in segment:
            continue
        for look_ahead in (1, 2):
            row_idx = idx + look_ahead
            if row_idx >= len(lines):
                continue

            next_segment = lines[row_idx][split_x:] if split_x is not None else lines[row_idx]
            match = re.search(r"([A-Za-z0-9._-]+\.txt)", next_segment)
            if match:
                return match.group(1)

            if split_x is not None:
                match = re.search(r"x\s+([^\s]+)", next_segment)
                if match:
                    return match.group(1)

            match = re.search(r"([A-Za-z0-9._-]+\.txt)", lines[row_idx])
            if match:
                return match.group(1)
    return None
