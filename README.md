# Chicken Coop Opener
Home Assistant Chicken Coop Opener. Designed to run:

- Wemos D1 Mini - ESP8266
- L298N Motor Driver
- 12v DC Power Supply
- 12v DC Geared Motor with Encoder - https://www.aliexpress.com/item/4000098341909.html?spm=a2g0s.9042311.0.0.40694c4dVEUa5w
- Reed Switches
- Home Assistant with MQTT

- L298N 12v input
- Pin configuration. Pins D7(Wemos 13) (L298N IN1) and D8(Wemos 15) (L298N IN2) are used for controlling the motor’s direction
- Pin configuration. D1(Wemos 5) (Motor Green) are used for reading the encoder
- L298N Motor A (Motor Red + and Motor White -)
- Pin configuration. Pins VIN (L298N +5V), VCC 3.3v (Motor Blue) and GND (Motor Black)

https://youtu.be/txGxSdKSX40
