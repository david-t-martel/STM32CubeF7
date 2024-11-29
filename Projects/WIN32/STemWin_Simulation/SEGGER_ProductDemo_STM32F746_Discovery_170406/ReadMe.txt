ReadMe.txt for STM32F746 Discovery prebuild binary

This package offers two prebuild binary files of a SEGGER product demo
application to run on a ST STM32F746G-Discovery. It is prepared for download into
internal flash using J-Link without the need for an IDE. An embOS stack-check and
profiling library is used.

Content of the demo application:
================================
embOS       - Shows a list of all the tasks running and provides information about
              the different tasks.
emFile      - This is a combination of emFile and emSecure. This application reads
              the images (*.bmp and *.jpeg) from the SD Card and displays those files.
              Further the emSecure part tries to verify the images with the signatures
              (*.*.sig). On success the images are getting displayed. On fail the images
              are getting displayed with a watermark.
              To have this working properly, please copy the content from \SDCard to a sd
              card and plug it into the board.
              Please make sure the image are in the proper size. The best size is 300x172 pixels.
              Anything above won't be displayed correctly.
emUSB       - This application shows how to realize a touch pad (like known from a notebook)
              and makes it possible to control the host PCs mouse.
              To have this application working properly make sure the board is connected via
              mini USB cabel on connector 'CN12 USB_HS'.
embOS/IP    - This application shows the network traffic related to the board. It displays
              received and send bytes. Of course this demo makes only sense if the VNC application
              is running and an ethernet cabel is connected. Further it shows the IP address
              of this board once a connection has been established.
VNC         - If the 'VNC on startup' version is running, VNC is always on. Otherwise it needs
              to enabled by tipping on the appropriate button. To connect to the board use a VNC
              client of your choice and use either the IP address displayed in the embOS/IP
              application or use 'STM32F746' as connection.
emWin       - This application shows a simple user interface to control the color of light bulbs.
Screensaver - If the application without VNC is running a screensaver will be shown after short
              period of inactivity.
           

How to run the prebuild application:
====================================
Per default the STM32F746 Discovery boards come with a ST-Link on board
debugging probe.

This needs to be upgraded to a J-Link OB. To do so please unzip the ST-Link
reflash tool from STLinkReflash_161221.zip, start STLinkReflash.exe and
follow the instructions.

The most recent version of this tool can be found here:
https://www.segger.com/downloads/jlink/STLinkReflash_161221.zip

Once the ST-Link is upgraded follow the steps below.

1. Make sure power selection jumper on the back of the board is fitted
   to '5V link'
2. Connect your target to your host PC with a mini USB cabel
3. Execute StartBinary.bat
4. Once the download is finished disconnect the USB cabel
5. Set the power selection jumper to 'USB_HS'
6. Connect a micro USB cabel to 'CN12 USB_HS'
