CSOPESY MCO1 - Process Scheduler and CLI
========================================

Group members
-------------
Last Name, First Name

Entry point
-----------
src/main.cpp contains the main() function.

Requirements
------------
Windows with Visual Studio 2022 (or the Visual Studio 2022 Build Tools)
and the "Desktop development with C++" workload.

Build
-----
Visual Studio: open CSOPESY.vcxproj, select Release | x64, then
Build > Build Solution.

Command line (Developer Command Prompt for VS 2022):
    msbuild CSOPESY.vcxproj /p:Configuration=Release /p:Platform=x64

Run
---
Run from this folder so the program finds config.txt:
    x64\Release\CSOPESY.exe
or press Ctrl+F5 in Visual Studio.

Type "initialize" first. It loads config.txt, which can be edited
without recompiling.

Commands
--------
Main menu:
    initialize                 load config.txt (required before other commands)
    screen -s <process name>   create a process and open its screen
    screen -r <process name>   reopen a running process's screen
    screen -ls                 show CPU utilization and process lists
    scheduler-start            start generating dummy processes
    scheduler-stop             stop generating dummy processes
    report-util                save the screen -ls report to csopesy-log.txt
    exit                       quit

Process screen:
    process-smi                show process info and logs
    exit                       return to the main menu
