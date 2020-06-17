#!/usr/bin/env python

# WS server example
import time
import asyncio
import websockets

import addressbook_pb2
import epoch_pb2


def add_devices(macs):
    epoch_evnt = epoch_pb2.EpochStep()
    for mac in macs:
        epoch_evnt.enter_l2devices.append(mac)

    return epoch_evnt

def new_random_epoch(mac_src):
    epoch_evnt = epoch_pb2.EpochStep()

    comm = epoch_evnt.l2_dev_comm.add()
    comm.mac_src = mac_src

    tx = comm.tx_summary.add()
    tx.mac_dst = 2
    tx.ipv4 = 10

    return epoch_evnt

async def publish_events(websocket, path):
    ee = add_devices([1, 2])
    await websocket.send(ee.SerializeToString())

    while(True):
        ef = new_random_epoch(1)
        time.sleep(0.014)
        await websocket.send(ef.SerializeToString())

    print("Sent everything")


start_server = websockets.serve(publish_events, "127.0.0.1", 8088)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
