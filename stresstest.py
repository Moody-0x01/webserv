import asyncio
import time
import random
import string
import aiohttp

# --- CONFIGURATION ---
TARGET_URL = "http://localhost:9090"  # Change to your webserver's host and port
UPLOAD_ENDPOINT = "/post"          # Route where file uploads are handled
CONCURRENT_TASKS = 50                 # Number of concurrent workers running simultaneously
TOTAL_REQUESTS_PER_WORKER = 20        # Number of cycles (Upload + Delete) each worker performs
FILE_SIZE_BYTES = 1024 * 512          # Size of the dummy file to upload (e.g., 512 KB)
# ---------------------

# Stats tracking
stats = {
    "upload_success": 0,
    "upload_fail": 0,
    "delete_success": 0,
    "delete_fail": 0,
    "exceptions": 0
}

def generate_random_string(length):
    return ''.join(random.choices(string.ascii_letters + string.digits, k=length))

async def worker(worker_id: int, session: aiohttp.ClientSession):
    """
    Each worker sequentially executes an upload followed by a delete 
    for a specified number of iterations, running concurrently with other workers.
    """
    for i in range(TOTAL_REQUESTS_PER_WORKER):
        filename = f"stress_worker_{worker_id}_file_{i}.txt"
        upload_url = f"{TARGET_URL.rstrip('/')}{UPLOAD_ENDPOINT}/{filename}"
        
        # Generate dummy data
        payload = generate_random_string(100) + "\n" + ("A" * (FILE_SIZE_BYTES - 100))
        
        # --- 1. TEST UPLOAD ---
        try:
            # Using PUT or POST depending on your webserver setup. 
            # If your server expects multipart/form-data, use the 'data' parameter with a FormData object.
            # Here we send raw binary/text data directly to test raw body parsing.
            async with session.post(upload_url, data=payload, headers={"Content-Type": "text/plain"}) as response:
                if response.status in [200, 201, 204]:
                    stats["upload_success"] += 1
                else:
                    stats["upload_fail"] += 1
                    # Skip delete if upload failed to prevent false negatives on DELETE
                    continue
        except Exception as e:
            stats["exceptions"] += 1
            stats["upload_fail"] += 1
            continue
    # for i in range(TOTAL_REQUESTS_PER_WORKER):
    #     filename = f"stress_worker_{worker_id}_file_{i}.txt"
    #     upload_url = f"{TARGET_URL.rstrip('/')}{UPLOAD_ENDPOINT}/{filename}"
    #     # Yield control briefly to simulate slight network interleaving
    #     # await asyncio.sleep(random.uniform(0.01, 0.05))
    #     # time.sleep(1)
    #     # # --- 2. TEST DELETE ---
    #     try:
    #         async with session.delete(upload_url) as response:
    #             if response.status in [200, 202, 204]:
    #                 stats["delete_success"] += 1
    #             else:
    #                 stats["delete_fail"] += 1
    #             print("code:" + str(response.status))
    #     except Exception as e:
    #         print(e)
    #         stats["exceptions"] += 1
    #         stats["delete_fail"] += 1

async def main():
    print(f"==================================================")
    print(f"🚀 Starting Heavy Stress Test on: {TARGET_URL}")
    print(f"🔥 Concurrency Level (Workers): {CONCURRENT_TASKS}")
    print(f"📦 Payload Size per Upload: {FILE_SIZE_BYTES / 1024:.2f} KB")
    print(f"🔁 Expected Total Operations: {CONCURRENT_TASKS * TOTAL_REQUESTS_PER_WORKER * 2}")
    print(f"==================================================")

    start_time = time.time()
    
    # Disable connection pooling limits to force raw socket opening stress
    connector = aiohttp.TCPConnector(limit=None, ttl_dns_cache=300)
    
    async with aiohttp.ClientSession(connector=connector) as session:
        tasks = [worker(id, session) for id in range(CONCURRENT_TASKS)]
        await asyncio.gather(*tasks)
        
    end_time = time.time()
    duration = end_time - start_time
    total_ops = stats["upload_success"] + stats["upload_fail"] + stats["delete_success"] + stats["delete_fail"]

    print(f"\n=================== RESULTS ===================")
    print(f"⏱️  Total Duration:      {duration:.2f} seconds")
    print(f"📊 Throughput:          {total_ops / duration:.2f} req/sec")
    print(f"🟢 Upload Successes:     {stats['upload_success']}")
    print(f"🔴 Upload Failures:      {stats['upload_fail']}")
    print(f"🟢 Delete Successes:     {stats['delete_success']}")
    print(f"🔴 Delete Failures:      {stats['delete_fail']}")
    print(f"⚠️  Connection Crashes:  {stats['exceptions']}")
    print(f"===============================================")
    
    if stats["upload_fail"] > 0 or stats["delete_fail"] > 0 or stats["exceptions"] > 0:
        print("🚨 Warning: Server dropped connections or returned errors during the test.")
    else:
        print("🏆 Success: Server handled the entire load smoothly without dropping requests!")

if __name__ == "__main__":
    asyncio.run(main())