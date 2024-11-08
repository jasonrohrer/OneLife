

progPath=""
pid=0
line=""
varName=""

if [ $# -lt 4 ]; then

	read -p "Enter process ID to attach to: " pid


	echo ""
	echo "Example:   ./OneLifeServer"

	read -p "Enter path to program file: " progPath



	echo ""
	echo "Example:   main.cpp:319"

	read -p "Enter line number to break at (where variable is defined): " line

	echo ""
	echo "Example:   myVariable"

	read -p "Enter variable name: " varName
else
	pid=$1
	progPath=$2
	line=$3
    varName=$4
fi

echo ""
echo "About to attach to $progPath with pid=$pid, break at like $line, and print value of variable $varName"

echo ""
read -p "Enter if ready, ctrl-c to cancel: " ready





# Attach to the process
gdb -p $pid $progPath <<EOF
# Set a breakpoint at main (if you know the function name)
break $line

continue

# Print the variable
print $varName
detach
quit
EOF