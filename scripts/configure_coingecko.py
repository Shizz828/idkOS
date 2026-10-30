"""Save a CoinGecko Demo key locally without echoing or logging it."""
import getpass
import json
import os
from pathlib import Path
import sys
import tempfile


def main():
    if not sys.stdin.isatty():
        raise SystemExit("Run this command directly in your computer's terminal.")
    key = getpass.getpass("Paste your CoinGecko Demo API key (hidden): ").strip()
    if not key.startswith("CG-") or len(key) <= 3 or any(c.isspace() for c in key):
        raise SystemExit("Expected a CG- API key without spaces. Nothing was saved.")

    root = Path(__file__).resolve().parents[1]
    directory = root / "secrets"
    directory.mkdir(mode=0o700, exist_ok=True)
    directory.chmod(0o700)
    config = {
        "provider": "CoinGecko",
        "plan": "demo",
        "base_url": "https://api.coingecko.com/api/v3",
        "auth_header": "x-cg-demo-api-key",
        "api_key": key,
        "status": "saved_not_yet_tested",
    }
    fd, temporary = tempfile.mkstemp(prefix=".coingecko-", dir=directory)
    try:
        with os.fdopen(fd, "w") as output:
            json.dump(config, output, indent=2)
            output.write("\n")
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, directory / "coingecko.json")
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
    print("Demo key saved locally with restricted permissions; not yet API-tested.")


if __name__ == "__main__":
    main()
