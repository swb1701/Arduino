"""
NeoPixel Animator code for ItsyBitsy nRF52840 NeoPixel Animation and Color Remote Control.
"""

import board
import neopixel
import time
import math
import random
from adafruit_led_animation.animation import Comet, Sparkle, AnimationGroup,\
    AnimationSequence
import adafruit_led_animation.color as color

from adafruit_ble import BLERadio
from adafruit_ble.advertising.standard import ProvideServicesAdvertisement
from adafruit_ble.services.nordic import UARTService

from adafruit_bluefruit_connect.packet import Packet
from adafruit_bluefruit_connect.color_packet import ColorPacket
from adafruit_bluefruit_connect.button_packet import ButtonPacket

# The number of NeoPixels in the externally attached strip
# If using two strips connected to the same pin, count only one strip for this number!
STRIP_PIXEL_NUMBER = 44

# Setup for comet animation
COMET_SPEED = 0.05  # Lower numbers increase the animation speed
STRIP_COMET_TAIL_LENGTH = 10  # The length of the comet on the NeoPixel strip
STRIP_COMET_BOUNCE = False  # Set to False to stop comet from "bouncing" on NeoPixel strip

# Setup for sparkle animation
SPARKLE_SPEED = 0.1  # Lower numbers increase the animation speed

# Create the NeoPixel strip
ORDER = neopixel.GRB
strip_pixels = neopixel.NeoPixel(board.D5, STRIP_PIXEL_NUMBER, auto_write=False)

# Setup BLE connection
ble = BLERadio()
uart = UARTService()
advertisement = ProvideServicesAdvertisement(uart)

# Setup animations
animations = AnimationSequence(
    AnimationGroup(
        Comet(strip_pixels, COMET_SPEED, color.TEAL, tail_length=STRIP_COMET_TAIL_LENGTH,
              bounce=STRIP_COMET_BOUNCE)
    ),
    AnimationGroup(
        Sparkle(strip_pixels, SPARKLE_SPEED, color.TEAL)
    ),
)

animation_color = color.BLUE
mode = 0
blanked = False

com_pos=STRIP_COMET_TAIL_LENGTH
com_speed=1
routine=0
routines=['updateComet','sparkle','rainbow']
rainbow_num=0

def wheel(pos):
    # Input a value 0 to 255 to get a color value.
    # The colours are a transition r - g - b - back to r.
    if pos < 0 or pos > 255:
        r = g = b = 0
    elif pos < 85:
        r = int(pos * 3)
        g = int(255 - pos*3)
        b = 0
    elif pos < 170:
        pos -= 85
        r = int(255 - pos*3)
        g = 0
        b = int(pos*3)
    else:
        pos -= 170
        r = 0
        g = int(pos*3)
        b = int(255 - pos*3)
    return (r, g, b) if ORDER == neopixel.RGB or ORDER == neopixel.GRB else (r, g, b, 0)

def rainbow():
    global rainbow_num
    for i in range(STRIP_PIXEL_NUMBER):
        pixel_index = (i * 256 // STRIP_PIXEL_NUMBER) + rainbow_num
        strip_pixels[i] = wheel(pixel_index & 255)
    strip_pixels.show()
    rainbow_num=(rainbow_num+1)%256

def updateComet():
  global com_pos
  com_delay=.01*(5-abs(com_speed))
  step=0
  if (com_speed<0):
      step=-1
  if (com_speed>0):
      step=1
  for x in range(STRIP_COMET_TAIL_LENGTH):
        strip_pixels[(com_pos-x*step)%STRIP_PIXEL_NUMBER]=animation_color
  strip_pixels[(com_pos-STRIP_COMET_TAIL_LENGTH*step)%STRIP_PIXEL_NUMBER]=color.BLACK
  strip_pixels.show()
  time.sleep(abs(com_delay))
  com_pos=(com_pos+step)%STRIP_PIXEL_NUMBER

def sparkle():
    strip_pixels.fill(color.BLACK)
    strip_pixels[random.randint(0,STRIP_PIXEL_NUMBER-1)]=animation_color
    strip_pixels.show()

while True:
    ble.start_advertising(advertisement)  # Start advertising.
    was_connected = False
    while not was_connected or ble.connected:
        if not blanked:  # If LED-off signal is not being sent...
            globals()[routines[routine]]()
            #updateComet()
            #sparkle()
            #animations.animate()  # Run the animations.
        if ble.connected:  # If BLE is connected...
            was_connected = True
            if uart.in_waiting:  # Check to see if any data is available from the Remote Control.
                try:
                    packet = Packet.from_stream(uart)  # Create the packet object.
                except ValueError:
                    continue
                if isinstance(packet, ColorPacket):  # If the packet is color packet...
                    if mode == 0:  # And mode is 0...
                        animations.color = packet.color  # Update the animation to the color.
                        print("Color:", packet.color)
                        animation_color = packet.color  # Keep track of the current color...
                    elif mode == 1:  # Because if mode is 1...
                        animations.color = animation_color  # Freeze the animation color.
                        print("Color:", animation_color)
                elif isinstance(packet, ButtonPacket):  # If the packet is a button packet...
                    if packet.pressed:  # If the buttons on the Remote Control are pressed...
                        if packet.button == ButtonPacket.LEFT:  # If button A is pressed...
                            if (com_speed>-5):
                                com_speed=com_speed-1
                            uart.write(str(com_speed)+'\n')
                            #print("A pressed: animation mode changed.")
                            #animations.next()  # Change to the next animation.
                        elif packet.button == ButtonPacket.RIGHT:  # If button B is pressed...
                            if (com_speed<5):
                                com_speed=com_speed+1
                            uart.write(str(com_speed)+'\n')
                            #mode += 1  # Increase the mode by 1.
                            #if mode == 1:  # If mode is 1, print the following:
                            #    print("B pressed: color frozen!")
                            #if mode > 1:  # If mode is > 1...
                            #    mode = 0  # Set mode to 0, and print the following:
                            #    print("B pressed: color changing!")
                        elif packet.button == ButtonPacket.UP:
                            routine=(routine+1)%len(routines)
                        elif packet.button == ButtonPacket.DOWN:
                            routine=(routine-1)%len(routines)
                            
                                
