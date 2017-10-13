cd ../../minorGems

echo "Pulling minorGems changes from server..."

git pull



cd ../OneLife

echo "Pulling editor changes from server..."

git pull


./configure 1
cd gameSource

rm ../../minorGems/game/platforms/SDL/gameSDL.o

echo
echo "Building latest version of game client..."

make

echo
echo "Building latest version of editor..."

./makeEditor.sh

echo
echo "Building latest version of server..."

cd ../server
make


echo
echo -n "Press ENTER to close."

read userIn