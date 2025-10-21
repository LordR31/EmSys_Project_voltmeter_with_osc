# Buttons

There are 4 physical buttons and 4 touchscreen buttons. In this section we are only talking about the physical buttons. The other 4 buttons will be discussed inside the [Touchscreen section](display.md) section of the documentation.

All physical buttons are configured to work with interrupts in falling edge mode. As such, they are all connected to a GPIO pin and to the board's GND.

The 4 physical buttons are:

- System Start
- Voltage Mode Toggle
- Cursor Left
- Cursor Right

## System Start

Connection: PE4 (Pin 2)

This button is used to start and stop the ADC.

## Voltage Mode Toggle

Connection: PD3 (Pin 18)

As explained in the previous section, this button toggles the Voltmeter mode from High Voltage to Low Voltage and vice-versa

# Cursor Left

Connection: PD0 (Pin 21)

When the Waveform Viewer is active and the cursor is turned on, this button moves the cursor to the previous recorded point.

# Cursor Left

Connection: PD1 (Pin 20)

When the Waveform Viewer is active and the cursor is turned on, this button moves the cursor to the next recorded point. 