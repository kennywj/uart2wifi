# uart2wifi
The project implements a protocol to bridge the ethernet traffic and contol message between PC emulator and RealTek Ameba (wifi module).
The traffic from wireless to ameba, then forward to PC emulator via UART port.

git clone --recursive https://chenwenruei@bitbucket.org/chenwenruei/uart2wifi.git <br>
git clone --recursive https://github.com/kennywj/uart2wifi.git
## usage
"make" will generate executable program "u2w".  
  $cd mbedtls-2.6.0/ <br>
  $make lib <br>
  $cd ../ <br>
  $make <br>
To execute "u2w" will have a command line console on screen.  
Input the "help" command will shows available commands in system.  
