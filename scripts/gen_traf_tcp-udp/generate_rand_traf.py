import asyncio
import random
import sys
import argparse
from datetime import datetime
from enum import Enum
import json
import socket
import logging
from pathlib import Path


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
    parser.add_argument(
        "--log_level", "-l",
        type=str,
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Logging level: DEBUG, INFO, WARNING, ERROR, CRITICAL (INFO by default)"
    )
    parser.add_argument(
        "--console_log", "-ncl",
        action="store_true",
        help="Disable console logging (by default console logging is enabled)"
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

    return args.quantity, sites, args.rps, args.timeout, args.max_concurrent, args.log_level, args.console_log

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

def setup_logger(flag_stream_handler, input_level_logging):
    log_dir = Path("logs")
    log_dir.mkdir(exist_ok=True)
    file_log = log_dir / f"LOG: {datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}.json"
    

    logger = logging.getLogger(__name__)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

    logger.setLevel(input_level_logging)
    


    if flag_stream_handler:
        console_handler = logging.StreamHandler()
        console_handler.setLevel(input_level_logging)
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)


    file_handler = logging.FileHandler(file_log, encoding='utf-8')
    file_handler.setLevel(input_level_logging)
    file_handler.setFormatter(formatter)
    logger.addHandler(file_handler)

    return logger, file_log

def log(quantity, rps, timeout, max_concurrent, results, logger, file_log):
     
    logger.info(f"   Запросов: {quantity}")
    logger.info(f"   RPS: {rps}")
    logger.info(f"   Таймаут: {timeout}с")
    logger.info(f"   Конкурентность: {max_concurrent}")
    logger.debug(f"   Файл результатов: {file_log}")
    
    success_count = timeout_count = error_count = fatal_error_count = 0

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
            logger.exception(f"Request to {site} - EXCEPTION: {res}")

        else:
            site, code = res
            if code.value == 0:
                status = "success"
                success_count += 1
                logger.debug(f"Request to {site} - SUCCESS")
            elif code.value == 1:
                status = "timeout"
                timeout_count += 1
                logger.error(f"Request to {site} - TIMEOUT")
            else:
                status = "error"
                error_count += 1
                logger.error(f"Request to {site} - ERROR")

            log_data["results"].append({
                    "site": site,
                    "status": status,
                    "code": code.name
                })

    log_data["statistics"] = {
        "success": success_count,
        "timeout": timeout_count,
        "error": error_count,
        "total": quantity,
        "fatal_error": fatal_error_count
    }

    try:
        with open(file_log, 'w', encoding='utf-8') as file:
            json.dump(log_data, file, indent=2, ensure_ascii=False)
        print(f"\nLogs are saved to a file: {file_log}")
    except Exception as e:
        print(f"\nError saving logs: {e}")


async def main():
    quantity, sites, rps, timeout, max_concurrent, log_level, console_log = pars()
    
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

    logger, file_log = setup_logger(console_log, log_level)
    log(quantity, rps, timeout, max_concurrent, results, logger, file_log)
    
 
if __name__ == "__main__":
    asyncio.run(main())