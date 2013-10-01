del /Q "%CD%\ide\VC100\*.sdf"
del /Q "%CD%\bin\*.pdb"
del /Q "%CD%\bin\*.ilk"

rmdir /S /Q ".\ide\VC100\ipch"
rmdir /S /Q ".\ide\VC100\Debug"
rmdir /S /Q ".\ide\VC100\Release"

pause