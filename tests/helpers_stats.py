import re


def detect_stats_split_x(snapshot):
    if len(snapshot) < 2:
        return None
    border = snapshot[1]
    marker = "wqqqqqqq FILTER"
    idx = border.find(marker)
    if idx != -1:
        return idx
    return None


def current_file_from_stats(snapshot, split_x=None):
    for idx, line in enumerate(snapshot):
        segment = line[split_x:] if split_x is not None else line
        if "CURRENT FILE" not in segment:
            continue
        for look_ahead in (1, 2):
            row_idx = idx + look_ahead
            if row_idx >= len(snapshot):
                continue

            next_segment = snapshot[row_idx][split_x:] if split_x is not None else snapshot[row_idx]
            match = re.search(r"([A-Za-z0-9._-]+\.txt)", next_segment)
            if match:
                return match.group(1)

            if split_x is not None:
                match = re.search(r"x\s+([^\s]+)", next_segment)
                if match:
                    return match.group(1)

            match = re.search(r"([A-Za-z0-9._-]+\.txt)", snapshot[row_idx])
            if match:
                return match.group(1)
    return None
