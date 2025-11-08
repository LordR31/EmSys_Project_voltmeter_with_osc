# ADC

The Analog Digital Converter (ADC) used by the Jade M1 Board is a 10bit ADC. The ADC was used to develop the Voltmeter side of the project.

The ADC was configured as follows:

- using channel 0
- using AVCC as refference
- prescalar was set to 128
- used with the polling method, not with interrupts.

![ADC Block Schematic](img/ADC.png)