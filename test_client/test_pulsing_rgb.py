###############################################################################
#
# Copyright (c) 2011, 2013 - Tony Gonçalves.
# All rights reserved.
#
# This file is part of Lightbox.
#
# Lightbox is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Lightbox is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Lightbox.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################


import socket
import struct
import timeit
from time import sleep
import threading

RED = bytearray(b'\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00')
GREEN = bytearray(b'\x00\x00\x00\x00\xff\xff\xff\xff\x00\x00\x00\x00')
BLUE = bytearray(b'\x00\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff')
colors = [RED, GREEN, BLUE]

LEDS_PER_CHAN = 30
host = '10.0.0.55'
port = 23

framebuffer = bytearray(b'\x00') * 12 * LEDS_PER_CHAN
#framebuffer = GREEN * LEDS_PER_CHAN

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((host,port))
n = struct.pack("I", LEDS_PER_CHAN)
s.send(n)


def bit12_to_bytes(val):
    barr = bytearray(12)
    for i in range(11, -1, -1):
        if ((val >> i) & 1):
            barr[i] = 1
        else:
            barr[i] = 0
    return barr


def send_data():
    while 1:
        sleep(.02)
        s.send(framebuffer)


def upd_fb():
    direction = -1
    cur_val = 0
    shift = 0
    while 1:
        sleep(.05)
        if cur_val == 0:
            shift = (shift + 4) % 12
        if cur_val <= 0 or cur_val >= 15:
            direction = -direction
        cur_val += direction
        # arr = bit12_to_bytes(cur_val << 8 | cur_val << 4 | cur_val)
        arr = bit12_to_bytes(cur_val << shift)
        for i in range(LEDS_PER_CHAN):
            for j in range(12):
                framebuffer[i*12+j] = arr[j]
t = threading.Thread(target=upd_fb)
t.daemon = True


#Enviar cenas maradas
t.start()
send_data()
s.close()
t.stop()
t.join()
