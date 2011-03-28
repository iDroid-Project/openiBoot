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
**Build an ARM toolchain (this only needs to be done once - for subsequent builds this step can be omitted unless there has been a toolchain update):**

`sudo toolchain/build-toolchain.sh make`
	
Wait, for a long time, as the toolchain is compiled.

**Initialise Submodules**
`git submodule init`
`git submodule update`

**Compile OpeniBoot:**

To run openiboot from recovery mode (a.k.a iboot), you’ll need to create an img3 image.
To run openiboot from DFU mode, you'll need to create a bin.

You will need a system capable of running x86 Linux binaries (Build requires scons, libssl, libpng, libcurl, libusb, libreadline and pthread). 

Change into the openiboot subfolder

**For iPod Touch 1G, run:**
`scons iPod1G`

**For iPhone 2G, run:**
`scons iPhone2G`

**For iPhone 3G, run:**
`scons iPhone3G`

**For iPod Touch 2G, run:**
`scons iPod2G`

**For iPhone 3GS, run:**
`scons iPhone3GS`

**For iPhone 4, run:**
`scons iPhone4`

**For iPad, run:**
`scons iPad1G`

**For Apple TV 2G, run:**
`scons aTV2G`

**Compile syringe:**
`cd ../utils/syringe && make && cd ../openiboot`

*Alternatively a Makefile has been provided in the openiboot subfolder should you prefer to use it this way - this is not covered by this README but is provided for your convenience should you wish to use it*

**Compile oibc:**
`cd ../utils/oibc && make && cd ../openiboot`

2. Running/Installing
---------------------------------------------------
**If you're on linux, you'll need to install the following as /etc/udev/rules.d/51-android.rules:**
	SUBSYSTEM=="usb" ID_VENDOR_ID=="0bb4", MODE="0666"
	SUBSYSTEM=="usb" ID_VENDOR_ID=="18d1", MODE="0666"
	SUBSYSTEM=="usb" ID_VENDOR_ID=="05ac", MODE="0666"

**Put your phone into recovery mode**: hold home whilst powering on until you see the iTunes logo.

*NOTE: For the 3GS, iPhone4, iPad and Apple TV 2G, you must put your device into DFU mode instead.*

**For iPhone 2G, iPhone 3G & iPod Touch 1G run:** 
`../utils/syringe/loadibec openiboot.img3`

**For newer devices run: (substituting *device* and *revision* with the actual device, for example: iphone_4_openiboot.bin):**
`../utils/syringe/loadibec device_revision_openiboot.bin`

You should now see openiBoot on your phone, use the volume buttons to scroll to the console icon, then press home

**Run the client:**
`../utils/oibc/oibc`

*NOTE: You cannot install openiboot on an iPhone 4, iPad or new bootrom 3GS/iPT2G - you can only run it*

You should now see the same output on your computer, as is on your phone's screen.

**Type**: 
`install` and press return

OpeniBoot will then be flashed to your device's NOR - This will take a while, your NOR will be backed up during this process, and can be found in the current directory as norbackup.dump.

3. Reporting issues/requesting features
--------------------------------------------------
Please leave bug reports/pull requests in the Github tracker.

For anything else, we can be found lurking in #idroid-dev on irc.freenode.net
