@echo off

echo:
echo:

echo This will copy all AHAP content files from the current editng folder
echo into the Git repository.
echo:

echo The checked-out Git repository must be in the current editing folder,
echo in a sub-folder called:

echo:

echo    AnotherPlanetData

echo:

set /p DUMMY=Hit ENTER to continue...




call :folderCheck AnotherPlanetData

call :folderCheck animations

call :folderCheck contentSettings

call :folderCheck faces

call :folderCheck ground

call :folderCheck music

call :folderCheck objects

call :folderCheck scenes

call :folderCheck sounds

call :folderCheck sprites

call :folderCheck transitions


call :folderCopy animations txt

call :folderCopy contentSettings ini

call :folderCopy faces png

call :folderCopy ground tga

call :folderCopy music ogg

call :folderCopy objects txt

call :folderCopy scenes txt

call :folderCopy sounds aiff

call :folderCopy sounds ogg

call :folderCopy sounds fcz

call :folderCopy sprites tga

call :folderCopy sprites txt

call :folderCopy transitions txt


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


:folderCopy
  echo copying %~2 files found in %~1
  if exist AnotherPlanetData\%~1\*.%~2 (
    del AnotherPlanetData\%~1\*.%~2
  )
  if exist %~1\*.%~2 (
    copy %~1\*.%~2 AnotherPlanetData\%~1\ >NUL
  )
goto :eof