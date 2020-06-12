#!/usr/bin/env python

# WS server example

import asyncio
import websockets

import addressbook_pb2

async def hello(websocket, path):
    name = await websocket.recv()
    print(f"< {name}")

    greeting = f"Hello {name}!"

    person = addressbook_pb2.Person()
    person.id = 9999
    person.name = "John Doe"
    person.email = "jdoe@example.com"
    phone = person.phones.add()
    phone.number = "555-5555"

    await websocket.send(person.SerializeToString())
    #print(f"> {name}")
    resp = await websocket.recv()
    print(resp)

start_server = websockets.serve(hello, "127.0.0.1", 8088)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()
