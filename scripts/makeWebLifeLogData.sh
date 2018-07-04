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


days=$1


tempDir=~/tempLifeLogs
webDir=~/public_html/publicLifeLogData


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
		find $f/* -mtime +$days -exec cp {} $tempDir/$f/ \;
		

		for g in $tempDir/$f/*; do
			
		# replace each email with a hash
			perl -MDigest::SHA=sha1_hex -pe 's/[^ @]*@[^ ]*/sha1_hex$&/ge' $g > "$g"_temp
			
			gFilename=$(basename -- "$g")
			
			echo "Processing $gFilename"

			mv "$g"_temp $webDir/$f/$gFilename
		done
	fi

done


rm -rf $tempDir
