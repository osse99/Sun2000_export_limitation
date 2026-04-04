#/bin/bash
#
# Get price from Nordpool spot price file
#	File should have one  spot price row for each 15 minutes, total 96 rows!
#
# Set export limitiation to 0 if the price is negative
# Set export limitation to main fuse maximum if price is positive
#
Max_export=11
SDongle05_IP="192.168.0.144"
#
# Set name of Nordpool file
#Filename=$(date --date="next day" +%d-%m-%Y)
Filename=$(date +%d-%m-%Y)
#
modbus_tcp="./sun2000_export"
#
spot_price=/opt/electricity/spot_prices_15min/Prices-${Filename}
#
# Functions
#
#
# MODBUS TCP using SDongle05 is a bit unstable, retry if it fails
modbus_rw(){
        # echo "Parameter #1 is $1"
        RETURN_CODE=1
        RETRIES=5
        i=1
        while [ $RETURN_CODE -ne 0 ] && [ $i -le $RETRIES ]
        do
                i=$((i++))
                curexp=$(./sun2000_export -i 192.168.0.144 $1 | sed 's/\...//')
                RETURN_CODE=$?
        done
}
#
# Main script
#
# Validate spot price file
if [ -r ${Filename} ]
then
	if [ $(wc -l ${Filename} | (read a b; echo $a)) -ne 96 ]
	then
		echo "The spot price file ${Filename} does not have the correct number of rows, exiting"
		exit 1
	fi
fi
#
# Check if the modbus communication binary exists and is executable
if ! [ -x ${modbus_tcp} ]
then
	echo "The modbus binary ${modbus_tcp} does not exist or is not executable, exiting"
	exit 1
fi
#
#
# Do some cheating to avoid timezone and daylight savings complexity (no solar production during night inyway)
Current_hour=$(date +%H)
if [ ${Current_hour} -lt "5" ] && [ ${Current_hour} -gt "20" ]
then
	echo "Outside production hours, exiting"
	exit 0
fi
#
# Get number of quaters for current hour and minute (0,15,30,45)
hour=$(a=$(printf '%(%s)T\n'); printf '%(%H)T\n' "$((a-a%(15*60)))")
min=$(a=$(printf '%(%s)T\n'); printf '%(%M)T\n' "$((a-a%(15*60)))")
#
# Remove leading zero
hour=$(echo ${hour} | sed 's/^0//')
min=$(echo ${min} | sed 's/^0//')

#echo "Hour: ${hour}"
#echo "Min: ${min}"
#
# Calculate corresponding price row in Nordpool file
spot_row=$(echo $(($hour*4 + ${min}/15 + 1)))
#echo "Spot row: ${spot_row}"
#
# Get price for this spot_row
Current_price=$(sed "${spot_row}q;d" $spot_price)
echo "Current price: ${Current_price}"
#
#
# Positive/Negative price actions
if (( $(echo "$Current_price < 0" |bc -l) ))
then
	echo "Negative price, set export limit to 0 KW"
	modbus_rw
	if [ ${RETURN_CODE} -ne 0 ]
	then
		echo "Unable to read export limitation from SUN2000"
		exit 1
	fi
	# echo "curexp: ${curexp}"
	if [ ${curexp} -ne 0 ]
	then
		modbus_rw 0
		if [ ${RETURN_CODE} -ne 0 ]
		then
			echo "Unable to set export limitation to 0 KW"
			exit 1
		else
			echo "Export limitation set to 0 KW"
		fi
	else
			echo "Export limit was already set to 0 KW"
	fi
else
	echo "Positive price, set export limit to ${Max_export} KW"
	modbus_rw
	if [ ${RETURN_CODE} -ne 0 ]
	then
		echo "Unable to read export limitation from SUN2000"
		exit 1
	fi
	# echo "curexp: ${curexp}"
	if [ ${curexp} -ne ${Max_export} ]
	then
		modbus_rw 11
		if [ ${RETURN_CODE} -ne 0 ]
		then
			echo "Unable to set export limitation to ${Max_export} KW"
			exit 1
		else
			echo "Export limit set to ${curexp} KW"
		fi
	else
			echo "Export limit was already set to ${curexp} KW"

	fi
fi
