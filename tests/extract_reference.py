"""Create host-only test references directly from the untouched Windows source.

Only platform arguments, sound output, and clock access are removed. Gameplay
statements are retained, giving the tests an independent source of expectations.
Generated references are never part of the UEFI module.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "CampfireHunt.c").read_text(encoding="utf-8")


def extract(name):
    start = re.search(r"^\w+ " + name + r"\(", source, re.M).start()
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


names = ["GenerateGoblinPosition", "GenerateMonMonPosition", "GenerateMeatPosition",
         "GenerateMoneyPosition", "ShuffleTile", "CheckGoblinCollision", "CheckMonmonCollision",
         "CollectMeat", "CollideGoblin", "CollideMonmon", "CalculateScore"]
text = "/* Generated from original CampfireHunt.c, for host tests only. */\n"
text += "static float ReferenceTime;\n"
for name in names:
    body = extract(name)
    body = re.sub(r"Texture2D \w+\[\], ", "", body)
    body = re.sub(r"Sound \w+, ", "", body)
    body = re.sub(r", Sound \w+\)", ")", body)
    body = re.sub(r"^\s*PlaySound\([^\n]+\);[^\n]*\n", "\n", body, flags=re.M)
    body = body.replace("GetTime()", "ReferenceTime")
    # Explicit casts preserve original small integer values under host /WX.
    body = re.sub(r"(\w+(?:\.position|\.velocity)?\.[xy] = )(MARGIN \+ rand[^;]+|rand\(\) % 3 - 1|\(rand\(\) % 3 - 1\) \* 1.5);", r"\1(float)(\2);", body)
    body = re.sub(r"(moneyPos\.[xy] = )\(float\)\(([^;]+)\);", r"\1\2;", body)
    for called in names:
        body = re.sub(r"\b" + called + r"\b", "Reference" + called, body)
    text += body + "\n\n"

# Copy the stage branches, rather than rewriting expected thresholds in Python.
body = extract("UpdateSingleplayerGame")
body = body[body.index("            *remainingTime -="):body.index("            UpdateObjects(")]
body = body.replace("GetFrameTime()", "deltaTime")
text += "void ReferenceStage(GameState *game, float deltaTime) {\n"
text += "float *remainingTime = &game->remainingTime; Stage *gamestage = &game->stage;\n"
text += "player *player1 = &game->players[0]; Screen *gamescreen = &game->screen;\n"
text += body + "}\n"
out = ROOT / "Build/tests/original_reference.h"
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(text, encoding="utf-8")
print("Extracted original gameplay references for host tests")
