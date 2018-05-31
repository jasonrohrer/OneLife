if [ "$#" -lt "1" ] ; then
   echo "Usage:"
   echo "./walkNameLine id"
   echo
   exit;
fi

origID=$1
id=$1


echo -n "" > ${id}_line.txt


while [[ ! -z "$id" ]]; do
	name=`grep "$id" *name* | awk 'END {print $(NF-1), $NF}' | tr A-Z a-z | awk '{for(i=1;i<=NF;i++){ $i=toupper(substr($i,1,1)) substr($i,2) }}1'`
	echo "Name = $name"
	echo -e "$name\n$(cat ${origID}_line.txt)" > ${origID}_line.txt

	noParentSearch=`grep "B [0-9]* $id .*noParent" *`;

	#echo "NO parent search = '$noParentSearch'"
	if [[ -n "$noParentSearch" ]]; then
		id=""
		#echo "No parent"
	else
		id=`grep "B [0-9]* $id .*parent" *  | awk -F"=" '{print $2}' | awk -F"," '{print $1}'`
	fi
done

