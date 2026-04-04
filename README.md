# Sun2000 export limitation
## Provided as is!
* Huawei SUN2000 12ktl-M0 with DTSU666 smartmeter and SDongle05
* Set zero export when negative Nordpool spot prices

## Dependencies
* "any" Linux/Unix
* Bash shell
* libmodbus
* Spot prices from Nordpool (or other source), example file provided

## Setup
* Compile the C program: cc sun2000_export.c -o sun2000_export -lmodbus
* Edit path's and max export value in the .sh script
* Start the script 0, 15, 30, 45 minutes every hour using cron

## ToDo
* The C program can easily be enhanced to read/set any MODBUS register 
