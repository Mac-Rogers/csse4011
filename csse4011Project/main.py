import asyncio
import random
import sys

import urllib3
from datetime import datetime
from datetime import timezone
from typing import List

import base_to_pc_pb2


def send_data_json_over_http(name: str, value: List[float]):
    url = 'https://api.tago.io/data'
    headers = {
        "Content-Type": "application/json",
        "Device-Token": "6c5d97bd-d7f5-42b3-93db-09cca82825e8"
    }

    match name:
        case "coords":
            payload = {
                "variable": "location",
                "value": value[2],
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "metadata": {"x": value[0], "y": value[1]}
            }
        case "obstacle":
            payload = {
                "variable": "obstacle",
                "value": value[0],
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "unit": "unitless"
            }
        case "tilt":
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
        data = bytes.fromhex(data.decode(encoding="ascii"))

        decoded_data = base_to_pc_pb2.SentData().FromString(data)

        # data will be empty on EOF (e.g. CTRL+D on *nix)
        if not data:
            break

        if decoded_data.HasField("coords"):
            x_coord = (5 + decoded_data.coords.x * 100) / 610.0
            y_coord = (5 + decoded_data.coords.y * 100) / 410.0

            send_data_json_over_http("coords",
                                     [x_coord, y_coord, z_coord])
            print(f"sent: {x_coord}, {y_coord}, {z_coord}")
        elif decoded_data.HasField("height"):
            z_coord = decoded_data.height

            send_data_json_over_http("coords",
                                     [x_coord, y_coord, z_coord])
            print(f"sent: {x_coord}, {y_coord}, {z_coord}")
        elif decoded_data.HasField("obstacle"):
            send_data_json_over_http("obstacle",[decoded_data.obstacle])
            print(f"sent: {decoded_data.obstacle}")
        elif decoded_data.HasField("tilt"):
            send_data_json_over_http("tilt",[decoded_data.tilt])
            print(f"sent: {decoded_data.tilt}")


if __name__ == '__main__':
    asyncio.run(main())
