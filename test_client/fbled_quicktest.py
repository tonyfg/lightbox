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


from fbled import FBLed
from time import sleep

black = bytearray(3)
pink = bytearray([255, 105, 180])
white = bytearray([255, 255, 255])

red = bytearray([255, 0, 0])
green = bytearray([0, 255, 0])
blue = bytearray([0, 0, 255])


ball_x = 4
ball_y = 1
ball_velx = 1
ball_vely = 1

fbled = FBLed("10.0.0.55", 6, 5, 1)
fbled.start()
fb = fbled.framebuffer

while True:
    sleep(.1)
    fbled.clear()
    if ball_x == 0 or ball_x == 5:
        ball_velx = -ball_velx
    if ball_y == 0 or ball_y == 4:
        ball_vely = -ball_vely
    ball_x += ball_velx
    ball_y += ball_vely
    fb[ball_x][ball_y] = pink
