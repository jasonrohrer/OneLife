#!/bin/bash

updatePostDir=~/www/updatePosts


latestPostVersion=-1


while read x; 
do 
if [[ $x == *.html ]]; then
  # extract number from file name
  version=$(echo $x | sed -n  's/[^0-9]*\([0-9]\+\).*/\1/p' );
  
  if [[ $version -gt $latestPostVersion ]]; then
	  latestPostVersion=$version
  fi
fi

done < <(ls -p $updatePostDir | grep -v / )


echo "Latest Version Reported: $latestPostVersion"



echo "" 
echo "Updating OneLifeData6Latest"
echo ""

cd ~/checkout/OneLifeData6Latest
hg pull
hg update


newerVersion="D"

reportCount=0



while read x; 
do 

if [[ $x -gt $latestPostVersion ]] || [ $x == "Start" ]; then

	if [ "$newerVersion" != "D" ]; then

		hg update OneLife_v$newerVersion
		
		echo ""
		echo "Generating report for v$newerVersion"

		reportFile=$updatePostDir/$newerVersion.html


		versionDate=$(hg log -r OneLife_v$newerVersion | grep "date" | sed 's/date:\s\s*//' | sed 's/ [0-9]\+:[0-9]\+:[0-9]\+/,/' | sed 's/\(20[0-9][0-9]\)\s[-+]*[0-9][0-9][0-9][0-9]/\1/' | sed 's/ 0\([0-9]\),/ \1,/' )

		echo "<h3>Version $newerVersion ($versionDate)</h3>" > $reportFile
		
		reportCount=$((reportCount+1))
		
		newObjCount=0
		while read y;
		do
			objName=$(cat $y | sed -n 2p )
			
			echo "$objName<br>" >> $reportFile
			
			newObjCount=$((newObjCount+1))

		done < <(hg status --rev OneLife_v$x:OneLife_v$newerVersion objects/ | grep "A " | sed 's/A //')

		echo "$newObjCount objects in report for v$newerVersion"

		if [[ $newObjCount -eq 0 ]];
		then
			rm $reportFile
		fi
	fi

	newerVersion=$x
fi

done < <( hg tags | grep "OneLife" | sed 's/\s\s.*//' | sed 's/OneLife_v\([0-9a-zA-Z]\+\)/\1/')


echo ""
echo "Generated $reportCount reports."
echo "Done."
