# Voltmeter

The voltmeter part of the project, as stated in the previous section, was developed using the board's 10bit ADC. It has two operation modes which can be toggled using one of the buttons:

- High Voltage (0 - 24V Range)
- Low Voltage (0 - 5V Range)

Since the board's ADC can only safely accept signal up to 5V, a voltage divider was used, to bring the 24V from the 24V power supply down to the safe range. As such, measurements in High Voltage mode are not as accurate as the ones in Low Voltage mode.

In the electrical diagram found on the [home page](index.md) we can see the circuit used for the Voltmeter measurement. A voltage divider using a 4.8k resistors and a B1K potentiometer were used to scale down the 24V down to the safe 5V range and to allow for different voltage values. Two 1N5817 diodes were used to protect the board on the 5V and GND terminals. To further simulate different voltage levels, the ADC Input also goes through a B1K potentimeter.

As stated, the toggle between High Voltage and Low Voltage is done by using the voltage button, but the change is just a software one. Because we need to scale down the voltage before feeding it to the ADC's input, we can choose to use it as is, in the 0-5V range, which gives us the low voltage mode, or scale it back up after measurements, to get the "actual" voltage. This voltage can be turned up or down artifically through the usage of the B1K potentiometer.