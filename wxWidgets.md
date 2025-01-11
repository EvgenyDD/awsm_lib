# Windows
>> download sources for windows & unpack
>> MSYS2 MINGW64 console as Admin
* mkdir build && cd build
* ../configure --disable-shared --with-libtiff=builtin --with-libjpeg=builtin --with-libpng=builtin --with-liblzma=builtin --with-zlib=builtin --with-expat=builtin
* make -j
* make install
* (optional) cd samples/minimal && make

# Linux
* sudo apt-get install build-essential libgtk2.0-dev libgtk-3-dev mesa-utils freeglut3-dev libjpeg-dev liblzma-dev -y
>> download sources for linux
* tar -xvf wxWidgets-3*
* cd wxWidgets-3...
* mkdir build-gtk && cd build-gtk
* ../configure --with-gtk=3 --with-opengl --disable-shared --with-libtiff=builtin --with-libjpeg=builtin --with-libpng=builtin --with-liblzma=builtin --with-zlib=builtin --with-expat=builtin
* make -j
* sudo make install
* (optional) cd samples/minimal && make
