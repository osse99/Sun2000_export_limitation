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
* Open MODBUS communication in the SUN2000, port 502
* Compile the C program: cc sun2000_export.c -o sun2000_export -lmodbus
* Edit path's and max export value in the .sh script
* Start the script 0, 15, 30, 45 minutes every hour using cron

## ToDo
* The C program can easily be enhanced to read/set any MODBUS register

## Caveats
* Depending on the SUN2000 software version other export limitation register should be used
* MODBUS TCP communication with the SUN2000 inverter using the SDongle05 as proxy is not 100% stable, the bash script tries to mitigate this
