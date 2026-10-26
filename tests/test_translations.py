#!/usr/bin/env python3
import json
from pathlib import Path

required = {
    "remap.press", "remap.saved", "remap.save_failed", "remap.skip",
    "button.up", "button.down", "button.left", "button.right",
}

paths = sorted((Path(__file__).parents[1] / "lang/builtin").glob("*.json"))
reference = set(json.loads((Path(__file__).parents[1] / "lang/builtin/en_US.json").read_text()))

for path in paths:
    data = json.loads(path.read_text())
    missing_keys = reference - data.keys()
    extra_keys = data.keys() - reference
    assert not missing_keys and not extra_keys, (
        f"{path.name}: missing {sorted(missing_keys)}, extra {sorted(extra_keys)}"
    )
    missing = required - data.keys()
    assert not missing, f"{path.name}: missing {sorted(missing)}"
    assert data["remap.press"].count("%s") == 1, path.name
    assert data["remap.press"].count("%d") == 2, path.name
