CPU Fan for the Raspberry Pi b
-------------------------------
Implements the PID algorithm to control the CPU fan on a Raspberry Pi b. There are two main parts to this repo, one is the kernel module for the actual control of the fan(fan_control.ko) and the other is a userspace utility that finds the proportional constant(tune).
The kernel module implements the following algorithm.
![PID Algorithm](https://wikimedia.org/api/rest_v1/media/math/render/svg/2ee061415fdfd20a6676bb328326795fec984cf1)

Interfacing the fan to the board
--------------------------------
Raspberry pi series of SBCs come with power pins(5V) on pins 2 and 4. Ground is on pin 6 . PWM is available on pin 12 (GPIO18). 
The fan chosen is the Noctua NF-A4x10 5 volt pwm fan mainly because of its form factor and the fact that it is controllable via PWM.

Running the software.
---------------------
Extract the code from github.
- run the make command in the kernel and uspace directories.  
- cd to the uspace directory and run the tune command. That will give you the proportional constant.
- cd to the kernel directory and insert the kernel module with the following command.  
*sudo insmod fan_control.ko set_point=<set point> prop_constant=<proportional constant>*  
set_point is the desired temperature of the cpu in millicentigrade and prop_constant is the proportional constant as reported by the tune utility.

Notes
-----
The tune utility will run the fan at half of maximum speed for 60 seconds and calculate the proportional constant from the temperature differences from before and after. In tests, we have noted a temperature decrease of upto 20 degrees centigrade under load.
