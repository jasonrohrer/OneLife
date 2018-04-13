if [ "$#" -lt "1" ] ; then
   echo "Usage:"
   echo "./walkNameLine id"
   echo
   exit;
fi

id=$1


echo -n "" > $id_line.txt


while [[ ! -z "$id" ]]; do
	name=`grep "67748" *name* | awk 'END {print $(NF-1), $NF}'`

	echo -e "$name\n$(cat $id_line.txt)" > $id_line.txt

	noParentSearch=`grep "B [0-9]* $id .*noParent" *`;
	
	if [[ -z $noParentSearch ]]; then
		id=""
	else
		id=`grep "B [0-9]* 67748 .*parent" *  | awk -F"=" '{print $2}' | awk -F"," '{print $1}'`
	fi
done

