if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "makeWebCurseLogData.sh  daysBack"
	echo ""
	echo "Example:"
	echo "makeWebCurseLogData.sh 2"
	echo ""
	
	exit 1
fi



emailSaltFile=~/lifeLogEmailSalt.txt

emailSaltValue="test_salt"


if [ -f $emailSaltFile ]
then
	emailSaltValue=$(head -n 1 $emailSaltFile)
fi



days=$1

echo "Processing files newer than $days days"


tempDir=~/tempCurseLogs
webDir=~/private_html/publicLifeLogData


mkdir $webDir


mkdir $tempDir

cd ~/checkout/OneLife/server


for f in curseLog*; do
	
	if [ -d "$f" ]
	then
		echo "Processing folder $f"

		mkdir $tempDir/$f
		
		mkdir $webDir/$f
		
        # copy new files ($days days or newer) into temp dir 
		find $f/* -mtime -$days -exec cp {} $tempDir/$f/ \;
		

		if [ -z "$(ls -A $tempDir/$f/)" ]
		then
			# empty
			empty=1
		else
			# some files
			for g in $tempDir/$f/*; do
			
				salt=$emailSaltValue

				# replace each email with a hash
				perl -MDigest::SHA=sha1_hex -pe 's/[^_ @][^ @]*@\S*/sha1_hex( $& . "'$salt'" )/ge' "$g" > "$g"_temp
				
				gFilename=$(basename -- "$g")
			
				echo "Processing $gFilename"
				
				mv "$g"_temp $webDir/$f/$gFilename
			done
		fi
	fi

done


rm -rf $tempDir
