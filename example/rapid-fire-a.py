#!/usr/bin/env python3
import argparse
import serial
from time import sleep
import time

parser = argparse.ArgumentParser()
parser.add_argument('port')
args = parser.parse_args()

def send(msg, duration=0):
    # print(msg)
    ser.write(f'{msg}\r\n'.encode('utf-8'))
    sleep(duration)
    ser.write(b'RELEASE\r\n')

ser = serial.Serial(args.port, 9600)

send('Button A', 0.1)
sleep(1)
send('Button A', 0.1)
sleep(1)
send('Button A', 0.1)
sleep(1)

count = 0
start = time.time()

try:
    while True:
        send('HAT LEFT', 1/30.0)
        # sleep(1/60.0)
        send('HAT LEFT', 1/30.0)
        # sleep(1/60.0)
        send('HAT LEFT', 1/30.0)
        sleep(1)
        send('HAT RIGHT', 1/30.0)
        # sleep(1/60.0)
        send('HAT RIGHT', 1/30.0)
        # sleep(1/60.0)
        send('HAT RIGHT', 1/30.0)
        sleep(1)
        
except KeyboardInterrupt:
    send('RELEASE')
    ser.close()
