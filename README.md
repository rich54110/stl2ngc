## Introduction

[![Join the chat at https://gitter.im/koppi/stl2ngc](https://badges.gitter.im/koppi/stl2ngc.svg)](https://gitter.im/koppi/stl2ngc?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

stl2ngc converts an [STL](https://en.wikipedia.org/wiki/STL_(file_format)) file to [LinuxCNC](http://linuxcnc.org/) compatible [G-Code](http://linuxcnc.org/docs/html/gcode.html).

### Build, install and run stl2ngc

First, install OpenCAMLib, (see: https://github.com/aewallin/opencamlib ):
```bash
sudo apt -y install cmake doxygen libboost-all-dev
git clone https://github.com/aewallin/opencamlib && cd opencamlib
mkdir build && cd build && cmake ../src && make -j4
sudo make install
sudo cp libocl.so.* /usr/lib
```

Next, checkout stl2ngc from git and run make and sudo make install:
```bash
git clone https://github.com/koppi/stl2ngc && cd stl2ngc
make
sudo make install
```

To convert ```example.stl``` to ```example.ngc``` run:
stl2ngc example.stl
 Or
 stl2ngc.exe --zsafe 15.00 --overlap 0.25 --xplain 0.0 --yplain 0.0 --zplain 0.0 --outfile example_fine.ngc -resize 0.0 --feed 200 --spindle 500  --zstep 50  --toollen 6.0 --tooldia 2.0 --threads 2 example.stl
 
 

### Demo

See [Wiki](../../wiki/).

### FAQ

* This is an early release, expect errors and missing features.

* No Gui? - Yes, only command-line for now.

### Authors

* [Richard Rehll](https://github.com/rich54110)

