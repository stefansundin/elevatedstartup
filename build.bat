@echo off

set prefix64=x86_64-w64-mingw32-

taskkill /F /IM ElevatedStartup.exe

%prefix64%windres include/resource.rc include/resource.o

if "%1" == "debug" (
  %prefix64%gcc -o ElevatedStartup.exe elevatedstartup.c include/resource.o -mwindows -lshlwapi -lole32 -g -DDEBUG
) else (
  %prefix64%gcc -o ElevatedStartup.exe elevatedstartup.c include/resource.o -mwindows -lshlwapi -lole32 -O2 -s

  if "%1" == "run" (
    start ElevatedStartup.exe %2 %3 %4 %5 %6 %7 %8 %9
  )
)
