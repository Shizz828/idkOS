"""Verify Demo access and resolve the selected watchlist without logging keys."""
import json
from pathlib import Path
import time
import urllib.error
import urllib.parse
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
BASE_URL = "https://api.coingecko.com/api/v3"
AUTH_HEADER = "x-cg-demo-api-key"


def normalized(value):
    return "".join(c for c in value.lower() if c.isalnum())


def main():
    credentials_path = ROOT / "secrets/coingecko.json"
    watchlist_path = ROOT / "data/crypto/watchlist.json"
    credentials = json.loads(credentials_path.read_text())
    key = credentials.get("api_key", "")
    if credentials.get("plan") != "demo" or not key:
        raise SystemExit("Configure your Demo key first; no credentials were printed.")

    def request(path, parameters=None):
        url = BASE_URL + path
        if parameters:
            url += "?" + urllib.parse.urlencode(parameters)
        req = urllib.request.Request(url, headers={
            AUTH_HEADER: key,
            "Accept": "application/json",
            "User-Agent": "idkOS-setup/1.3.0",
        })
        with urllib.request.urlopen(req, timeout=20) as response:
            body = response.read(1024 * 1024 + 1)
        if len(body) > 1024 * 1024:
            raise ValueError("Response exceeded the setup size limit.")
        return json.loads(body)

    try:
        ping = request("/ping")
        if not isinstance(ping, dict) or "gecko_says" not in ping:
            raise ValueError("Unexpected API connection response.")
        print("API connection succeeded. Resolving your watchlist...")
        watchlist = json.loads(watchlist_path.read_text())
        unresolved = False
        for coin in watchlist["coins"]:
            time.sleep(0.5)
            result = request("/search", {"query": coin["name"]})
            matches = [entry for entry in result.get("coins", [])
                       if normalized(entry.get("name", "")) == normalized(coin["name"])
                       and entry.get("symbol", "").upper() == coin["symbol"].upper()]
            if len(matches) != 1 or not matches[0].get("id"):
                unresolved = True
                print(f"Needs review: {coin['name']} ({coin['symbol']})")
                for candidate in result.get("coins", [])[:5]:
                    print(f"  {candidate.get('name')} / {candidate.get('symbol')} / {candidate.get('id')}")
                continue
            coin["coingecko_id"] = matches[0]["id"]
            print(f"Resolved {coin['symbol']}: {coin['coingecko_id']}")
        if unresolved:
            print("Resolution incomplete; existing watchlist was left unchanged. Share the review lines.")
            return

        time.sleep(0.5)
        prices = request("/simple/price", {
            "ids": ",".join(c["coingecko_id"] for c in watchlist["coins"]),
            "vs_currencies": "usd",
            "include_24hr_change": "true",
            "include_last_updated_at": "true",
        })
        for coin in watchlist["coins"]:
            value = prices.get(coin["coingecko_id"], {})
            if not isinstance(value.get("usd"), (int, float)):
                raise ValueError(f"No USD price available for {coin['symbol']}; watchlist not changed.")
        watchlist["status"] = "provider_ids_verified_demo_request_succeeded"
        temporary = watchlist_path.with_suffix(".json.tmp")
        temporary.write_text(json.dumps(watchlist, indent=2) + "\n")
        temporary.replace(watchlist_path)
        print("SUCCESS: Demo price request passed for all 10 coins; verified IDs saved.")
    except urllib.error.HTTPError as error:
        print(f"CoinGecko returned HTTP {error.code}; no credentials were printed.")
        raise SystemExit(1)
    except urllib.error.URLError:
        print("Cannot reach CoinGecko from this terminal. Check the internet connection.")
        raise SystemExit(1)
    except (ValueError, KeyError, TypeError) as error:
        print(f"Setup stopped: {error}")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
