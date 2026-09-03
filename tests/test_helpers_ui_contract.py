from helpers_ui import drive_action_until


class _ActionDrivenTUI:
    def __init__(self, screens):
        self.screens = iter(screens)
        self.actions = []
        self.current_screen = next(self.screens)

    def get_screen_dump(self):
        return self.current_screen

    def wait_for_condition(self, predicate, timeout):
        return predicate(self.current_screen)

    def send_and_wait_for_condition(self, action, predicate, timeout):
        self.actions.append(action)
        self.current_screen = next(self.screens)
        return predicate(self.current_screen)


def test_drive_action_until_stops_when_visible_identity_is_selected():
    tui = _ActionDrivenTUI([["root"], ["selected-fixture"]])

    selected = drive_action_until(
        tui,
        "down",
        lambda lines: lines if "selected-fixture" in lines else False,
        max_actions=4,
        timeout=0.1,
    )

    assert selected == ["selected-fixture"]
    assert tui.actions == ["down"]


def test_drive_action_until_preserves_an_already_selected_identity():
    tui = _ActionDrivenTUI([["selected-fixture"]])

    selected = drive_action_until(
        tui,
        "down",
        lambda lines: lines if "selected-fixture" in lines else False,
        max_actions=4,
        timeout=0.1,
    )

    assert selected == ["selected-fixture"]
    assert tui.actions == []
