
if [ "$#" -lt "1" ] ; then
   echo "Usage:"
   echo "./makeCurrent folderName"
   echo
   exit;
fi

ln -snf $1 CurrentOneLifeData