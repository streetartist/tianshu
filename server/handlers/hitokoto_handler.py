# Hitokoto Handler for TianShu Gateway
# API Type: python_handler

import requests

HITOKOTO_API = "https://v1.hitokoto.cn"

try:
    resp = requests.get(HITOKOTO_API, timeout=10)
    data = resp.json()

    # 返回完整信息
    result = {
        "hitokoto": data.get("hitokoto", ""),
        "from": data.get("from", ""),
        "from_who": data.get("from_who", ""),
        "type": data.get("type", ""),
        "creator": data.get("creator", "")
    }
except Exception as e:
    result = {"error": str(e)}
