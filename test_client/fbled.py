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
import threading
from time import sleep


PORT = 23


class FBLed(object):
    def __init__(self, ipaddr, xres, yres, num_channels):
        # Set necessary vars
        self.ipaddr = ipaddr
        self.XRES = xres
        self.YRES = yres
        self.CHANNELS = num_channels
        self.PERIOD = (xres * yres / num_channels) / 20480.0
        self.framebuffer = [[bytearray(3) for y in range(self.YRES)]
                            for x in range(self.XRES)]
        self.socket = None

    def start(self):
        # Init connection to uC
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((self.ipaddr, PORT))
        n = struct.pack("I", self.XRES * self.YRES / self.CHANNELS)
        self.socket.send(n)

        # Start sending frames
        self.blitter = threading.Thread(target = self.__send_data__)
        self.blitter.daemon = True
        self.blitter.start()

    def stop(self):
        if self.socket is not None:
            self.socket.shutdown(socket.SHUT_RDWR)
            self.socket.close()
            self.socket = None

    def clear(self, color=(0,0,0)):
        for i in xrange(self.XRES):
            for j in xrange(self.YRES):
                self.framebuffer[i][j] = bytearray(color)

    def __px_to_bytes__(self, rgbarr):
        '''
        Transform an array of up to 8 RGB (255, 255, 255) pixel values
        (one per channel) to the binary format accepted by the
        microcontroller.
        '''
        v12bit = [(((rgb[0]/16) << 8) | ((rgb[1]/16) << 4) | rgb[2]/16)
                  for rgb in rgbarr]
        barr = bytearray(12)
        for chan in range(self.CHANNELS):
            for i in range(11, -1, -1):
                barr[i] |= (((v12bit[chan] >> i) & 1) << chan)
        return barr

    def __fb_to_bytes__(self):
        '''
        Transform the self.framebuffer to the binary format accepted by the
        microcontroller, and return the corresponding bytearray.
        '''
        barr = bytearray()
        for i in range(self.XRES / self.CHANNELS):
            if i % 2: y = range(self.YRES-1, -1, -1)
            else: y = range(self.YRES)
            for j in y:
                px = [self.framebuffer[i + (chan*self.XRES/self.CHANNELS)][j]
                      for chan in range(self.CHANNELS)]
                bytes = self.__px_to_bytes__(px)
                barr += bytes
        return barr

    def __send_data__(self):
        while self.socket is not None:
            sleep(self.PERIOD)
            self.socket.send(self.__fb_to_bytes__())

    def __del__(self):
        self.stop()
