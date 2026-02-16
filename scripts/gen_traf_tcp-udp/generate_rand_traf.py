import asyncio
import random
import sys
import argparse
from datetime import datetime
import json


def check():
    parser = argparse.ArgumentParser(
        description="Генератор трафика с контролем RPS и таймаутами",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "--quantity", "-q",
        type=int,
        default=10,
        help="Количество запросов (обязательный параметр)"
    )
    parser.add_argument(
        "--rps", "-r",
        type=int,
        default=10,
        help="Желаемое количество запросов в секунду (по умолчанию 10)"
    )
    parser.add_argument(
        "--timeout", "-t",
        type=int,
        default=5,
        help="Таймаут на один запрос в секундах (по умолчанию 5)"
    )
    parser.add_argument(
        "--file", "-f",
        type=str,
        default="sites.txt",
        help="Файл со списком сайтов (по умолчанию sites.txt)"
    )
    parser.add_argument(
        "--max_concurrent", "-m",
        type=int,
        default=50,
        help="Максимальное количество одновременно выполеняемых задач (по умолчанию 50)"
    )

    args = parser.parse_args()

    if args.quantity <= 0:
        print("Ошибка: количество запросов должно быть положительным числом")
        sys.exit(1)
    if args.rps <= 0:
        print("Ошибка: RPS должно быть положительным числом, используем значение по умолчанию 10")
        args.rps = 10
    if args.timeout <= 0:
        print("Ошибка: таймаут должен быть положительным числом, используем значение по умолчанию 5")
        args.timeout = 5
    if args.max_concurrent <= 0:
        print("Ошибка: количество одновременно выполеняемых задач должно быть положительным числом, используем значение по умолчанию 50")
        args.max_concurrent = 50

    try:
        with open(args.file, 'r') as f:
            sites = [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        print(f"Ошибка: файл '{args.file}' не найден")
        sys.exit(1)

    if not sites:
        print(f"Ошибка: файл '{args.file}' пуст")
        sys.exit(1)

    return args.quantity, sites, args.rps, args.timeout, args.max_concurrent


async def check_one(site, timeout):
    try:
        process = await asyncio.create_subprocess_exec('./generate_traf.sh', '1',\
                                                        site, stdout=asyncio.subprocess.DEVNULL, stderr=asyncio.subprocess.DEVNULL)  
        await asyncio.wait_for(process.communicate(), timeout=timeout)
        if process.returncode == 0:
            return site, 0
        return site, -1 
    
    except asyncio.TimeoutError:
        process.kill()
        await process.wait()
        return site, 1

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
        print(f"\nЛоги сохранёны в файл: {filename}")
    except Exception as e:
        print(f"\nОшибка при сохранении логов: {e}")


async def main():
    quantity, sites, rps, timeout, max_concurrent = check()
    
    semaphore = asyncio.Semaphore(max_concurrent)
    delay = 1.0 / rps

    tasks = []
    for _ in range(quantity):
        site = random.choice(sites)
        async def task_wrapper():
            async with semaphore:
                return await check_one(site, timeout)
        
        task = asyncio.create_task(task_wrapper())
        tasks.append(task)
        
        await asyncio.sleep(delay)
    
    results = await asyncio.gather(*tasks, return_exceptions=True)

    log(quantity, rps, timeout, max_concurrent, results)
    
 
if __name__ == "__main__":
    asyncio.run(main())