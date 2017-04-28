cd /home/jcr15

cd checkout/OneLifeData7Latest

hg pull
hg update

# clear caches
rm */cache.fcz


cd ../OneLifeWorking/gameSource

hg pull
hg update


sh makePrintReportHTML

cd ../../OneLifeData7Latest

../OneLifeWorking/gameSource/printReportHTML ../../public_html/objectsReport.php