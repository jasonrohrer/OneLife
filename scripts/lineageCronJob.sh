cd ~/checkout/OneLife
~/checkout/OneLife/scripts/gitPullComplete.sh

cd documentation/html

cp noBotsHeader.php ~/public_html
cp noCounterFooter.php ~/public_html
cp header.php ~/public_html
cp footer.php ~/public_html

cd ~/checkout/OneLifeData7
~/checkout/OneLife/scripts/gitPullComplete.sh

rsync faces/*.png ~/public_html/faces/

rsync objects/*.txt ~/public_html/objects/



cd ~/checkout/AnotherPlanetData
~/checkout/OneLife/scripts/gitPullComplete.sh

rsync faces/*.png ~/public_html/ahapLineageServer/faces/

rsync objects/*.txt ~/public_html/ahapLineageServer/objects/



curl http://lineage.onehouronelife.com/server.php?action=purge_prepare
curl http://lineage.onehouronelife.com/server.php?action=purge

curl http://lineage.onehouronelife.com/ahapLineageServer/server.php?action=purge_prepare
curl http://lineage.onehouronelife.com/ahapLineageServer/server.php?action=purge