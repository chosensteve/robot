from machine import Pin, ADC, PWM
from time import sleep_us

s0 = Pin(2, Pin.OUT)
s1 = Pin(3, Pin.OUT)
s2 = Pin(4, Pin.OUT)
s3 = Pin(5, Pin.OUT)
sig = ADC(26)

in1 = Pin(6, Pin.OUT)
in2 = Pin(7, Pin.OUT)
ena = PWM(Pin(8))
in3 = Pin(9, Pin.OUT)
in4 = Pin(10, Pin.OUT)
enb = PWM(Pin(11))

ena.freq(1000)
enb.freq(1000)

Kp = 30
Ki = 0
Kd = 0.2
base_speed = 70000
last_error = 0
total_error = 0
ninety_time = 300000

def read_mux(ch):
    s0.value(ch & 1)
    s1.value((ch >> 1) & 1)
    s2.value((ch >> 2) & 1)
    s3.value((ch >> 3) & 1)
    sleep_us(100)
    return sig.read_u16()

def read_line():
    vals = [read_mux(i) for i in range(5)]
    levels = [1 if v < 30000 else 0 for v in vals]
    total = 0
    count = 0
    for i, lvl in enumerate(levels):
        total += i * lvl
        count += lvl
    if count == 0:
        return 2, levels
    return total / count, levels

def set_motor(l, r):
    l = max(-65535, min(65535, int(l)))
    r = max(-65535, min(65535, int(r)))
    in1.value(0); in2.value(1)
    in3.value(0); in4.value(1)
    ena.duty_u16(l)
    enb.duty_u16(r)

while True:
    pos, levels = read_line()
    error = 2 - pos
    total_error += error

    if levels[0] == 1 and sum(levels[1:]) == 0:
        set_motor(65000, -65535)  
        sleep_us(ninety_time)         
        continue
    elif levels[4] == 1 and sum(levels[:4]) == 0:
        set_motor(-65535, 65000)  
        sleep_us(ninety_time)
        continue

    corr = (Kp * error) + (Ki * total_error) + (Kd * (error - last_error))
    left = base_speed + (corr * 2000)
    right = base_speed - (corr * 2000)

    set_motor(left, right)
    last_error = error
    sleep_us(5000)
