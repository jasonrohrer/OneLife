flagFile=~/serverCodeUpdateFlag.txt

if [ -f $flagFile ]
then


	flag=$(head -n 1 $flagFile)


	if [ "$flag" = "1" ]
	then
		echo ""
		echo "Server code update scheduled.  Running it now."
		
		~/checkout/OneLifeWorking/scripts/updateServerCode.sh
		
		
		echo "0" > $flagFile
	fi
fi