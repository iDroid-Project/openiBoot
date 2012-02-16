#include "openiboot.h"
#include "usbphy.h"
#include "hardware/usbphy.h"
#include "timer.h"
#include "clock.h"

void usb_phy_init() {
	// power on PHY
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_POWERON);
	udelay(USB_PHYPWRPOWERON_DELAYUS);

	// select clock
	SET_REG(USB_PHY + OPHYCLK, (GET_REG(USB_PHY + OPHYCLK) & (~OPHYCLK_CLKSEL_MASK)) | OPHYCLK_CLKSEL_48MHZ);

	// reset phy
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) | ORSTCON_PHYSWRESET);
	udelay(USB_RESET2_DELAYUS);
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) & (~ORSTCON_PHYSWRESET));
	udelay(USB_RESET_DELAYUS);
}

void usb_phy_shutdown() {
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_FORCESUSPEND | OPHYPWR_PLLPOWERDOWN
		| OPHYPWR_XOPOWERDOWN | OPHYPWR_ANALOGPOWERDOWN | OPHYPWR_UNKNOWNPOWERDOWN); // power down phy
	SET_REG(USB_PHY + ORSTCON, ORSTCON_PHYSWRESET | ORSTCON_LINKSWRESET | ORSTCON_PHYLINKSWRESET); // reset phy/link

	udelay(USB_RESET_DELAYUS);	// wait a millisecond for the changes to stick
}

