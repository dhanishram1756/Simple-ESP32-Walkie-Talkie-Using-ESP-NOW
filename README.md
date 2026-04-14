# Simple-ESP32-Walkie-Talkie-Using-ESP-NOW
A simple ESP-32 Walkie-Talkie using inbuilt ESP-NOW protocol that can be made using minimal parts. Feel free to give suggestions on improving it.

Parts required:
1. 8 ohm speakers
2. PAM 8403 amplifier
3. ESP-32 DEVKIT
4. MAX4466 electret microphone
5. Push-to-Talk button
6. some connecting wires or just solder it. your choice.

Procedure:
1. connect PTT button's one corner to ground pin and the corner that's diagonal to that grnd pin to GPIO32.
2. connect Vcc of amplifier and MAX4466 mic to Vcc.
3. connect Grnd of microphone and amplifier to GND pin on the board.
4. connect microphone OUT to GPIO34.
5. connect amplifier L IN to GPIO25.
6. connect speaker '+' to L out+ and speaker '-' to L out-.
7. connect a microusb with power bank or let it have a Li-ion battery with a charging module to be safe.
8. ENJOY!

procedure in code:
In the code, make sure to have the MAC address of walkie-1 on walkie-2 and vice-versa, so they can acknowledge each other to communicate properly.
Also, change the walkie name that has 'A' into 'B' for the walkie-B.
