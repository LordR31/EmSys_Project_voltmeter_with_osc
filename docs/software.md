# Program Logic

The program logic can be divided into 3 parts, starting from the high level software to the low level hardware. 

# Block Diagram

The first and upper layer is the Application Layer. This is the high level software that was developed for the program to be able to run and it includes all the functions and libraries / modules developed.

The next layer is the Service Layer, which refers to the ISR functions used by the system for the hardware and software buttons, as well as the Timer1 delay functions.

Lastly, at the lowest level, there is the Hardware Layer, which is represented by the hardware components and peripherals used in this project. It includes the SPI and ADC peripherals, as well as the hardware buttons used for various system functions.

![Bloc Diagram](img/Diagrama_Bloc_Sistem.drawio.svg)

# Program Work Flow

The program logic is best understood when paired with a flow chart, such as the one below.

After the system is powered-on, we wait for the user to press the Start Button. When that happens, we turn on the ADC and clear the screen. We check to see if we keep the voltage type as is or toggle it, then we check if we need to process any touchscreen command. We measure the voltage using the ADC, we store the points and get the minimum and maximum values among the last 25 measurements.

We check to see if the Waveform Viewer is turned on and if it is set to display in digital or analogue mode. If the Waveform Viewer is on, we check if the cursor is on and if any of the cursor buttons were pressed, then we get cursor voltage.

Finally, we draw the indicator LEDs and the loop can begin again.

![Flow Chart](img/Flow_Chart.svg)