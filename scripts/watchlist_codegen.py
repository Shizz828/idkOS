"""Generate public firmware watchlist metadata; never read credentials."""
import json
from pathlib import Path
import re
import sys


def generate(source, target):
    data = json.loads(Path(source).read_text())
    coins = data.get("coins", [])
    if data.get("plan") != "demo" or not 1 <= len(coins) <= 20:
        raise ValueError("Expected a Demo watchlist with 1-20 coins.")
    if data.get("status") != "provider_ids_verified_demo_request_succeeded":
        raise ValueError("Verify the watchlist IDs before building.")
    entries = []
    seen = set()
    for coin in coins:
        symbol, identifier = coin["symbol"], coin["coingecko_id"]
        if not re.fullmatch(r"[A-Z0-9]{1,10}", symbol) or not re.fullmatch(r"[a-z0-9-]{1,64}", identifier or ""):
            raise ValueError("Invalid watchlist metadata.")
        if identifier in seen:
            raise ValueError("Duplicate watchlist ID.")
        seen.add(identifier)
        entries.append('    {"%s", "%s"},' % (symbol, identifier))
    text = "#pragma once\n#include <stddef.h>\nnamespace spark {\n"
    text += "struct WatchedCoin { const char* symbol; const char* id; };\n"
    text += "constexpr WatchedCoin Watchlist[]={\n" + "\n".join(entries) + "\n};\n"
    text += "constexpr size_t CoinCount=sizeof(Watchlist)/sizeof(Watchlist[0]);\n}\n"
    destination = Path(target)
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text() != text:
        destination.write_text(text)


if __name__ == "__main__":
    generate(sys.argv[1], sys.argv[2])
