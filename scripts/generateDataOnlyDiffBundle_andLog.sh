

if [ $# -ne 1 ]
then
	echo "" 
	echo "Usage:"
	echo "" 
	echo "genrateDataOnlyDiffBundle_andLog.sh logFileName"
	echo "" 
	exit 1
fi


# tee usage for stdout and stderr found here:
# https://stackoverflow.com/questions/418896/how-to-redirect-output-to-a-file-and-stdout


./generateDataOnlyDiffBundle.sh 2>&1 | tee $1
