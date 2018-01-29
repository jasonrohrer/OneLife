path=~/checkout/OneLife/gameSource/

for f in *.aiff; 
do 
	echo "Applying EQ to $f ...";
	$path/convolveTest $f $path/eqImpulseResponse.aiff $f
done