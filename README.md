# Swelldone Rudda
A Bluetooth LE steering controller for the Swelldone app. 

Uses a ESP32 and four SPST buttons to rudder left, right, change camera, and pause/resume game.

![ESP32 Schematic](/esp32/schematic.png)
![Concept2 RowErg Steerer Housing](/concept2-steerer/images/steerer-1.png)
![KayakPro PaddleErg Pedals Housing](/kayakpro-pedals/images/pedals-1.png)

## Materials
    ESP32 ESP-WROOM-32 Dev Board
    4 single pole single throw buttons
    3D printed housing for the Concept 2 Row Erg (Models D/E) or KayakPro paddle ergs

## Setup
    Standard settings for ESP32 are used for compile & download
    Written on Arduino IDE 1.8.12

## Wiring
    Hook up 4 SPST switches a ESP32 board, on pins 22 (left), 21 (right), 18 (Button 1), 17 (Button 2)
    Tie the other pole of the SPSTs to 3V3 (high)

## License
    Creative Commons Attribution 4.0 International Public License
