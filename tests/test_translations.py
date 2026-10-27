#!/usr/bin/env python3
import json
from pathlib import Path

required = {
    "remap.press", "remap.saved", "remap.save_failed", "remap.skip",
    "button.up", "button.down", "button.left", "button.right",
}

for path in sorted((Path(__file__).parents[1] / "lang/builtin").glob("*.json")):
    data = json.loads(path.read_text())
    missing = required - data.keys()
    assert not missing, f"{path.name}: missing {sorted(missing)}"
    assert data["remap.press"].count("%s") == 1, path.name
    assert data["remap.press"].count("%d") == 2, path.name
