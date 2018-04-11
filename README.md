libscip2hat
============
Library for Hokuyo's laser range sensor " URG " 


Operating environment
---------------------
-  OSX
-  Linux
-  Microsoft Windows
    with MinGW/Cygwin
         and pthreads_win32 (http://sources.redhat.com/pthreads-win32/)


Build Install
-------------
 Build and install this project using cmake in the following procedure.
 Please install cmake in advance.

    $ git clone https://github.com/khattari540750/libscip2hat.git
    $ cd libscip2hat
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install


How to use
----------
We test the operation using the sample program.
In this program, the distance to the object in the front direction of the URG is displayed.


1. First, connect the URG to the PC and turn on the power.
2. Move from the directory of the package downloaded earlier to the directory where the sample is saved.
   `$ cd libscip2hat/sample`
3. Execute the sample program.
   `$./test-ms /dev/cu.usbmodem`
   When finishing,
   `control + c`
You can stop by pressing the button.
At this time, /dev/cu.usbmodem indicates the port information file to which URG is connected, and its character string will change depending on the environment.
When URG is plugged in or removed from a port, what appears or disappears in /dev/ is the corresponding port information file, so give it as an argument.
