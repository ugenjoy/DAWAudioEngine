#!/usr/bin/env python3
"""
Test script for save/load project functionality via WebSocket
"""

import asyncio
import websockets
import json
import sys
import os

async def test_save_load():
    uri = "ws://localhost:8080/ws"

    try:
        async with websockets.connect(uri) as websocket:
            print("✓ Connected to WebSocket server")

            # Test 1: Save project
            test_project_path = "/tmp/test_project.dawproj"
            save_message = {
                "type": "action",
                "payload": {
                    "action": "saveProject",
                    "path": test_project_path
                }
            }

            print(f"\n📝 Sending saveProject command...")
            print(f"   Path: {test_project_path}")
            await websocket.send(json.dumps(save_message))

            response = await websocket.recv()
            save_response = json.loads(response)
            print(f"   Response: {json.dumps(save_response, indent=2)}")

            if save_response.get("status") == "ok":
                print("   ✓ Save successful!")

                # Check if files were created
                if os.path.exists(test_project_path):
                    print(f"   ✓ Project folder exists")

                    project_json_path = os.path.join(test_project_path, "project.json")
                    if os.path.exists(project_json_path):
                        print(f"   ✓ project.json exists")

                        # Read and display the saved JSON
                        with open(project_json_path, 'r') as f:
                            saved_data = json.load(f)
                            print(f"\n📄 Saved project.json content:")
                            print(json.dumps(saved_data, indent=2))
                    else:
                        print(f"   ✗ project.json not found!")
                else:
                    print(f"   ✗ Project folder not created!")
            else:
                print(f"   ✗ Save failed: {save_response.get('message')}")
                return

            # Test 2: Load project
            print(f"\n📂 Sending loadProject command...")
            load_message = {
                "type": "action",
                "payload": {
                    "action": "loadProject",
                    "path": test_project_path
                }
            }

            await websocket.send(json.dumps(load_message))
            response = await websocket.recv()
            load_response = json.loads(response)
            print(f"   Response: {json.dumps(load_response, indent=2)}")

            if load_response.get("status") == "ok":
                print("   ✓ Load successful!")
            else:
                print(f"   ✗ Load failed: {load_response.get('message')}")

            print("\n✅ All tests completed!")

    except websockets.exceptions.ConnectionRefused:
        print("✗ Connection refused. Is the backend running?")
        sys.exit(1)
    except Exception as e:
        print(f"✗ Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    print("=== DAW Project Save/Load Test ===\n")
    asyncio.run(test_save_load())
