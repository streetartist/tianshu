# Weather Handler for TianShu Gateway
# API Type: python_handler

import time
import jwt
import requests

# QWeather config - 请填写你的配置
PRIVATE_KEY = """-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIMQ/lMtJwsx70GDRBe7PisPMxGb7h7Ia1Qh/iXZBKXbZ
-----END PRIVATE KEY-----"""
PROJECT_ID = "YOUR_PROJECT_ID"  # 项目ID
KEY_ID = "YOUR_KEY_ID"  # 凭据ID
BASE_URL = "https://ph6k5pmxwq.re.qweatherapi.com"

def get_token():
    payload = {
        'iat': int(time.time()) - 30,
        'exp': int(time.time()) + 900,
        'sub': PROJECT_ID
    }
    headers = {'kid': KEY_ID}
    return jwt.encode(payload, PRIVATE_KEY, algorithm='EdDSA', headers=headers)

def lookup_city(city):
    token = get_token()
    resp = requests.get(
        f"{BASE_URL}/geo/v2/city/lookup",
        headers={"Authorization": f"Bearer {token}"},
        params={"location": city},
        timeout=10
    )
    data = resp.json()
    if data.get("code") == "200" and data.get("location"):
        return data["location"][0]["id"]
    return None

def get_weather(location_id):
    token = get_token()
    resp = requests.get(
        f"{BASE_URL}/v7/weather/3d",
        headers={"Authorization": f"Bearer {token}"},
        params={"location": location_id},
        timeout=10
    )
    return resp.json()

# Main
city = params.get("city", "上海")
loc_id = lookup_city(city)

if loc_id:
    data = get_weather(loc_id)
    if data.get("code") == "200" and data.get("daily"):
        d = data["daily"][0]
        result = f"{city}今天{d['textDay']}，{d['tempMin']}~{d['tempMax']}°C，湿度{d['humidity']}%"
    else:
        result = "获取天气失败"
else:
    result = f"未找到城市: {city}"
