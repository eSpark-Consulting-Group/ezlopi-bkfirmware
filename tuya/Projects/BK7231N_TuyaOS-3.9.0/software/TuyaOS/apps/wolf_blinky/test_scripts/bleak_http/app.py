import os
import json
import base64
from fastapi import FastAPI, File, UploadFile, Form, HTTPException, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
from bleak import BleakClient
import asyncio

# Setup FastAPI
app = FastAPI()

# Setup Jinja2 template rendering
templates = Jinja2Templates(directory="templates")

# Serve static files (e.g., CSS)
app.mount("/static", StaticFiles(directory="static"), name="static")


async def send_json_directly(client, json_data, characteristic_uuid):
    """Send JSON data directly without chunking."""
    payload_bytes = json.dumps(json_data).encode()
    try:
        await client.write_gatt_char(characteristic_uuid, payload_bytes)
        print(f"Data sent directly: {json_data}")
    except Exception as e:
        print(f"Failed to send data: {e}")
        raise e


async def connect_and_send_data(device_address, characteristic_uuid, json_data, chunk=False):
    """Connect to BLE device and send data."""
    try:
        async with BleakClient(device_address) as client:
            print(f"Connected to {device_address}")
            if chunk:
                try:
                    mtu = await client.request_mtu(247)
                    print(f"MTU size set to: {mtu}")
                except Exception as e:
                    print(f"Failed to set MTU size: {e}")
                await send_large_json(client, json_data, chunk_size=180, characteristic_uuid=characteristic_uuid)
            else:
                await send_json_directly(client, json_data, characteristic_uuid)
    except Exception as e:
        print(f"Error during BLE operation: {e}")
        raise e


async def send_large_json(client, json_data, chunk_size, characteristic_uuid):
    """Send JSON data in chunks if it exceeds BLE payload size."""
    encoded_data = base64.b64encode(json.dumps(json_data).encode()).decode()
    total_len = len(encoded_data)

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
            await client.write_gatt_char(characteristic_uuid, payload_bytes)
            print(f"Chunk {sequence} sent: {payload}")
        except Exception as e:
            print(f"Failed to send chunk {sequence}: {e}")
            raise e


@app.get("/", response_class=HTMLResponse)
async def read_root(request: Request):
    """Serve the upload page."""
    return templates.TemplateResponse("upload.html", {"request": request})


@app.get("/favicon.ico")
async def favicon():
    """Return 404 for missing favicon."""
    return JSONResponse(content={"detail": "Not Found"}, status_code=404)


@app.post("/upload/")
async def upload_file(file: UploadFile = File(...), mac_address: str = Form(...)):
    """Upload and send JSON file to BLE device."""
    if not mac_address:
        raise HTTPException(status_code=400, detail="MAC address is required")

    try:
        contents = await file.read()
        json_data = json.loads(contents.decode())
    except Exception as e:
        raise HTTPException(status_code=400, detail="Invalid JSON file")

    characteristic_uuid = "0000e301-0000-1000-8000-00805f9b34fb"
    try:
        await connect_and_send_data(mac_address, characteristic_uuid, json_data, chunk=True)
        return JSONResponse(content={"message": "Data sent successfully"}, status_code=200)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to send data: {str(e)}")


@app.post("/set_wifi/")
async def set_wifi(
    mac_address: str = Form(...),
    wifi_ssid: str = Form(...),
    wifi_password: str = Form(...),
    user_id: str = Form(...)
):
    """Set WiFi credentials on BLE device."""
    if not mac_address:
        raise HTTPException(status_code=400, detail="MAC address is required")

    wifi_data = {
        "wifi_ssid": wifi_ssid,
        "wifi_password": wifi_password,
        "user_id": user_id
    }

    characteristic_uuid = "0000e302-0000-1000-8000-00805f9b34fb"
    try:
        await connect_and_send_data(mac_address, characteristic_uuid, wifi_data, chunk=False)
        return JSONResponse(content={"message": "WiFi credentials sent successfully"}, status_code=200)
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Failed to send WiFi credentials: {str(e)}")


# Run the FastAPI server
if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
