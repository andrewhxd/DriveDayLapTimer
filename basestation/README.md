# Basestation

The basestation waits for LoRa packets from the remote sensor. When it gets a packet, it checks the sender ID and uses that to keep track of laps and segments.

## What it does

- Starts the LoRa radio in receive mode
- Waits for packets from the remote sensor
- Uses the first ID it sees as the lap marker
- When that same ID comes back again, it counts 1 lap
- Prints lap info to the serial monitor
- Shows basic info on the OLED display

* for acceleration testing, the basestation will handle pulsing the timer

## How the lap logic works

The first packet that comes in is saved as the start/finish marker.

If the basestation sees that same ID again, it means a lap finished. The code measures the time between the two packets and saves it as the last lap time.

If a different ID comes in, it is treated like a segment marker during the current lap.

## Notes

The display only updates when a packet from the lap marker is received. This is to reduce the effects of the display buffer updates being blocking and preventing the reception of packets.
* update after receive (listen after send)
