import urllib3
from datetime import datetime
from datetime import timezone
from typing import List


if __name__ == '__main__':
    url = 'https://api.tago.io/data'
    headers = {
        "Content-Type": "application/json",
        "Device-Token": "6c5d97bd-d7f5-42b3-93db-09cca82825e8"
    }

    variable : str = "tilt"
    value = 0.0
    unit : str = "degrees"

    payload = {
        "variable": variable,
        "value": value,
        "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
        "unit": unit
    }

    urllib3.request("POST", url, json=payload, headers=headers)
    print("sent:", headers, payload)