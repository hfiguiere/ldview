@echo off
set VS2019=/p:PlatformToolset=v142 /p:WindowsTargetPlatformVersion=10.0
set VS2015=/p:PlatformToolset=v140 /p:WindowsTargetPlatformVersion=8.1
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsamd64_x86.bat" set VS=%VS2019%
set CONFIG=/p:Configuration=Release
msbuild %CONFIG% %VS% LDView.sln /p:Platform=x64
msbuild %CONFIG% %VS% LDView.sln /t:Launcher /p:Platform=win32
msbuild %CONFIG% %VS% Translations\Hungarian\Hungarian.vcxproj /p:Platform=x64
msbuild %CONFIG% %VS% Translations\German\German.vcxproj       /p:Platform=x64
msbuild %CONFIG% %VS% Translations\Italian\Italian.vcxproj     /p:Platform=x64
msbuild %CONFIG% %VS% Translations\Czech\Czech.vcxproj         /p:Platform=x64
msbuild %CONFIG% %VS% LDViewThumbs\LDViewThumbs.vcxproj /p:SolutionDir=%CD%\ /p:Platform=win32
msbuild %CONFIG% %VS% LDViewThumbs\LDViewThumbs.vcxproj /p:SolutionDir=%CD%\

if exist "c:\Program Files (x86)\Inno Setup 6\iscc.exe" "c:\Program Files (x86)\Inno Setup 6\iscc.exe" LDView64.iss
