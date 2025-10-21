# Voltmeter

The voltmeter part of the project, as stated in the previous section, was developed using the board's 10bit ADC. It has two operation modes which can be toggled using one of the buttons:

- High Voltage (0 - 24V Range)
- Low Voltage (0-5V Range)

Since the board's ADC can only safely accept signal up to 5V, a voltage divider was used, to bring the 24V from the 24V power supply down to the safe range. As such, measurements in High Voltage mode are not as accurate as the ones in Low Voltage mode. The error is within 14mV, so the Voltmeter is usable, but not for works where error needs to be under 1%.

In the [electrical diagram](index.md) we can see the circuit used for the Voltmeter measurement. A voltage divider consisting of two 4.7k resistors and one 2.2k resistor were used to scale down the 24V down to the safe 5V range. Two 1N5817 diodes were used to protect the board on the 5V and GND terminals. To further simulate different voltage levels, the ADC Input also goes through a B1K potentimeter.

As stated, the toggle between High Voltage and Low Voltage is done by using a button, but the 24V Power Supply *should* also be disconnected, though it is not necessary, as the voltage is already scaled to the low voltage range.