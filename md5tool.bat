@echo off
reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\FileSystem" /v "LongPathsEnabled" /t "REG_DWORD" /d "1" /f
powershell -NoProfile -ExecutionPolicy Unrestricted .\md5tool.ps1 %1
