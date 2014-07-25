ipup
-------------------------------------
Program pobiera z zdalnej strony aktualne IP i zapisuje do pliku. Zbudowany na bazie biblioteki [wxWidgets](http://www.wxwidgets.org/)

Polecenie do skompilowania programu pod Linux:

    g++ -Wall -std=c++11 `wx-config --cxxflags` ipup.cpp `wx-config --libs core,net,base` -lcurl -o ipup

Ikona programu pochodzi z kolekcji **Fugue Icons** autorstwa Yusuke Kamiyamane.

> Copyright (C) 2010 Yusuke Kamiyamane. All rights reserved.
> The icons are licensed under a Creative Commons Attribution
> 3.0 license. <http://creativecommons.org/licenses/by/3.0/>
>
> http://p.yusukekamiyamane.com/

#### Screeny programu z systemu Fedora 18 i Windows 7:

![gim-fedora18](https://dl.dropboxusercontent.com/sh/kitbmes32iwm7f8/W2yGBywspZ/ipup-fedora18.png) ![gim-window7](https://dl.dropboxusercontent.com/sh/kitbmes32iwm7f8/oLk1TsYGov/ipup-windows7.png)
