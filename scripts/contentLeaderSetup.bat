@echo off

echo:
echo:

echo This setup script should be run once BEFORE you start your
echo editing operations as Content Leader.

echo:

echo WARNING:  this will overwrite content in your editing folder with
echo the most current content from Github

echo:

echo You should be running this from a "clean" install of AnotherPlanet
echo before you do any editing.

echo:

set /p DUMMY=Hit ENTER to continue...

echo:
echo Step 1:
echo:
echo Use your preferred Git client to clone AnotherPlanetData
echo into this editing folder.

echo:
echo After you do this, you should have a sub-folder in this folder called
echo   AnotherPlanetData

echo:

set /p DUMMY=Hit ENTER when you have completed Step 1...

call :folderCheck AnotherPlanetData

echo:

echo:
echo Step 2:
echo:
echo Now copying latest content from AnotherPlanetData Git repository
echo into your local editing folder.

echo:

call :folderCreate faces

call :folderCreate scenes


echo:

call :folderCopy animations txt

call :folderCopy contentSettings ini

call :folderCopy faces png

call :folderCopy ground tga

call :folderCopy music ogg

call :folderCopy objects txt

call :folderCopy scenes txt

call :folderCopy sounds aiff

call :folderCopy sounds ogg

call :folderCopy sounds txt

call :folderCopy sprites tga

call :folderCopy sprites txt

call :folderCopy transitions txt

echo:

echo Setup done.

echo:


set /p DUMMY=Hit ENTER to exit...

exit



:folderCheck
  if not exist %~1\ (
    echo:
    echo ERROR:  "%~1" folder not found in main editing folder.
    echo:
    set /p DUMMY=Hit ENTER to exit...
    exit
  )
goto :eof



:folderCreate
  if not exist %~1\ (
    echo Creating folder "%~1"
    mkdir %~1
  )
goto :eof



:folderCopy
  echo copying %~2 files found in AnotherPlanetData\%~1
  if exist %~1\*.%~2 (
    del %~1\*.%~2
  )
  if exist %~1\*.fcz (
    del %~1\*.fcz
  )
  if exist AnotherPlanetData\%~1\*.%~2 (
    copy AnotherPlanetData\%~1\*.%~2 %~1\ >NUL
  )
goto :eof