cd /home/jcr15

cd checkout/OneLifeData6Latest

hg pull
hg update

cd ../OneLifeWorking/gameSource

hg pull
hg update


sh makePrintReportHTML

cd ../../OneLifeData6Latest

../OneLifeWorking/gameSource/printReportHTML ../../public_html/objectReport.php