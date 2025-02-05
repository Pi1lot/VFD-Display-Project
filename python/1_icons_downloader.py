import requests
import os

STEAM_API_KEY = ""

STEAM_ID = ""

def get_total_playtime(steam_id, api_key):
    """Retrieves the total playtime for all games."""
    url = "https://api.steampowered.com/IPlayerService/GetOwnedGames/v1/"
    params = {
        "key": api_key,
        "steamid": steam_id,
        "include_played_free_games": True,
        "include_appinfo": True,
    }

    response = requests.get(url, params=params)
    if response.status_code != 200:
        raise Exception(f"Error calling Steam API: {response.status_code}")

    data = response.json()
    games = data.get("response", {}).get("games", [])

    playtime_info = {}
    for game in games:
        playtime_info[game["name"]] = game["playtime_forever"] / 60  # Time in hours

        # Download the game icon with appid as the filename
        appid = game["appid"]
        icon_hash = game.get("img_icon_url", "")
        if icon_hash:
            icon_url = f"http://media.steampowered.com/steamcommunity/public/images/apps/{appid}/{icon_hash}.jpg"
            download_icon(icon_url, appid, game["name"])

def download_icon(url, appid, game_name):
    """Downloads the icon from the URL and saves it with appid as the filename."""
    response = requests.get(url)
    if response.status_code == 200:
        icon_dir = "icons"
        os.makedirs(icon_dir, exist_ok=True)
        icon_path = os.path.join(icon_dir, f"{appid}.jpg")
        with open(icon_path, "wb") as f:
            f.write(response.content)
        print(f"Icon downloaded for {game_name} (appid {appid}): {icon_path}")
    else:
        print(f"Error downloading icon for {game_name} (appid {appid}).")

if __name__ == "__main__":
    playtime = get_total_playtime(STEAM_ID, STEAM_API_KEY)
    for game, hours in playtime.items():
        print(f"{game}: {hours:.2f} hours")

