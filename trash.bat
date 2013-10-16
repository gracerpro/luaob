del /Q ".\ide\VC100\*.sdf"
del /Q ".\bin\*.pdb"
del /Q ".\bin\*.ilk"

rmdir /S /Q ".\ide\VC100\ipch"
rmdir /S /Q ".\ide\VC100\Debug"
rmdir /S /Q ".\ide\VC100\Release"

pause