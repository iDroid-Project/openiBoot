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

**Compile OpeniBoot:**

To run openiboot from the recovery mode (a.k.a iboot), you’ll need to create an img3 image.
You will need a system capable of running x86 Linux binaries (Build requires scons, libssl, libpng, libcurl, libusb, libreadline and pthread). 

If compiling on 64-bit linux, you may need libc6-dev-i386 and type export ARCH=x86_64 before you proceed with the next steps.

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

**Compile oibc:**
`cd ../utils/oibc && make && cd ../openiboot`

- The linux client will also build on OSX with the correct libusb version installed.

2. Installing
---------------------------------------------------
**If you're on linux, you'll need to install the following as /etc/udev/rules.d/51-android.rules:**
	SUBSYSTEM=="usb" ID_VENDOR_ID=="0bb4", MODE="0666"
	SUBSYSTEM=="usb" ID_VENDOR_ID=="18d1", MODE="0666"
	SUBSYSTEM=="usb" ID_VENDOR_ID=="05ac", MODE="0666"

**Put your phone into recovery mode**: hold home whilst powering on until you see the iTunes logo.

*NOTE: For the 3GS, iPhone4, iPad and Apple TV 2G, you must put your device into DFU mode instead.*

**Run:** 
`../utils/syringe/loadibec openiboot.img3`

You should now see openiBoot on your phone, use the volume buttons to scroll to the console icon, then press home

**Run the client:**
`../utils/oibc/oibc`

*NOTE: You cannot install openiboot on an iPhone 4, iPad or new bootrom 3GS/iPT2G - you can only run it*

You should now see the same output on your computer, as is on your phone's screen.

**Type**: 
`install` and press return

This will take a while, your NOR will be backed up during this process, and can be found in the current directory as norbackup.dump but will flash openiBoot to your phone.

3. Installing multitouch firmware for openiBoot (This is optional)
-------------------------------------------------------------------
A. iPhone 3G user (Convert zephyr2.bin):

 *  Copy your zephyr2.bin to openiboot/mk8900image
 *  Go to "openiboot/mk8900image"
 *  From "openiboot/mk8900image" execute this command (please adjust path to your firmware location): 

	>mk8900image zephyr2.bin zephyr2.img3 template-3g.img3

 *  Using your favorite hex editor, edit "zephyr2.img3"
     change data at offset 0x10 to 0x13 from *cebi* to *2ztm* and save it.

   2. Installing zephyr2.img3
*   Run oibc executable
*   From oibc prompt send "zephyr2.img3" to your iphone (default address is 0x09000000):

	>!zephyr2.img3

	It will return this: file received (XXXXX bytes).

*   Install the firmware from the address and with the size the upload command returned:

    >multitouch_fw_install 0x09000000 XXXXX


B. iPhone 2G user (Convert zephyr_aspeed.bin and zephyr_main.bin):

*	Copy your zephyr bins (zephyr_aspeed.bin and zephyr_main.bin) to openiboot/mk8900image
*   Go to "openiboot/mk8900image"
*   From "openiboot/mk8900image" execute this command (please adjust path to your firmware location): 

	>mk8900image zephyr_aspeed.bin to zephyr_aspeed.img3 template.img3

*   Using your favorite hex editor, edit "zephyr_aspeed.img3"
     change data at offset 0x10 to 0x13 from *cebi* to *aztm* and save it.

*   From "openiboot/mk8900image" execute this command (please adjust path to your firmware location): 

	>mk8900image zephyr_main.bin to zephyr_main.img3 template.img3

*   Using your favorite hex editor, edit "zephyr_main.img3"
     change data at offset 0x10 to 0x13 from *cebi* to *mztm* and save it.

   2. Installing firmwares
*   Run oibc executable
*   From oibc prompt send "zephyr_aspeed.img3" to your iphone (default address is 0x09000000):

	>!zephyr_aspeed.img3

     It will return this: file received (XXXXX bytes).
*   From oibc prompt send "zephyr_main.img3" to your iphone, in this case we will copy to memory location next to zephyr_aspeed.img:

	>!zephyr_main.img3@0x09000000+XXXXX

     It will return this: file received (YYYYY bytes).

*   Install the firmware from the address and with the size the upload command returned:

	>multitouch_fw_install 0x09000000 XXXXX 0x09000000+XXXX YYYYY


4. Reporting issues/requesting features
--------------------------------------------------
The iDroid Project has an issue tracking system set up to handle bug reports & feature requests, please see [http://dev.idroidproject.org/projects/openiboot][1] for more information.

For anything else, we can be found lurking in #idroid-dev on irc.osx86.hu

  [1]: http://dev.idroidproject.org/projects/openiboot
