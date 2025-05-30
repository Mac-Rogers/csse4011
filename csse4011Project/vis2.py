import asyncio
import random
import sys

import urllib3
from datetime import datetime
from datetime import timezone
from typing import List
import aiohttp

#import base_to_pc_pb2

import aiohttp
import numpy as np
import pygame
import time
import os
import threading

should_beep = False
beep_thread = None

last_coord = None
last_tilt = None
last_height = None

def beep_loop():
    pygame.mixer.init()
    channel = None
    sound = pygame.mixer.Sound("beep1.mp3")

    while True:
        if should_beep:
            if channel is None or not channel.get_busy():
                channel = sound.play()
            time.sleep(0.05)
        else:
            if channel is not None and channel.get_busy():
                channel.stop()  # Stop the current beep sound immediately
            time.sleep(0.05)

async def send_data_json_over_http(name: str, value: List[str]):
    url = 'https://api.tago.io/data'
    headers = {
        "Content-Type": "application/json",
        "Device-Token": "6c5d97bd-d7f5-42b3-93db-09cca82825e8"
    }

    match name:
        case "(coords)" | "(height)":
            payload = {
                "variable": "location",
                "value": value[2],
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "metadata": {"x": value[0], "y": value[1]}
            }
        case "(distance)":
            payload = {
                "variable": "obstacle",
                "value": value[0],
                "time": datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'),
                "unit": "centimetres"
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

    async with aiohttp.ClientSession() as session:
        async with session.post(url, json=payload, headers=headers) as resp:
            await resp.text()  # you can log or ignore the result

async def main():
    global should_beep
    global last_coord
    global last_tilt
    global last_height

    beep_thread = threading.Thread(target=beep_loop, daemon=True)
    beep_thread.start()

    x_coord : float = 0
    y_coord : float = 0
    z_coord : float = 0

    dthresh = 10

    while True:
            # This waits until you type a line and press ENTER.
            # A real terminal program might put stdin in raw mode so that things
            # like CTRL+C get passed to the remote device.
            data = await asyncio.get_running_loop().run_in_executor(None, sys.stdin.buffer.readline)

            # data will be empty on EOF (e.g. CTRL+D on *nix)
            if not data:
                break

            try:
                s = data.decode("utf-8").strip()
                l = s.split()
                task = None
                if l[0] == "(coords)":
                    t = time.time()
                    if last_coord is not None and t - last_coord < 0.5:
                        continue
                    last_coord = t
                    print("coords", float(l[1]), float(l[2]))
                    x_coord = (5 + float(l[1]) * 100) / 610.0
                    y_coord = (5 + float(l[2]) * 100) / 410.0
                    task = send_data_json_over_http("(coords)", [x_coord, y_coord, z_coord])
                elif l[0] == "(height)":
                    t = time.time()
                    if last_height is not None and t - last_height < 0.5:
                        continue
                    last_height = t
                    print("height", float(l[1]))
                    z_coord = float(l[1])
                    task = send_data_json_over_http("(height)", [x_coord, y_coord, z_coord])
                elif l[0] == "(tilt)":
                    t = time.time()
                    if last_tilt is not None and t - last_tilt < 0.5:
                        continue
                    last_tilt = t
                    print("tilt", float(l[1]))
                    task = send_data_json_over_http("(tilt)", [l[1]])
                elif l[0] == "(distance)":
                    print("distance", float(l[1]))
                    task = send_data_json_over_http("(distance)", [l[1]])
                    distance = float(l[1])
                    try:
                        f = open("threshold")
                        s = f.read()
                        f.close()
                        dthresh = float(s)
                    except:
                        print("INVALID TRESHOLD FILE")
                    if distance < dthresh and distance > 0:
                        print("START BEEP")
                        should_beep = True
                    else:
                        print("STOP BEEP")
                        should_beep = False
                if task is not None:
                    asyncio.create_task(task)
                #print("info:", dthresh, last_coord, last_tilt)
            except Exception as e:
                print("exception:", e)

asyncio.run(main())
