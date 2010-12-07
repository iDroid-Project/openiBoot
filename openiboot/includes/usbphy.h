#ifndef USBPHY_H
#define USBPHY_H

typedef enum USBChargerIdentificationMode {
	USBChargerIdentificationDN,
	USBChargerIdentificationDP,
	USBChargerIdentificationNone,
} USBChargerIdentificationMode;

void usb_phy_init();
void usb_phy_shutdown();
void usb_set_charger_identification_mode(USBChargerIdentificationMode mode);

#endif
