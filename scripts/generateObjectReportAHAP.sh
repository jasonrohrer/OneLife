cd /home/jcr15

cd checkout/AnotherPlanetDataLatest

~/checkout/OneLifeWorking/scripts/gitPullComplete.sh


# clear caches
rm */cache.fcz
rm */bin_*cache.fcz


cd ../OneLifeWorking

~/checkout/OneLifeWorking/scripts/gitPullComplete.sh

linesOfCode=`sloccount . | grep 'Total Physical Source' | sed 's/^.*= //'`

cd gameSource


sh makePrintReportHTML

cd ../../AnotherPlanetDataLatest

../OneLifeWorking/gameSource/printReportHTML $linesOfCode ../../public_html/objectsReportAHAP.php
