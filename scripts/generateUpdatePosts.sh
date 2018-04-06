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
echo "Updating OneLifeData7Latest"
echo ""

cd ~/checkout/OneLifeData7Latest
git pull --tags


newerVersion="D"

reportCount=0



while read x; 
do 

if [[ $x -ge $latestPostVersion ]] || [ $x == "Start" ]; then

	if [ "$newerVersion" != "D" ] && 
	   [[ $newerVersion -gt $latestPostVersion ]];
		then

		git checkout -q OneLife_v$newerVersion
		
		echo ""
		echo "Generating report for v$newerVersion as compared to v$x"

		reportFile=$updatePostDir/${newerVersion}.html
		
		objDiffFile=$updatePostDir/${newerVersion}_objDiff.php

		objDiffURL=updatePosts/${newerVersion}_objDiff.php

		
		versionDate=$(git log -1 OneLife_v$newerVersion | grep "Date" | sed 's/Date:\s\s*//' | sed 's/ [0-9]\+:[0-9]\+:[0-9]\+/,/' | sed 's/\(20[0-9][0-9]\)\s[-+]*[0-9][0-9][0-9][0-9]/\1/' | sed 's/ 0\([0-9]\),/ \1,/' )

		echo "<h3>Version $newerVersion ($versionDate)</h3>" > $reportFile
		
		reportCount=$((reportCount+1))

		
		newObjCount=0
		while read y;
		do
			objName=$(cat $y | sed -n 2p )

			echo "<font color=#00ff00>+</font>" >> $reportFile;

			echo "$objName<br>" >> $reportFile
			
			newObjCount=$((newObjCount+1))

		done < <(git diff --name-status OneLife_v$x OneLife_v$newerVersion objects/ | grep -v nextObjectNumber.txt | grep "^A" | sed 's/A\t//')
		

		
		echo "$newObjCount new objects in report for v$newerVersion"

		echo "<?php include_once( '../header.php' );?>" >> $objDiffFile

		echo "<table width=100% border=0><tr><td></td><td align=center><font size=5>One Hour One Life - v$newerVersion Raw Object Change Log</font></td><td align=right>[<a href=..>Home</a>]</td></tr></table>" >> $objDiffFile
		echo "<br><br><br>" >> $objDiffFile

		
		changedObjCount=0
		while read y;
		do
			objName=$(cat $y | sed -n 2p )

			echo "<font color=#ffff00>~</font>" >> $reportFile;

			echo "$objName<br>" >> $reportFile

			echo "<b>$objName</b></br>" >> $objDiffFile
			echo "<pre>" >> $objDiffFile
			
		   git diff OneLife_v$x OneLife_v$newerVersion $y | \
				sed 's/^\(\+[^\+].*\)/<font color=#00DD00>\1<\/font>/' | \
				sed 's/^\(\-[^\-].*\)/<font color=#FF0000>\1<\/font>/' \
				>> $objDiffFile

			echo "</pre><br><br><hr><br><br>" >> $objDiffFile
			
			changedObjCount=$((changedObjCount+1))

		done < <(git diff --name-status OneLife_v$x OneLife_v$newerVersion objects/ | grep -v nextObjectNumber.txt | grep "^M" | sed 's/M\t//')


		echo "<?php include_once( '../footer.php' );?>" >> $objDiffFile

		if [[ $changedObjCount -eq 0 ]];
		then
			rm $objDiffFile
		fi

		
		echo "$changedObjCount changed objects in report for v$newerVersion"



		# update to old version so that removed object files exist
		git checkout -q OneLife_v$x
		
		removedObjCount=0
		while read y;
		do
			objName=$(cat $y | sed -n 2p )
			
			echo "<font color=#ff0000>x</font>" >> $reportFile;
			echo "$objName<br>" >> $reportFile
			
			removedObjCount=$((removedObjCount+1))

		done < <(git diff --name-status OneLife_v$x OneLife_v$newerVersion objects/ | grep -v nextObjectNumber.txt | grep "^D" | sed 's/D\t//')

		# back to newer
		git checkout -q OneLife_v$newerVersion
		

		echo "$removedObjCount removed objects in report for v$newerVersion"

		total=$(($newObjCount + $changedObjCount + $removedObjCount))
		
		fullDiffText=""

		if [[ $changedObjCount -gt "0" ]];
		then
			fullDiffText="(<a href=$objDiffURL>raw changes</a>)"
		fi
		
		
		if [[ $total -gt "1" ]];
		then
			echo "($total object updates) $fullDiffText<br>" >> $reportFile
		else if [[ $total -gt "0" ]];
			 then
				 echo "($total object update) $fullDiffText<br>" >> $reportFile
			 fi
		fi
		
		echo "<br>" >> $reportFile

		echo "<table border=0>" >> $reportFile

		cellColor="#111111"
		otherCellColor='#222222'
		
		
		newTransCount=0
		while read y;
		do
			
			objNumA=$(echo -n "$y" | sed 's/transitions\/\([0-9\-]*\)_\([0-9\-]*\)\(_L.*\)*\.txt/\1/' )
			objNumB=$(echo -n "$y" | sed 's/transitions\/\([0-9\-]*\)_\([0-9\-]*\)\(_L.*\)*\.txt/\2/' )

			objNameA="-";
			objNameB="-";

			if [[ $objNumA -gt "0" ]];
			then
				objNameA=$(cat objects/$objNumA.txt | sed -n 2p )
			fi

			if [[ $objNumB -gt "0" ]];
			then
				objNameB=$(cat objects/$objNumB.txt | sed -n 2p )
			fi

			echo -n "<tr><td valign=top bgcolor=$cellColor><font color=#00ff00>+</font></td>" >> $reportFile;

			echo "<td valign=top bgcolor=$cellColor>[ $objNameA ] +<br>[ $objNameB ]</td></tr>" >> $reportFile

			temp=$cellColor
			cellColor=$otherCellColor
			otherCellColor=$temp

			newTransCount=$((newTransCount+1))

		done < <(git diff --name-status OneLife_v$x OneLife_v$newerVersion transitions/ | grep "^A" | sed 's/A\t//')

		echo "$newTransCount new transitions in report for v$newerVersion"

		

		changedTransCount=0
		while read y;
		do
			objNumA=$(echo -n "$y" | sed 's/transitions\/\([0-9\-]*\)_\([0-9\-]*\)\(_L\)*\.txt/\1/' )
			objNumB=$(echo -n "$y" | sed 's/transitions\/\([0-9\-]*\)_\([0-9\-]*\)\(_L\)*\.txt/\2/' )

			objNameA="-";
			objNameB="-";

			if [[ $objNumA -gt "0" ]];
			then
				objNameA=$(cat objects/$objNumA.txt | sed -n 2p )
			fi

			if [[ $objNumB -gt "0" ]];
			then
				objNameB=$(cat objects/$objNumB.txt | sed -n 2p )
			fi

			echo -n "<tr><td valign=top bgcolor=$cellColor><font color=#ffff00>~</font></td>" >> $reportFile;

			echo "<td valign=top bgcolor=$cellColor>[ $objNameA ] +<br>[ $objNameB ]</td></tr>" >> $reportFile

			
			changedTransCount=$((changedTransCount+1))

		done < <(git diff --name-status OneLife_v$x OneLife_v$newerVersion transitions/ | grep "^M" | sed 's/M\t//')

		echo "$changedTransCount changed transitions in report for v$newerVersion"

		# update to old version so that objects exist
		git checkout -q OneLife_v$x
		
		removedTransCount=0
		while read y;
		do
			
			objNumA=$(echo -n "$y" | sed 's/transitions\/\([0-9\-]*\)_\([0-9\-]*\)\(_L\)*\.txt/\1/' )
			objNumB=$(echo -n "$y" | sed 's/transitions\/\([0-9\-]*\)_\([0-9\-]*\)\(_L\)*\.txt/\2/' )

			objNameA="-";
			objNameB="-";

			if [[ $objNumA -gt "0" ]];
			then
				objNameA=$(cat objects/$objNumA.txt | sed -n 2p )
			fi

			if [[ $objNumB -gt "0" ]];
			then
				objNameB=$(cat objects/$objNumB.txt | sed -n 2p )
			fi

			echo -n "<tr><td valign=top bgcolor=$cellColor><font color=#ff0000>x</font></td>" >> $reportFile;

			echo "<td valign=top bgcolor=$cellColor>[ $objNameA ] +<br>[ $objNameB ]</td></tr>" >> $reportFile
			
			removedTransCount=$((removedTransCount+1))

		done < <(git diff --name-status OneLife_v$x OneLife_v$newerVersion transitions/ | grep "^D" | sed 's/D\t//')
		# back to newer
		git checkout -q OneLife_v$newerVersion
		

		echo "$removedTransCount removed transitions in report for v$newerVersion"

		
		echo "</table>"  >> $reportFile

		total=$(($newTransCount + $changedTransCount + $removedTransCount))

		if [[ $total -gt "1" ]];
		then
			echo "($total transition updates)<br>" >> $reportFile
		else if [[ $total -gt "0" ]];
			 then
				 echo "($total transition update)<br>" >> $reportFile
			 fi
		fi


		if [[ $newObjCount -eq 0 && $changedObjCount -eq 0 && $removedObjCount -eq 0 && $newTransCount -eq 0 && $changedTransCount -eq 0 && $removedTransCount -eq 0 ]];
		then
			rm $reportFile
		fi

		echo ""
		echo ""
	fi

	newerVersion=$x
fi

done < <( git for-each-ref --sort=-creatordate --format '%(refname:short)' refs/tags | grep "OneLife" | sed 's/\s\s.*//' | sed 's/OneLife_v\([0-9a-zA-Z]\+\)/\1/')


git checkout -q master


echo ""
echo "Generated $reportCount reports."
echo "Done."
