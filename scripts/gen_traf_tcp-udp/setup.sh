#!/bin/bash

set -e
sudo apt update
sudo apt install -y nmap
sudo apt install -y python3-pip
pip3 install python-nmap
