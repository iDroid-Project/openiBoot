iDroid Project openiBoot
---------------------------------------------------
Copyright (C) 2008 David Wang (planetbeing).

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

1. Compiling
---------------------------------------------------
**Build an ARM toolchain (only needs to be done once):**

`sudo toolchain/build-toolchain.sh make`
	
Wait, for a long time, as the toolchain is compiled.

**Compile OpeniBoot:**

To run openiboot from the recovery mode (a.k.a iboot), you’ll need to create an img3 image.
You will need a system capable of running x86 Linux binaries (Build requires libusb, libreadline and pthread). If compiling on 64-bit linux, you may need libc6-dev-i386 and type export ARCH=x86_64 before you proceed with the next steps, All community builds are run on an Ubuntu 10.04 Lucid Lynx system.

**For iPod touch, run:**
`PLATFORM=IPOD make openiboot.img3`

**For iPhone 2G, run:**
`PLATFORM=IPHONE make openiboot.img3`

**For iPhone 3G, run:**
`PLATFORM=3G make openiboot.img3`

**Compile all in tools/client/:**
`cd tools/client && make all && cd ..`

2. Installing
---------------------------------------------------
**Copy openiboot.img3 into tools/client**

**Put your phone into recovery mode**: hold home whilst powering on until you see the iTunes logo.

**Run:** 
`sudo tools/client/loadibec openiboot.img3`

You should now see openiBoot on your phone, use the volume buttons to scroll to the console icon, then press home

**Run the client:**
sudo client/oibc

*NOTE: If you are on a mac, you will most likely need to do this as soon as you have selected the console - Linux does not manifest this issue.*

You should now see the same output on your computer, as is on your phone's screen.

**Type**: 
`install` and press return

This will take a while, your NOR will be backed up during this process, and can be found in the current directory as norbackup.bin but will flash openiBoot to your phone.

3. Installing multitouch firmware
--------------------------------------------------
See the guides at the [iDroid Project Development site][1] for a HOWTO

4. Reporting issues/requesting features
--------------------------------------------------
The iDroid Project has an issue tracking system set up to handle bug reports & feature requests, please see [http://dev.idroidproject.org/projects/openiboot][2] for more information.


  [1]: http://dev.idroidproject.org/projects/openiboot/documents
  [2]: http://dev.idroidproject.org/projects/openiboot