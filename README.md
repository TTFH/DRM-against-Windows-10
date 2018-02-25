# DRM-against-Windows-10
Windows and Linux DRM software with dual-boot support.

**Building:**

g++-7 -Wall DRM.cpp -o DRM-Linux

g++-7 -Wall -DDEBUG DRM.cpp -o DRM-Linux-Debug

g++ -Wall DRM.cpp -o DRM-Windows.exe

g++ -Wall -DDEBUG DRM.cpp -o DRM-Windows-Debug.exe

**Running:**

./DRM-Linux

./DRM-Linux-Debug

DRM-Windows.exe

DRM-Windows-Debug.exe

