@echo off
cd /d %~dp0

echo "Creating symlinks..."

mklink /D categories ..\categories
mklink /D objects ..\objects
mklink /D transitions ..\transitions
mklink /D tutorialMaps ..\tutorialMaps
mklink dataVersionNumber.txt ..\dataVersionNumber.txt

echo "Starting server..."

OneLifeServer

pause