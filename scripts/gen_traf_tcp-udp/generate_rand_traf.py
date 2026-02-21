import asyncio
import random
import sys
import argparse
from datetime import datetime
from enum import Enum
import json
import socket



class ResultFunction(Enum):
    TIME_EXCEEDED = 1
    REQUEST_COMPLETED = 0
    ERROR_EXECUTING_SCRIPT = -1

def pars():
    parser = argparse.ArgumentParser(
        description="Traffic generator with RPS control and timeouts",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "--quantity", "-q",
        type=int,
        default=10,
        help="Number of requests (10 by default)"
    )
    parser.add_argument(
        "--rps", "-r",
        type=int,
        default=10,
        help="The desired number of requests per second (10 by default)"
    )
    parser.add_argument(
        "--timeout", "-t",
        type=int,
        default=5,
        help="Timeout per request in seconds (5 by default)"
    )
    parser.add_argument(
        "--file", "-f",
        type=str,
        default="sites.txt",
        help="A file with a list of sites (by default sites.txt )"
    )
    parser.add_argument(
        "--max_concurrent", "-m",
        type=int,
        default=50,
        help="Maximum number of simultaneous tasks (50 by default)"
    )

    args = parser.parse_args()

    if args.quantity <= 0:
        print("Error: the number of requests must be a positive number, using the default value of 10")
        args.quantity = 10
    if args.rps <= 0:
        print("Error: RPS must be a positive number, using the default value of 10")
        args.rps = 10
    if args.timeout <= 0:
        print("Error: the timeout must be a positive number, using the default value of 5")
        args.timeout = 5
    if args.max_concurrent <= 0:
        print("Error: the number of tasks being completed at the same time must be a positive number, using the default value of 50")
        args.max_concurrent = 50

    try:
        with open(args.file, 'r') as f:
            sites = [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        print(f"Error: file '{args.file}' not found")
        sys.exit(1)

    if not sites:
        print(f"Error: file '{args.file}' is empty")
        sys.exit(1)

    return args.quantity, sites, args.rps, args.timeout, args.max_concurrent

async def check_one(site, timeout):
    try:
        # process_udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # process_tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        process = await asyncio.create_subprocess_exec('./generate_traf.sh', '1',\
                                                        site, stdout=asyncio.subprocess.DEVNULL, stderr=asyncio.subprocess.DEVNULL)  
        await asyncio.wait_for(process.communicate(), timeout=timeout)
        if process.returncode == 0:
            return site, ResultFunction.REQUEST_COMPLETED
        return site, ResultFunction.ERROR_EXECUTING_SCRIPT
    
    except asyncio.TimeoutError:
        process.kill()
        await process.wait()
        return site, ResultFunction.TIME_EXCEEDED

def log(quantity, rps, timeout, max_concurrent, results):
    success_count = 0
    timeout_count = 0
    error_count = 0
    fatal_error_count = 0

    log_data = {
        "parameters": {
            "quantity": quantity,
            "rps": rps,
            "timeout": timeout,
            "max_concurrent": max_concurrent
        },
        "results": []
    }

    for res in results:
        if isinstance(res, Exception):
            fatal_error_count += 1
            log_data["results"].append({
                "site": "unknown",
                "status": "exception",
                "details": str(res)
            })
        else:
            site, code = res
            if code == 0:
                status = "success"
                success_count += 1
            elif code == 1:
                status = "timeout"
                timeout_count += 1
            else:
                status = "error"
                error_count += 1

            log_data["results"].append({
                    "site": site,
                    "status": status,
                    "code": code
                })

    log_data["statistics"] = {
        "success": success_count,
        "timeout": timeout_count,
        "error": error_count,
        "total": quantity,
        "fatal_error": fatal_error_count
    }

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"log_{timestamp}.json"

    try:
        with open(filename, 'w', encoding='utf-8') as file:
            json.dump(log_data, file, indent=2, ensure_ascii=False)
        print(f"\nLogs are saved to a file: {filename}")
    except Exception as e:
        print(f"\nError saving logs: {e}")


async def main():
    quantity, sites, rps, timeout, max_concurrent = pars()
    
    semaphore = asyncio.Semaphore(max_concurrent)
    delay = 1.0 / rps

    tasks = []
    for _ in range(quantity):
        site = random.choice(sites)
        async def task_wrapper(certain_site=site):
            async with semaphore:
                return await check_one(certain_site, timeout)
        
        task = asyncio.create_task(task_wrapper())
        tasks.append(task)
        
        await asyncio.sleep(delay)
    
    results = await asyncio.gather(*tasks, return_exceptions=True)

    log(quantity, rps, timeout, max_concurrent, results)
    
 
if __name__ == "__main__":
    asyncio.run(main())