
if [ "$#" -ne 5 ]; then
    echo ""
	echo "Usage"
    echo "runMapPull x y mapWide mapHigh shrinkFactor"
    echo ""
    exit 1
fi

xStart=$(($1 - $3/2))
xEnd=$(($1 + $3/2))

yStart=$(($2 - $4/2))
yEnd=$(($2 + $4/2))

screenW=$((1280 / $5))
screenH=$((720 / $5))


echo -n $xStart > settings/mapPullStartX.ini
echo -n $xEnd > settings/mapPullEndX.ini
echo -n $yStart > settings/mapPullStartY.ini
echo -n $yEnd > settings/mapPullEndY.ini

echo -n "1" > settings/mapPullMode.ini
echo -n "0" > settings/fullscreen.ini

echo -n "0" > settings/useLargestWindow.ini

echo -n $screenW > settings/screenWidth.ini
echo -n $screenH > settings/screenHeight.ini


./OneLife

echo -n "0" > settings/mapPullMode.ini
echo -n 1280 > settings/screenWidth.ini
echo -n 720 > settings/screenHeight.ini
