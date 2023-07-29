# lilygo-twatch-sleepdevice
Converts a LilyGo T-Watch 2020 V3 into a sleep therapy device for positional obstructive sleep apnea (POSA) and snoring

![SleepDevice](https://github.com/a1studmuffin/lilygo-twatch-sleepdevice/assets/6295625/cdb76fb6-1317-4a38-9692-305ed1191a97)

## Background

I was recently diagnosed with positional obstructive sleep apnea (POSA) while sleeping on my back. I was presented with two treatment options: a low-tech solution (a tennis ball in a stocking tied to the upper back), or a high-tech solution (a commercial sleep therapy device worn as a collar which vibrates when the wearer is lying on their back). The second option appealed to my inner nerd, but the devices were around AUD400. So I invented a third treatment option: a DIY sleep therapy device!

I was going to roll my own hardware solution, but conveniently stumbled across the open source Lilygo T-Watch, which contained everything I needed: an ESP-based microcontroller, rechargeable battery, vibration motor and accelerometer. The OLED display was a lovely bonus, and the price was right at 60AUD. All I needed to do was remove the watch band and add a clip to attach it to the front of my shorts or t-shirt at night.

## Installation

### Software

Follow the setup guide at https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library, then download or clone this repository and build SleepDevice.ino for your target device in Arduino.

### Hardware

The LilyGo T-Watch wrist strap can be easily removed. This leaves the wifi antenna dangling from the side of the watch, but this can easily be cut off or desoldered from the PCB.

I designed and 3D printed a clip to attach the device to my clothes (included in this repository), but I'm sure you can think of other solutions if you don't have a 3D printer available. I just glued the 3D printed clip onto the back of the device.

## Usage

- Power on the device. The battery status and brief instructions are presented on the display momentarily.
- Attach the device to your clothes, with the display facing your body. I've had good results wearing the device on the front of my shorts, but you may find a t-shirt collar more comfortable.
- Every 2 minutes, the device wakes up and checks the accelerometer to determine if the wearer is on their back (assuming they've followed the above step).
- After 4 minutes of the wearer being on their back, the device will vibrate once. If the user ignores the vibration, every 2 minutes another vibration will occur, increasing in severity. After 30 minutes the device will give up, in case the user has taken off the device.
- The battery should last a few days per charge. To achieve this the ESP32 spends most of its time in deep sleep mode, and only turns on the display briefly when powered on, or when the user presses the button on the side of the watch.
