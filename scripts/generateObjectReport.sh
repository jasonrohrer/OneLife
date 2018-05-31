cd /home/jcr15

cd checkout/OneLifeData7Latest

git pull

# clear caches
rm */cache.fcz


cd ../OneLifeWorking

git pull

linesOfCode=`sloccount . | grep 'Total Physical Source' | sed 's/^.*= //'`

cd gameSource


sh makePrintReportHTML

cd ../../OneLifeData7Latest

../OneLifeWorking/gameSource/printReportHTML $linesOfCode ../../public_html/objectsReport.php
