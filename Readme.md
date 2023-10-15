## What is does
A c program written for avr. It controls a lcd, stepper motor (curtain), timer (date-time) and lamp.

## How
Serial is used to receive commands and set time. The date-time, curtain status and lamp light level is shown on lcd.<br>
The serial command input is protected with a hardcoded password. If it became idle for 1 minute, it will lock automatically.
<br>
The stepper motor will rotate 190 degrees to close or open curtain. Each step is 5 degree.

## Commands
- `set time HH:mm:ss MM/DD/YYYY`: to set date and time, stored in ds3232
- `set lamp NN`: to set the lamp's light level, number between 0 and 100
- `open curtain`: to open the curtain using stepper motor
- `close curtain`: to close the curtain using stepper motor
- `help`: to show this list
- Anything else: error message Unknown input...

## Credits
I used open source libraries from these sources. Their names are on top of the files as author. Only `main.c` is my job.
- Efthymios Koktsidis
- Denis Goriachev
- Barrett

## Disclaimer
This is a university project written solely for learning AVR and C. It has not been properly tested and has never been used in production.

