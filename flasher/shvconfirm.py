#!/usr/bin/env python3

# Copyright: Michal Lenc 2025 <michallenc@seznam.cz>
#            Stepan Pressl 20025 <pressl.stepan@gmail.com>
from shv import RpcUrl, ValueClient


async def shv_confirm(connection: str, path_to_root: str) -> None:
    url = RpcUrl.parse(connection)
    client = await ValueClient.connect(url)
    assert client is not None

    await client.call(f"{path_to_root}/fwStable", "confirm")

    print("FW confirmed.")
    await client.disconnect()
