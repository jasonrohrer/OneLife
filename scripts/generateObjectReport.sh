cd /home/jcr15

cd checkout/OneLifeData7Latest

git pull

# clear caches
rm */cache.fcz


cd ../OneLifeWorking/gameSource

git pull


sh makePrintReportHTML

cd ../../OneLifeData7Latest

../OneLifeWorking/gameSource/printReportHTML ../../public_html/objectsReport.php