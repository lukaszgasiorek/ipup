ipup - program pobiera z zdalnej strony aktualne IP i zapisuje do pliku
-------------------------------------

Program zbudowany na bazie biblioteki wxWidgets w wersji z SVN rev. #74043

Polecenie do skompilowania programu pod systemem Fedora 18:

    g++ -Wall -std=c++11 `wx-config --cxxflags` ipup.cpp `wx-config --libs core,net,base` -lcurl -o ipup

#### Screeny programu z systemu Fedora 18 i Windows 7:

![gim-fedora18](https://dl.dropboxusercontent.com/sh/kitbmes32iwm7f8/W2yGBywspZ/ipup-fedora18.png) ![gim-window7](https://dl.dropboxusercontent.com/sh/kitbmes32iwm7f8/oLk1TsYGov/ipup-windows7.png)
