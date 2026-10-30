Import("env")
import sys
from pathlib import Path
root = Path(env.subst("$PROJECT_DIR"))
sys.path.insert(0, str(root / "scripts"))
from watchlist_codegen import generate
output = root / ".pio/generated/Watchlist.h"
generate(root / "data/crypto/watchlist.json", output)
env.Append(CPPPATH=[str(output.parent)])
