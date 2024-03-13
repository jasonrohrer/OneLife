cd ~/checkout/OneLife
git pull

cd documentation/html

cp noBotsHeader.php ~/public_html
cp noCounterFooter.php ~/public_html
cp header.php ~/public_html
cp footer.php ~/public_html

cd ~/checkout/OneLifeData7
git pull

rsync faces/*.png ~/public_html/faces/

rsync objects/*.txt ~/public_html/objects/



cd ~/checkout/AnotherPlanetData
git pull

rsync faces/*.png ~/public_html/ahapLineageServer/faces/

rsync objects/*.txt ~/public_html/ahapLineageServer/objects/
