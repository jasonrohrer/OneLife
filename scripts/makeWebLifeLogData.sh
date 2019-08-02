if [ $# -lt 1 ]
then
	echo "Usage:"
	echo "makeWebLifeLogData.sh  daysBack"
	echo ""
	echo "Example:"
	echo "makeWebLifeLogData.sh 2"
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


tempDir=~/tempLifeLogs
webDir=~/private_html/publicLifeLogData


mkdir $webDir


mkdir $tempDir

cd ~/checkout/OneLife/server


for f in lifeLog*; do
	
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
			
				cat $g | sed -e 's/\(killer_[0-9]*\)_[^ ]*/\1/' > "$g"_temp
				cat "$g"_temp | sed -e 's/\(parent=[0-9]*\),[^ ]*/\1/' > "$g"_temp2
				salt=$emailSaltValue

				# replace each email with a hash
				perl -MDigest::SHA=sha1_hex -pe 's/[^_ @][^ @]*@[^ ]*/sha1_hex( $& . "'$salt'" )/ge' "$g"_temp2 > "$g"_temp3
				
				gFilename=$(basename -- "$g")
			
				echo "Processing $gFilename"
				
				mv "$g"_temp3 $webDir/$f/$gFilename
			done
		fi
	fi

done


rm -rf $tempDir
