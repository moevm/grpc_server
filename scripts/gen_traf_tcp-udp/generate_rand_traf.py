import asyncio
import random
import sys
import argparse
from datetime import datetime
import json
import logging
from pathlib import Path
import nmap
import requests
from bs4 import BeautifulSoup
import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


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
        default=15,
        help="The desired number of requests per second (15 by default)"
    )
    parser.add_argument(
        "--timeout", "-t",
        type=int,
        default=20,
        help="Timeout per request in seconds (20 by default)"
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
        "--no_console_log", "-ncl",
        action="store_true",
        help="Disable console logging (by default console logging is enabled)"
    )
    parser.add_argument(
        "--name_folder_log", "-n",
        type=str,
        default="logs",
        help="A name of folder logs(by default logs)"
    )

    args = parser.parse_args()

    if args.quantity <= 0:
        print("Error: the number of requests must be a positive number, using the default value of 10")
        args.quantity = 10
    if args.rps <= 0:
        print("Error: RPS must be a positive number, using the default value of 15")
        args.rps = 15
    if args.timeout <= 0:
        print("Error: the timeout must be a positive number, using the default value of 20 - the optimal time for analyzing a compound is at standard values.")
        args.timeout = 20
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

    return args.quantity, sites, args.rps, args.timeout, args.max_concurrent, args.log_level, args.no_console_log, args.name_folder_log

async def check_one(site, timeout):
    scan_timeout = max(2, timeout - 2)
    
    result = {
        'site': site,
        'tcp_ports': {},
        'udp_ports': {},
        'status': 'unknown',
        'ip': None
    }
    
    try:
        nm = nmap.PortScanner()
        
        await asyncio.wait_for(
            asyncio.to_thread(
                nm.scan,
                site, 
                '53,80,443,123,161',
                f'-sS -sU -T4 --host-timeout {scan_timeout}s'
            ),
            timeout
        )
        
        if nm.all_hosts():
            result['ip'] = nm.all_hosts()[0]
            
            for proto in nm[result['ip']].all_protocols():
                for port in nm[result['ip']][proto].keys():
                    state = nm[result['ip']][proto][port]['state']
                    service = nm[result['ip']][proto][port].get('name', 'unknown')
                    
                    if state == 'open':
                        if proto == 'tcp':

                            result['tcp_ports'][port] = {
                                'state': state,
                                'service': service
                            }

                            if port == 80 or port == 443:
                                protocol = 'https' if port == 443 else 'http'
                                url = f"{protocol}://{site}:{port}"
                                response = requests.get(url, timeout=5, verify=False)
                                status_code = response.status_code
                                soup = BeautifulSoup(response.text, 'html.parser')
                                title = soup.find('title').text if soup.find('title') else None
                                
                                result['tcp_ports'][port]['http_status'] = status_code
                                result['tcp_ports'][port]['title'] = title
                            
                            
                        elif proto == 'udp':
                            result['udp_ports'][port] = {
                                'state': state,
                                'service': service
                            }
        
        if result['tcp_ports'] or result['udp_ports']:
            result['status'] = 'success'
        else:
            result['status'] = 'no_open_ports'
        
    except asyncio.TimeoutError:
        result['status'] = 'timeout'
    
    except Exception as e:
        result['status'] = 'error'
        result['error'] = str(e)


    
    
    return result
     

def setup_logger(flag_stream_handler, input_level_logging, name_folder_log):
    log_dir = Path(name_folder_log)
    log_dir.mkdir(exist_ok=True)
    file_log = log_dir / f"LOG_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S-%f')}.json"
    

    logger = logging.getLogger(__name__)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')

    logger.setLevel(input_level_logging)
    


    if not flag_stream_handler:
        console_handler = logging.StreamHandler()
        console_handler.setLevel(input_level_logging)
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)


    return logger, file_log

def log(quantity, rps, timeout, max_concurrent, results, logger, file_log):
     
    logger.info(f"   Requests: {quantity}")
    logger.info(f"   RPS: {rps}")
    logger.info(f"   Timeout: {timeout}с")
    logger.info(f"   Max concurrent processes: {max_concurrent}")
    logger.debug(f"   The results file: {file_log}")
    
    no_ports_count = success_count = timeout_count = error_count = 0

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
            log_data["results"].append({
                "site": "unknown",
                "status": "exception",
                "details": str(res)
            })
            logger.exception(f"Request - EXCEPTION: {res}")

        else:
            site_data = res
            
            log_entry = {
                "site": site_data['site'],
                "ip": site_data['ip'],
                "status": site_data['status'],
                "tcp_ports": site_data['tcp_ports'],
                "udp_ports": site_data['udp_ports']
            }
            
            if 'error' in site_data:
                log_entry["error"] = site_data['error']
            
            log_data["results"].append(log_entry)
            
            if site_data['status'] == 'success':
                success_count += 1
                logger.debug(f"Request to {site_data['site']} - SUCCESS")
            elif site_data['status'] == 'timeout':
                timeout_count += 1
                logger.error(f"Request to {site_data['site']} - TIMEOUT")
            elif site_data['status'] == 'no_open_ports':
                no_ports_count += 1
                logger.warning(f"Request to {site_data['site']} - NO OPEN PORTS")
            else:
                error_count += 1
                logger.error(f"Request to {site_data['site']} - ERROR")


    log_data["statistics"] = {
        "no_ports_count" : no_ports_count,
        "success": success_count,
        "timeout": timeout_count,
        "error": error_count,
        "total": quantity
    }

    try:
        with open(file_log, 'w', encoding='utf-8') as file:
            json.dump(log_data, file, indent=2, ensure_ascii=False)
        print(f"\nLogs are saved to a file: {file_log}")
    except Exception as e:
        print(f"\nError saving logs: {e}")


async def main():
    quantity, sites, rps, timeout, max_concurrent, log_level, console_log, name_folder_log = pars()
    
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

    logger, file_log = setup_logger(console_log, log_level, name_folder_log)
    log(quantity, rps, timeout, max_concurrent, results, logger, file_log)
    
 
if __name__ == "__main__":
    asyncio.run(main())