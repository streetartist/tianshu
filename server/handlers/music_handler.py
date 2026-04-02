# Music Handler for TianShu Gateway
# API Type: python_handler

import requests

API_KEY = "th_fa137f10c89ec4c4bc69cbe7343ec571ad9bee13a2e345fc"
BASE_URL = "https://tunehub.sayqz.com/api"

def search_and_parse(keyword, platform="netease"):
    headers = {"X-API-Key": API_KEY}

    # Get search config
    cfg_resp = requests.get(
        f"{BASE_URL}/v1/methods/{platform}/search",
        headers=headers, timeout=10
    )
    cfg = cfg_resp.json().get("data", {})
    if not cfg:
        return None

    # Execute search
    params = {}
    for k, v in cfg.get("params", {}).items():
        params[k] = str(v).replace("{{keyword}}", keyword).replace("{{page}}", "0")

    search_resp = requests.get(
        cfg.get("url"),
        params=params,
        headers=cfg.get("headers", {}),
        timeout=10
    )
    songs = search_resp.json().get("data", {}).get("songs", [])
    if not songs:
        return None

    # Parse first song
    song_id = songs[0].get("id")
    parse_resp = requests.post(
        f"{BASE_URL}/v1/parse",
        headers=headers,
        json={"platform": platform, "ids": str(song_id), "quality": "320k"},
        timeout=15
    )
    return parse_resp.json()

# Main
song = params.get("song", "")
if not song:
    result = {"error": "请提供歌曲名"}
else:
    data = search_and_parse(song)
    if data and data.get("code") == 0:
        tracks = data.get("data", [])
        if tracks:
            t = tracks[0]
            result = {
                "name": t.get("name"),
                "artist": t.get("artist"),
                "url": t.get("url"),
                "summary": f"找到歌曲: {t.get('name')} - {t.get('artist')}"
            }
        else:
            result = {"error": "解析失败"}
    else:
        result = {"error": f"未找到: {song}"}
