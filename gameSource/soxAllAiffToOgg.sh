for i in *.aiff
do 
sox -S $i -t ogg -C 6 $(basename $i .aiff).ogg 
done