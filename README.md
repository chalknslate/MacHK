# MacHK
A scriptable hotkey language written for MacOS. Soon to allow for HTTP/S transmission and programmable GUIs. AHK like, lots of modules for simple things but with a C-like syntax.

Example script in a folder. To compile:
```bash

clang main.c -I modules -o main.exe -framework ApplicationServices && ./main.exe

```