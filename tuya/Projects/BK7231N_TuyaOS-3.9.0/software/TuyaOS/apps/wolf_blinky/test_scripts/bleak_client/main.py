import os
import asyncio
import base64
import json
from bleak import BleakClient

# Replace with your BLE device address and characteristic UUID
DEVICE_ADDRESS = "C8:47:8C:00:00:01"
CHARACTERISTIC_UUID = "0000e301-0000-1000-8000-00805f9b34fb"  # UUID for 0xE301
DATA_FOLDER = "provisioning_data_folder"
DATA_FILE = "100004961.json"

async def send_large_json(client, json_data, chunk_size):
    # Convert JSON to a Base64-encoded string
    encoded_data = base64.b64encode(json.dumps(json_data).encode()).decode()
    total_len = len(encoded_data)

    # Split Base64 data into chunks
    for sequence, start in enumerate(range(0, total_len, chunk_size)):
        chunk_data = encoded_data[start:start + chunk_size]
        payload = {
            "total_len": total_len,
            "len": len(chunk_data),
            "sequence": sequence,
            "data": chunk_data
        }
        payload_bytes = json.dumps(payload).encode()

        try:
            await client.write_gatt_char(CHARACTERISTIC_UUID, payload_bytes)
            print(f"Chunk {sequence} sent: {payload}")
        except Exception as e:
            print(f"Failed to send chunk {sequence}: {e}")
            break

async def main():
    # Path to the JSON file
    file_path = os.path.join(DATA_FOLDER, DATA_FILE)
    
    # Check if the file exists
    if not os.path.exists(file_path):
        print(f"File not found: {file_path}")
        return
    
    # Read the JSON data from the file
    with open(file_path, "r") as f:
        json_data = json.load(f)

    # Chunk size in bytes
    chunk_size = 180

    # Connect to the BLE device
    async with BleakClient(DEVICE_ADDRESS) as client:
        print(f"Connected to {DEVICE_ADDRESS}")
        
        # Request maximum MTU size
        try:
            mtu = await client.request_mtu(247)  # Request maximum MTU size
            print(f"MTU size set to: {mtu}")
        except Exception as e:
            print(f"Failed to set MTU size: {e}")
        
        # Send JSON data in chunks
        await send_large_json(client, json_data, chunk_size)

# Run the asynchronous function
asyncio.run(main())
