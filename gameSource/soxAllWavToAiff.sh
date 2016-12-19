for i in *.wav
do 
sox $i -t aiff -b 16 $(basename $i .wav).aiff 
done