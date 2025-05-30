import asyncio
import random
import sys

import urllib3
from datetime import datetime
from datetime import timezone
from typing import List


def send_data_json_over_http(name: str, value: List[str]):
    url = 'https://api.tago.io/data'
    headers = {
        "Content-Type": "application/json",
        "Device-Token": "6c5d97bd-d7f5-42b3-93db-09cca82825e8"
    }

    match name:
        case "(coords)" | "(height)":
            payload = {
                "variable": "location",
                "value": float(value[2]),
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "metadata": {"x": float(value[0]), "y": float(value[1])}
            }
        case "(distance)":
            payload = {
                "variable": "obstacle",
                "value": value[0],
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "unit": "unitless"
            }
        case "(tilt)":
            payload = {
                "variable": "tilt",
                "value": value[0],
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "unit": "degrees"
            }
        case _:
            return

    urllib3.request("POST", url, json=payload, headers=headers)


async def main():
    x_coord : float = 0
    y_coord : float = 0
    z_coord : float = 0

    while True:
        # This waits until you type a line and press ENTER.
        # A real terminal program might put stdin in raw mode so that things
        # like CTRL+C get passed to the remote device.
        data = await asyncio.get_running_loop().run_in_executor(None, sys.stdin.buffer.readline)

        # data will be empty on EOF (e.g. CTRL+D on *nix)
        if not data:
            break

        if data.__contains__(b"(coords)"):
            words = data.split(b" ")
            coords_word = [word for word in words if word in b"(coords)"][0]
            coord_index = words.index(coords_word)

            x_coord = float(words[coord_index + 1].decode(encoding="ascii"))
            y_coord = float(words[coord_index + 2].decode(encoding="ascii"))

            x_coord = (5 + x_coord * 100) / 610.0
            y_coord = (5 + y_coord * 100) / 410.0

            send_data_json_over_http(coords_word.decode(encoding="ascii"), [str(x_coord), str(y_coord), z_coord])
        elif data.__contains__(b"(height)"):
            words = data.split(b" ")
            height_word = [word for word in words if word in b"(height)"][0]
            height_index = words.index(height_word)

            z_coord = float(words[height_index + 1].decode(encoding="ascii"))

            send_data_json_over_http(height_word.decode(encoding="ascii"), [str(x_coord), str(y_coord), z_coord])
        elif data.__contains__(b"(distance)"):
            words = data.split(b" ")
            obstacle_word = [word for word in words if word in b"(distance)"][0]
            obstacle_index = words.index(obstacle_word)
            send_data_json_over_http(obstacle_word.decode(encoding="ascii"),
                                     [words[obstacle_index + 1].decode(encoding="ascii")])
        elif data.__contains__(b"(tilt)"):
            words = data.split(b" ")
            tilt_word = [word for word in words if word in b"(tilt)"][0]
            tilt_index = words.index(tilt_word)
            send_data_json_over_http(tilt_word.decode(encoding="ascii"),
                                     [words[tilt_index + 1].decode(encoding="ascii")])

        print("sent:", data)

if __name__ == '__main__':
    asyncio.run(main())
