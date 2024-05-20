cd /home/jcr15

cd checkout/OneLifeData7Latest

~/checkout/OneLifeWorking/scripts/gitPullComplete.sh

# clear caches
rm */cache.fcz
rm */bin_*cache.fcz


cd ../OneLifeWorking

~/checkout/OneLifeWorking/scripts/gitPullComplete.sh

linesOfCode=`sloccount . | grep 'Total Physical Source' | sed 's/^.*= //'`

cd gameSource


sh makePrintReportHTML

cd ../../OneLifeData7Latest

../OneLifeWorking/gameSource/printReportHTML $linesOfCode ../../public_html/objectsReport.php
