# Remote Sensor

The remote sensor looks for a lap trigger from the TF-Luna distance sensor or from the button. When a trigger happens, it sends its ID over LoRa and also pulses the gym timer display lines.

## What it does

- Reads distance from the TF-Luna sensor
- Lets the user trigger a lap with the button
- Sends the board ID over LoRa when a lap happens
- Controls the gym timer with output pins
- Shows simple status on the OLED display

## How the trigger works

The code checks if the sensor sees something inside the trigger distance.

If the distance is less than or equal to the trigger value, it counts as a lap trigger.

There is also a button trigger for testing or manual use.

After a trigger happens, the code waits for a short re-arm time before it can trigger again. This helps stop repeated false triggers.

## Timer behavior

On the first lap trigger, the code starts the timer.

On the next lap triggers, the code:

- stops the timer
- resets the timer
- starts the timer again

This lets the external gym timer show each lap one at a time.


## Display

The OLED screen is only used for simple info.

- At startup it shows an initializing screen
- After that it shows waiting
- After a lap trigger it shows the lap count
