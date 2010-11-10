#include "openiboot.h"
#include "usb.h"
#include "power.h"
#include "util.h"
#include "hardware/power.h"
#include "hardware/usb.h"
#include "timer.h"
#include "clock.h"
#include "interrupt.h"
#include "openiboot-asmhelpers.h"

static void change_state(USBState new_state);

static Boolean usb_inited = FALSE;

static USBState usb_state;
static USBDirection endpoint_directions[USB_NUM_ENDPOINTS];
static USBEndpointBidirTransferInfo endpointTransferInfos[USB_NUM_ENDPOINTS];

USBEPRegisters* InEPRegs;
USBEPRegisters* OutEPRegs;

static uint8_t* controlSendBuffer = NULL;
static uint8_t* controlRecvBuffer = NULL;

static USBEnumerateHandler enumerateHandler;
static USBStartHandler startHandler;

static Boolean txFifosUsed[USB_NUM_TX_FIFOS];
static uint32_t nextDptxfsizStartAddr;

// Descriptor/configuration functions and variables
static USBDeviceDescriptor deviceDescriptor;
static USBDeviceQualifierDescriptor deviceQualifierDescriptor;

static uint8_t numStringDescriptors;
static USBStringDescriptor** stringDescriptors;
static USBFirstStringDescriptor* firstStringDescriptor;

static USBConfiguration* configurations;

static void initializeDescriptors();

static uint8_t addConfiguration(uint8_t bConfigurationValue, uint8_t iConfiguration, uint8_t selfPowered, uint8_t remoteWakeup, uint16_t maxPower);

static void endConfiguration(USBConfiguration* configuration);

static USBInterface* addInterfaceDescriptor(USBConfiguration* configuration, uint8_t interface, uint8_t bAlternateSetting, uint8_t bInterfaceClass, uint8_t bInterfaceSubClass, uint8_t bInterfaceProtocol, uint8_t iInterface);

static uint8_t addEndpointDescriptor(USBInterface* interface, uint8_t endpoint, USBDirection direction, USBTransferType transferType, USBSynchronisationType syncType, USBUsageType usageType, uint16_t wMaxPacketSize, uint8_t bInterval);

static void releaseConfigurations();

static uint8_t addStringDescriptor(const char* descriptorString);
static void releaseStringDescriptors();

static uint32_t getConfigurationTree(int i, uint8_t speed_id, void* buffer);
static void setConfiguration(int i);

// Interrupt handling/transmitting
static void processSetupPacket(void);
static void usbIRQHandler(uint32_t token);

static void send(uint8_t endpoint);
static void receive(uint8_t endpoint);
static void sendControl(void * buffer, int bufferLen);
static void receiveControl(void);

static void handleEndpointInInterrupt(uint8_t endpoint);
static void handleEndpointOutInterrupt(uint8_t endpoint);
static void handleEndpointTransferCompleted(uint8_t endpoint, USBDirection direction);
static void handleTransferCompleted(USBTransfer * transfer);
static void handleControlSent(USBTransfer * transfer);

static void startTransfer(uint8_t endpoint, USBDirection direction, USBTransfer * transfer);
static void setupTransfer(uint8_t endpoint, USBDirection direction, void * buffer, int bufferLen, USBEndpointHandler handler);

static void setAddress(uint8_t address);
static void sendInNak(void);
static void flushTxFifo(uint32_t txFifo);
static void setStall(uint8_t endpoint, USBDirection direction, OnOff onoff);

// Setup/cleanup
static void clearEndpointConfiguration(uint8_t endpoint, USBDirection direction);

static void shutdownEndpoint(uint8_t endpoint, USBDirection direction);
static void disableEndpoint(uint8_t endpoint, USBDirection direction);
static void disableEndpointIn(uint8_t endpoint);

// Variables read from the usb hardware
static uint32_t hwMaxPacketCount;
static uint32_t hwMaxPacketSize;
static Boolean hwDedicatedFifoEnabled;

int usb_setup(USBEnumerateHandler hEnumerate, USBStartHandler hStart) {
	enumerateHandler = hEnumerate;
	startHandler = hStart;

	int i;

	if(usb_inited) {
		return 0;
	}

	InEPRegs = (USBEPRegisters*)(USB + USB_INREGS);
	OutEPRegs = (USBEPRegisters*)(USB + USB_OUTREGS);

	change_state(USBStart);

	initializeDescriptors();

#ifndef CONFIG_IPHONE_4
	// Power on hardware
	power_ctrl(POWER_USB, ON);
	udelay(USB_START_DELAYUS);
#endif

	// Set up the hardware
	clock_gate_switch(USB_OTGCLOCKGATE, ON);
	clock_gate_switch(USB_PHYCLOCKGATE, ON);

	// Generate a soft disconnect on host
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SFTDISCONNECT);
	udelay(USB_SFTDISCONNECT_DELAYUS);

	// power on OTG
	SET_REG(USB + PCGCCTL, (GET_REG(USB + PCGCCTL) & (~PCGCCTL_ONOFF_MASK)) | PCGCCTL_ON);
	udelay(USB_ONOFFSTART_DELAYUS);

#ifndef CONFIG_IPHONE_4
	// power on PHY
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_POWERON);
#else
	// reset unknown registers
	SET_REG(USB_PHY + OPHYUNK3, OPHYUNK3_START);
	// power down some things
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_PLLPOWERDOWN | OPHYPWR_XOPOWERDOWN);
	SET_REG(USB_PHY + OPHYUNK1, OPHYUNK1_START);
	SET_REG(USB_PHY + OPHYUNK2, OPHYUNK3_START);
#endif

	udelay(USB_PHYPWRPOWERON_DELAYUS);

	// select clock
#ifdef CONFIG_IPHONE_4
	uint32_t phyClockBits;
	switch (clock_get_frequency(FrequencyBaseUsbPhy)) {
		case OPHYCLK_SPEED_12MHZ:
			phyClockBits = OPHYCLK_CLKSEL_12MHZ;
			break;
		case OPHYCLK_SPEED_24MHZ:
			phyClockBits = OPHYCLK_CLKSEL_24MHZ;
			break;
		case OPHYCLK_SPEED_48MHZ:
			phyClockBits = OPHYCLK_CLKSEL_48MHZ;
			break;
		default:
			phyClockBits = OPHYCLK_CLKSEL_OTHER;
			break;
	}

	SET_REG(USB_PHY + OPHYCLK, (GET_REG(USB_PHY + OPHYCLK) & (~OPHYCLK_CLKSEL_MASK)) | phyClockBits);
#else
	//TODO: get clock_get_frequency working for S5L8900
	SET_REG(USB_PHY + OPHYCLK, (GET_REG(USB_PHY + OPHYCLK) & (~OPHYCLK_CLKSEL_MASK)) | OPHYCLK_CLKSEL_48MHZ);
#endif

	// reset phy
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) | ORSTCON_PHYSWRESET);
	udelay(USB_RESET2_DELAYUS);
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) & (~ORSTCON_PHYSWRESET));
	udelay(USB_RESET_DELAYUS);

	// Initialize our data structures
	for(i = 0; i < USB_NUM_ENDPOINTS; i++) {
		switch(USB_EP_DIRECTION(i)) {
			case USB_ENDPOINT_DIRECTIONS_BIDIR:
				endpoint_directions[i] = USBBiDir;
				break;
			case USB_ENDPOINT_DIRECTIONS_IN:
				endpoint_directions[i] = USBIn;
				break;
			case USB_ENDPOINT_DIRECTIONS_OUT:
				endpoint_directions[i] = USBOut;
				break;
		}
		bufferPrintf("EP %d: %d\r\n", i, endpoint_directions[i]);
	}

	if(controlSendBuffer == NULL)
		controlSendBuffer = memalign(DMA_ALIGN, CONTROL_SEND_BUFFER_LEN);

	if(controlRecvBuffer == NULL)
		controlRecvBuffer = memalign(DMA_ALIGN, CONTROL_RECV_BUFFER_LEN);

	memset(endpointTransferInfos, 0, sizeof(endpointTransferInfos));
	memset(txFifosUsed, 0, sizeof(txFifosUsed));

	interrupt_install(USB_INTERRUPT, usbIRQHandler, 0);

	bufferPrintf("USB: Hardware Configuration\n"
		"	HWCFG1 = 0x%08x\n"
		"	HWCFG2 = 0x%08x\n"
		"	HWCFG3 = 0x%08x\n"
		"	HWCFG4 = 0x%08x\n",
		GET_REG(USB+GHWCFG1),
		GET_REG(USB+GHWCFG2),
		GET_REG(USB+GHWCFG3),
		GET_REG(USB+GHWCFG4));

	usb_start();

	interrupt_enable(USB_INTERRUPT);

	usb_inited = TRUE;

	return 0;
}

int usb_start(void) {
	// check some hardware bits for the configuration that we are running on
	if (GET_REG(USB + GHWCFG4) & GHWCFG4_DED_FIFO_EN) {
		hwDedicatedFifoEnabled = TRUE;
	} else {
		hwDedicatedFifoEnabled = FALSE;
	}

	hwMaxPacketCount = ((1 << (GET_BITS(USB + GHWCFG3, 4, 3) + 4))-1);
	hwMaxPacketSize = ((1 << (GET_BITS(USB + GHWCFG3, 0, 4) + 0xB))-1);

	// generate a usb core reset
	SET_REG(USB + GRSTCTL, GRSTCTL_CORESOFTRESET);

	// wait until reset takes
	while((GET_REG(USB + GRSTCTL) & GRSTCTL_CORESOFTRESET) == GRSTCTL_CORESOFTRESET);

	// wait until reset completes
	while((GET_REG(USB + GRSTCTL) & GRSTCTL_AHBIDLE) != GRSTCTL_AHBIDLE);

	// allow host to reconnect
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) & (~DCTL_SFTDISCONNECT));
	udelay(USB_SFTCONNECT_DELAYUS);

	SET_REG(USB + GAHBCFG, GAHBCFG_DMAEN | GAHBCFG_BSTLEN_INCR8 | GAHBCFG_MASKINT);
	SET_REG(USB + GUSBCFG, GUSBCFG_PHYIF16BIT | ((GUSBCFG_TURNAROUND & GUSBCFG_TURNAROUND_MASK) << GUSBCFG_TURNAROUND_SHIFT));

	if (hwDedicatedFifoEnabled) {
		SET_REG(USB + DCFG, DCFG_HISPEED | DCFG_NZSTSOUTHSHK);
	} else {
		SET_REG(USB + DCFG, DCFG_HISPEED | DCFG_NZSTSOUTHSHK | DCFG_EPMSCNT);
	}

	// disable all interrupts until endpoint descriptors and configuration structures have been setup
	SET_REG(USB + GINTMSK, GINTMSK_NONE);
	SET_REG(USB + DIEPMSK, USB_EPINT_NONE);
	SET_REG(USB + DOEPMSK, USB_EPINT_NONE);
/*
#ifndef CONFIG_IPHONE_4
	SET_REG(USB + DAINT, DAINT_ALL);
#endif
	SET_REG(USB + DAINTMSK, DAINTMSK_NONE);
#ifdef CONFIG_IPHONE_4
	SET_REG(USB + GINTSTS, USB_EPINT_ALL);
#endif
*/

	// set up endpoints
	int i;	
	for(i = 0; i < USB_NUM_ENDPOINTS; i++) {
		InEPRegs[i].interrupt = USB_EPINT_INTknTXFEmp | USB_EPINT_TimeOUT | USB_EPINT_AHBErr
			| USB_EPINT_EPDisbld | USB_EPINT_XferCompl;
		OutEPRegs[i].interrupt = USB_EPINT_SetUp | USB_EPINT_AHBErr | USB_EPINT_EPDisbld
			| USB_EPINT_XferCompl;
	}

	// enable the first interrupts that we will respond to
	SET_REG(USB + GINTMSK, GINTMSK_RESET | GINTMSK_ENUMDONE);

	change_state(USBPowered);

	return 0;
}

static int resetUSB() {
	disableEndpoint(USB_CONTROLEP, USBIn);
	setAddress(DCFG_DEVICEADDR_RESET);

	if (hwDedicatedFifoEnabled) {
		nextDptxfsizStartAddr = DPTXFSIZ_BASE_STARTADDR;
		memset(txFifosUsed, 0, sizeof(txFifosUsed));
	} else {
		//TODO: S5L8900 stuff
	}

	memset(endpointTransferInfos, 0, sizeof(endpointTransferInfos));

	SET_REG(USB + DOEPMSK, USB_EPINT_NONE);
	SET_REG(USB + DIEPMSK, USB_EPINT_NONE);
	SET_REG(USB + DAINTMSK, DAINTMSK_NONE);

	// set up endpoints
	int i;
	for(i = 0; i < USB_NUM_ENDPOINTS; i++) {
		InEPRegs[i].interrupt = USB_EPINT_INTknTXFEmp | USB_EPINT_TimeOUT | USB_EPINT_AHBErr
			| USB_EPINT_EPDisbld | USB_EPINT_XferCompl;
		OutEPRegs[i].interrupt = USB_EPINT_SetUp | USB_EPINT_AHBErr | USB_EPINT_EPDisbld
			| USB_EPINT_XferCompl;
	}

	SET_REG(USB + GRXFSIZ, GRXFSIZ_DEPTH);
	SET_REG(USB + GNPTXFSIZ, GNPTXFSIZ_STARTADDR | (GNPTXFSIZ_DEPTH << FIFO_DEPTH_SHIFT));

	endpointTransferInfos[USB_CONTROLEP].out.maxPacketSize = USB_CONTROLEP_MAX_TRANSFER_SIZE;
	endpointTransferInfos[USB_CONTROLEP].in.maxPacketSize = USB_CONTROLEP_MAX_TRANSFER_SIZE;

	InEPRegs[USB_CONTROLEP].control = USB_EPCON_NONE;
	OutEPRegs[USB_CONTROLEP].control = USB_EPCON_NONE;

	// enable the interrupts that we will be handling
	SET_REG(USB + GINTMSK, GET_REG(USB + GINTMSK) | GINTMSK_SUSPEND
		| GINTMSK_RESUME | GINTMSK_EPMIS | GINTMSK_INEP | GINTMSK_OEP);
	SET_REG(USB + DOEPMSK, USB_EPINT_SetUp | USB_EPINT_AHBErr | USB_EPINT_XferCompl);
	SET_REG(USB + DIEPMSK, USB_EPINT_TimeOUT | USB_EPINT_AHBErr | USB_EPINT_XferCompl);

	// enable interrupts for endpoint 0
	SET_REG(USB + DAINTMSK, GET_REG(USB + DAINTMSK)
		| ((1 << USB_CONTROLEP) << DAINTMSK_OUT_SHIFT)
		| ((1 << USB_CONTROLEP) << DAINTMSK_IN_SHIFT));

	receiveControl();

	return 0;
}

static void sendControl(void * buffer, int bufferLen) {
	USBTransfer * transfer = malloc(sizeof(USBTransfer));
	if (transfer == NULL) {
		setStall(USB_CONTROLEP, USBIn, TRUE);
	} else {
		if (buffer != NULL && buffer != controlSendBuffer) {
			// controlSendBuffer is used for all data transfers on ep 0 in
			memcpy(controlSendBuffer, buffer, bufferLen);
		}
		transfer->buffer = controlSendBuffer;
		transfer->size = bufferLen;
		transfer->handler = handleControlSent;
		startTransfer(USB_CONTROLEP, USBIn, transfer);
	}
}

static void handleControlSent(USBTransfer * transfer) {

}

void setupTransfer(uint8_t endpoint, USBDirection direction, void * buffer, int bufferLen, USBEndpointHandler handler) {
	USBTransfer * transfer = malloc(sizeof(USBTransfer));
	if (transfer != NULL) {
		transfer->buffer = buffer;
		transfer->size = bufferLen;
		transfer->handler = handler;
		startTransfer(endpoint, direction, transfer);
	}
}

static void startTransfer(uint8_t endpoint, USBDirection direction, USBTransfer * transfer) {
	transfer->bytesTransferred = 0;
	transfer->status = USBTransferInProgress;
	transfer->next = NULL;

	USBEndpointTransferInfo * endpointInfo;
	if (direction == USBIn) {
		endpointInfo = &(endpointTransferInfos[endpoint].in);
	} else {
		endpointInfo = &(endpointTransferInfos[endpoint].out);
	}

	if (endpointInfo->currentTransfer == NULL) {
		// no other transfers in queue
		endpointInfo->currentTransfer = transfer;
		endpointInfo->lastTransfer = transfer;
		if (direction == USBIn) {
			send(endpoint);
		} else {
			receive(endpoint);
		}
	} else {
		// add our transfer to the back of the queue
		endpointInfo->lastTransfer->next = transfer;
		endpointInfo->lastTransfer = transfer;
	}
}

void usb_send_bulk(uint8_t endpoint, void* buffer, int bufferLen, USBEndpointHandler handler) {
	setupTransfer(endpoint, USBIn, buffer, bufferLen, handler);
}

void usb_send_interrupt(uint8_t endpoint, void* buffer, int bufferLen, USBEndpointHandler handler) {
	setupTransfer(endpoint, USBIn, buffer, bufferLen, handler);
}

void usb_receive_bulk(uint8_t endpoint, void* buffer, int bufferLen, USBEndpointHandler handler) {
	setupTransfer(endpoint, USBOut, buffer, bufferLen, handler);
}

void usb_receive_interrupt(uint8_t endpoint, void* buffer, int bufferLen, USBEndpointHandler handler) {
	setupTransfer(endpoint, USBOut, buffer, bufferLen, handler);
}

static void usbIRQHandler(uint32_t token) {
	// get and acknowledge all interrupts
	uint32_t status = GET_REG(USB + GINTSTS);
	SET_REG(USB + GINTSTS, status);

	if (status & GINTMSK_OTG) {
		uint32_t otg_interrupt_status = GET_REG(USB + GOTGINT);
		if ((otg_interrupt_status & GINTMSK_OTG) == GINTMSK_OTG) {
			SET_REG(USB + GOTGINT, otg_interrupt_status);			
			shutdownEndpoint(1, USBIn);
			shutdownEndpoint(2, USBOut);
			shutdownEndpoint(3, USBIn);
			shutdownEndpoint(4, USBOut);
			usb_start();
		}
	}

	if (status & GINTMSK_RESET) {
		if(usb_state < USBError) {
			bufferPrintf("usb: reset detected\r\n");
			change_state(USBPowered);
		}

		int retval = resetUSB();

		if(retval) {
			bufferPrintf("usb: listening for further usb events\r\n");
			return;
		}
	}

	if (status & GINTMSK_ENUMDONE) {
		// iboot reads this for some reason
		GET_REG(USB + DSTS);
	}

	if ((status & GINTMSK_INEP) || (status & GINTMSK_OEP)) {
		uint32_t daint_status = GET_REG(USB + DAINT);
		if (daint_status != DAINT_NONE) {
			// aha, got something on one of the endpoints. Now the real fun begins

			// control endpoint in
			if (daint_status & ((1 << USB_CONTROLEP) << DAINTMSK_IN_SHIFT)) {
				handleEndpointInInterrupt(USB_CONTROLEP);
			}

			// control endpoint out
			if (daint_status & ((1 << USB_CONTROLEP) << DAINTMSK_OUT_SHIFT)) {
				uint32_t control_ep_out_status = OutEPRegs[USB_CONTROLEP].interrupt;
				OutEPRegs[USB_CONTROLEP].interrupt = control_ep_out_status;
				udelay(USB_INTERRUPT_CLEAR_DELAYUS);

				if (control_ep_out_status & USB_EPINT_SetUp) {
					// received a setup packet					
					processSetupPacket();

					// get the next SETUP packet
					receiveControl();
				} else {
					uint32_t transferSize;
					if (control_ep_out_status & USB_EPINT_XferCompl) {
						transferSize = USB_CONTROLEP_MAX_TRANSFER_SIZE - (OutEPRegs[USB_CONTROLEP].transferSize
							& DEPTSIZ0_XFERSIZ_MASK);
						OutEPRegs[USB_CONTROLEP].transferSize = 0;
					} else {
						transferSize = 0;
					}

					receiveControl();
				}
			}

			// check for interrupts on the non-control endpoints
			int endpoint;
			for (endpoint = 1; endpoint < USB_NUM_ENDPOINTS; endpoint++) {
				if (daint_status & ((1 << endpoint) << DAINTMSK_IN_SHIFT)) {
					handleEndpointInInterrupt(endpoint);
				}
				if (daint_status & ((1 << endpoint) << DAINTMSK_OUT_SHIFT)) {
					handleEndpointOutInterrupt(endpoint);
				}
			}
		}
	}

	if (status & GINTMSK_EPMIS) {
	}
}

static void processSetupPacket(void) {
	// recall our earlier receiveControl calls. We now should have 8 bytes of goodness in controlRecvBuffer.
	USBSetupPacket* setupPacket = (USBSetupPacket*) controlRecvBuffer;

	uint16_t length;
	uint32_t totalLength;
	USBStringDescriptor* strDesc;
	uint32_t requestType = USBSetupPacketRequestTypeType(setupPacket->bmRequestType);
	switch(requestType) {
		case USBSetupPacketStandard:
			switch(setupPacket->bRequest) {
				case USB_GET_DESCRIPTOR:
					length = setupPacket->wLength;
					// descriptor type is high, descriptor index is low
					int stall = FALSE;
					switch(setupPacket->wValue >> 8) {
						case USBDeviceDescriptorType:
							if(length > sizeof(USBDeviceDescriptor))
								length = sizeof(USBDeviceDescriptor);

							memcpy(controlSendBuffer, usb_get_device_descriptor(), length);
							break;
						case USBConfigurationDescriptorType:
							// hopefully SET_ADDRESS was received beforehand to set the speed
							totalLength = getConfigurationTree(setupPacket->wValue & 0xFF, DSTS_GET_SPEED(GET_REG(USB + DSTS)), controlSendBuffer);
							if(length > totalLength)
								length = totalLength;
							break;
						case USBStringDescriptorType:
							strDesc = usb_get_string_descriptor(setupPacket->wValue & 0xFF);
							if(length > strDesc->bLength)
								length = strDesc->bLength;
							memcpy(controlSendBuffer, strDesc, length);
							break;
						case USBDeviceQualifierDescriptorType:
							if(length > sizeof(USBDeviceQualifierDescriptor))
								length = sizeof(USBDeviceQualifierDescriptor);

							memcpy(controlSendBuffer, usb_get_device_qualifier_descriptor(), length);
							break;
						default:
							bufferPrintf("Unknown descriptor request: %d\r\n", setupPacket->wValue >> 8);
							stall = TRUE;
					}

					if(usb_state < USBError) {
						if(stall) {
							setStall(USB_CONTROLEP, USBIn, TRUE);
						} else {
							sendControl(controlSendBuffer, length);
						}
					}

					break;

				case USB_GET_STATUS:
					// send an acknowledgement
					sendControl(NULL, 0);
					break;

				case USB_SET_ADDRESS:
					SET_REG(USB + DCFG, (GET_REG(USB + DCFG) & ~DCFG_DEVICEADDRMSK)
						| ((setupPacket->wValue & DCFG_DEVICEADDR_UNSHIFTED_MASK) << DCFG_DEVICEADDR_SHIFT));

					// send an acknowledgement
					sendControl(NULL, 0);

					if(usb_state < USBError) {
						change_state(USBAddress);
					}
					break;

				case USB_SET_CONFIGURATION:
					setConfiguration(0);
					// send an acknowledgment
					sendControl(NULL, 0);

					if(usb_state < USBError) {
						change_state(USBConfigured);
						startHandler();
					}
					break;

				case USB_SET_INTERFACE:
					// send an acknowledgement
					sendControl(NULL, 0);
					break;

				case USB_GET_CONFIGURATION:
					// we only use configuration 0
					*controlSendBuffer = '0';
					sendControl(controlSendBuffer, 0);
					break;

				default:
					if(usb_state < USBError) {
						change_state(USBUnknownRequest);
					}
			}

			break;
		default:
			// we don't care about class or vendor requests
			break;
	}
}

static void create_descriptors() {
	if(configurations == NULL) {
		deviceDescriptor.bLength = sizeof(USBDeviceDescriptor);
		deviceDescriptor.bDescriptorType = USBDeviceDescriptorType;
		deviceDescriptor.bcdUSB = USB_2_0;
		deviceDescriptor.bDeviceClass = 0;
		deviceDescriptor.bDeviceSubClass = 0;
		deviceDescriptor.bDeviceProtocol = 0;
		deviceDescriptor.bMaxPacketSize = USB_MAX_PACKETSIZE;
		deviceDescriptor.idVendor = VENDOR_NETCHIP;
		deviceDescriptor.idProduct = PRODUCT_IPHONE;
		deviceDescriptor.bcdDevice = DEVICE_IPHONE;
		deviceDescriptor.iManufacturer = addStringDescriptor("Apple Inc.");
		deviceDescriptor.iProduct = addStringDescriptor("Apple Mobile Device (OpenIBoot Mode)");
		deviceDescriptor.iSerialNumber = addStringDescriptor("");
		deviceDescriptor.bNumConfigurations = 0;

		deviceQualifierDescriptor.bDescriptorType = USBDeviceDescriptorType;
		deviceQualifierDescriptor.bcdUSB = USB_2_0;
		deviceQualifierDescriptor.bDeviceClass = 0;
		deviceQualifierDescriptor.bDeviceSubClass = 0;
		deviceQualifierDescriptor.bDeviceProtocol = 0;
		deviceDescriptor.bMaxPacketSize = USB_MAX_PACKETSIZE;
		deviceDescriptor.bNumConfigurations = 0;

		addConfiguration(1, addStringDescriptor("OpenIBoot Mode Configuration"), 0, 0, 500);
	}
}

USBDeviceDescriptor* usb_get_device_descriptor() {
	create_descriptors();
	return &deviceDescriptor;
}

USBDeviceQualifierDescriptor* usb_get_device_qualifier_descriptor() {
	create_descriptors();
	return &deviceQualifierDescriptor;
}

static void setConfiguration(int i) {
	int8_t j;
	for(j = 0; j < configurations[i].descriptor.bNumInterfaces; j++) {
		int8_t k;
		for(k = 0; k < configurations[i].interfaces[j].descriptor.bNumEndpoints; k++) {
			int endpoint = configurations[i].interfaces[j].endpointDescriptors[k].bEndpointAddress & 0x3;
			if((configurations[i].interfaces[j].endpointDescriptors[k].bEndpointAddress & (0x1 << 7)) == (0x1 << 7)) {
				InEPRegs[endpoint].control = InEPRegs[endpoint].control | DCTL_SETD0PID;
			} else {
				OutEPRegs[endpoint].control = OutEPRegs[endpoint].control | DCTL_SETD0PID;
			}
		}
	}
}

static uint32_t getConfigurationTree(int i, uint8_t speed_id, void* buffer) {
	uint8_t *buf = (uint8_t*) buffer;
	uint32_t pos = 0;

	if(configurations == NULL) {
		return 0;
	}

	memcpy(buf + pos, usb_get_configuration_descriptor(i, speed_id), sizeof(USBConfigurationDescriptor));
	pos += sizeof(USBConfigurationDescriptor);

	int8_t j;
	for(j = 0; j < configurations[i].descriptor.bNumInterfaces; j++) {
		memcpy(buf + pos, &configurations[i].interfaces[j].descriptor, sizeof(USBInterfaceDescriptor));
		pos += sizeof(USBInterfaceDescriptor);
		int8_t k;
		for(k = 0; k < configurations[i].interfaces[j].descriptor.bNumEndpoints; k++) {
			memcpy(buf + pos, &configurations[i].interfaces[j].endpointDescriptors[k], sizeof(USBEndpointDescriptor));
			pos += sizeof(USBEndpointDescriptor);
		}
	}

	return pos;
}

USBConfigurationDescriptor* usb_get_configuration_descriptor(int index, uint8_t speed_id) {
	if(index == 0 && configurations[0].interfaces == NULL) {
		USBInterface* interface = addInterfaceDescriptor(&configurations[0], 0, 0,
			OPENIBOOT_INTERFACE_CLASS, OPENIBOOT_INTERFACE_SUBCLASS, OPENIBOOT_INTERFACE_PROTOCOL, addStringDescriptor("IF0"));

		enumerateHandler(interface);
		endConfiguration(&configurations[0]);
	}

	return &configurations[index].descriptor;
}

void usb_add_endpoint(USBInterface* interface, int endpoint, USBDirection direction, USBTransferType transferType) {
	if(transferType == USBInterrupt) {
		if(usb_get_speed() == USBHighSpeed)
			addEndpointDescriptor(interface, endpoint, direction, transferType, USBNoSynchronization, USBDataEndpoint, getPacketSizeFromSpeed(), 9);
		else
			addEndpointDescriptor(interface, endpoint, direction, transferType, USBNoSynchronization, USBDataEndpoint, getPacketSizeFromSpeed(), 32);
	} else {
		addEndpointDescriptor(interface, endpoint, direction, transferType, USBNoSynchronization, USBDataEndpoint, getPacketSizeFromSpeed(), 0);
	}
}

static void initializeDescriptors() {
	numStringDescriptors = 0;
	stringDescriptors = NULL;
	configurations = NULL;
	firstStringDescriptor = NULL;
	firstStringDescriptor = (USBFirstStringDescriptor*) malloc(sizeof(USBFirstStringDescriptor) + (sizeof(uint16_t) * 1));
	firstStringDescriptor->bLength = sizeof(USBFirstStringDescriptor) + (sizeof(uint16_t) * 1);
	firstStringDescriptor->bDescriptorType = USBStringDescriptorType;
	firstStringDescriptor->wLANGID[0] = USB_LANGID_ENGLISH_US;
}

static void releaseConfigurations() {
	if(configurations == NULL) {
		return;
	}

	int8_t i;
	for(i = 0; i < deviceDescriptor.bNumConfigurations; i++) {
		int8_t j;
		for(j = 0; j < configurations[i].descriptor.bNumInterfaces; j++) {
			free(configurations[i].interfaces[j].endpointDescriptors);
		}
		free(configurations[i].interfaces);
	}

	free(configurations);
	deviceDescriptor.bNumConfigurations = 0;
	configurations = NULL;
}

static uint8_t addConfiguration(uint8_t bConfigurationValue, uint8_t iConfiguration, uint8_t selfPowered, uint8_t remoteWakeup, uint16_t maxPower) {
	uint8_t newIndex = deviceDescriptor.bNumConfigurations;
	deviceDescriptor.bNumConfigurations++;
	deviceQualifierDescriptor.bNumConfigurations++;

	configurations = (USBConfiguration*) realloc(configurations, sizeof(USBConfiguration) * deviceDescriptor.bNumConfigurations);
	configurations[newIndex].descriptor.bLength = sizeof(USBConfigurationDescriptor);
	configurations[newIndex].descriptor.bDescriptorType = USBConfigurationDescriptorType;
	configurations[newIndex].descriptor.wTotalLength = 0;
	configurations[newIndex].descriptor.bNumInterfaces = 0;
	configurations[newIndex].descriptor.bConfigurationValue = bConfigurationValue;
	configurations[newIndex].descriptor.iConfiguration = iConfiguration;
	configurations[newIndex].descriptor.bmAttributes = ((0x1) << 7) | ((selfPowered & 0x1) << 6) | ((remoteWakeup & 0x1) << 5);
	configurations[newIndex].descriptor.bMaxPower = maxPower / 2;
	configurations[newIndex].interfaces = NULL;

	return newIndex;
}

static void endConfiguration(USBConfiguration* configuration) {
	configuration->descriptor.wTotalLength = sizeof(USBConfigurationDescriptor);

	int i;
	for(i = 0; i < configurations->descriptor.bNumInterfaces; i++) {
		configuration->descriptor.wTotalLength += sizeof(USBInterfaceDescriptor) + (configuration->interfaces[i].descriptor.bNumEndpoints * sizeof(USBEndpointDescriptor));
	}
}

static USBInterface* addInterfaceDescriptor(USBConfiguration* configuration, uint8_t bInterfaceNumber, uint8_t bAlternateSetting, uint8_t bInterfaceClass, uint8_t bInterfaceSubClass, uint8_t bInterfaceProtocol, uint8_t iInterface) {
	uint8_t newIndex = configuration->descriptor.bNumInterfaces;
	configuration->descriptor.bNumInterfaces++;

	configuration->interfaces = (USBInterface*) realloc(configuration->interfaces, sizeof(USBInterface) * configuration->descriptor.bNumInterfaces);
	configuration->interfaces[newIndex].descriptor.bLength = sizeof(USBInterfaceDescriptor);
	configuration->interfaces[newIndex].descriptor.bDescriptorType = USBInterfaceDescriptorType;
	configuration->interfaces[newIndex].descriptor.bInterfaceNumber = bInterfaceNumber;
	configuration->interfaces[newIndex].descriptor.bAlternateSetting = bAlternateSetting;
	configuration->interfaces[newIndex].descriptor.bInterfaceClass = bInterfaceClass;
	configuration->interfaces[newIndex].descriptor.bInterfaceSubClass = bInterfaceSubClass;
	configuration->interfaces[newIndex].descriptor.bInterfaceProtocol = bInterfaceProtocol;
	configuration->interfaces[newIndex].descriptor.iInterface = iInterface;
	configuration->interfaces[newIndex].descriptor.bNumEndpoints = 0;
	configuration->interfaces[newIndex].endpointDescriptors = NULL;

	return &configuration->interfaces[newIndex];
}

static uint8_t addEndpointDescriptor(USBInterface* interface, uint8_t endpoint, USBDirection direction, USBTransferType transferType, USBSynchronisationType syncType, USBUsageType usageType, uint16_t wMaxPacketSize, uint8_t bInterval) {
	if(direction > 2)
		return -1;

	uint8_t newIndex = interface->descriptor.bNumEndpoints;
	interface->descriptor.bNumEndpoints++;

	interface->endpointDescriptors = (USBEndpointDescriptor*) realloc(interface->endpointDescriptors, sizeof(USBEndpointDescriptor) * interface->descriptor.bNumEndpoints);
	interface->endpointDescriptors[newIndex].bLength = sizeof(USBEndpointDescriptor);
	interface->endpointDescriptors[newIndex].bDescriptorType = USBEndpointDescriptorType;
	interface->endpointDescriptors[newIndex].bEndpointAddress = (endpoint & 0xF) | ((direction & 0x1) << 7);	// see USB specs for the bitfield spec
	interface->endpointDescriptors[newIndex].bmAttributes = (transferType & 0x3) | ((syncType & 0x3) << 2) | ((usageType & 0x3) << 4);
	interface->endpointDescriptors[newIndex].wMaxPacketSize = wMaxPacketSize;
	interface->endpointDescriptors[newIndex].bInterval = bInterval;

	return newIndex;
}

static uint8_t addStringDescriptor(const char* descriptorString) {
	uint8_t newIndex = numStringDescriptors;
	numStringDescriptors++;

	stringDescriptors = (USBStringDescriptor**) realloc(stringDescriptors, sizeof(USBStringDescriptor*) * numStringDescriptors);

	int sLen = strlen(descriptorString);
	stringDescriptors[newIndex] = (USBStringDescriptor*) malloc(sizeof(USBStringDescriptor) + sLen * 2);
	stringDescriptors[newIndex]->bLength = sizeof(USBStringDescriptor) + sLen * 2;
	stringDescriptors[newIndex]->bDescriptorType = USBStringDescriptorType;
	uint16_t* string = (uint16_t*) stringDescriptors[newIndex]->bString;
	int i;
	for(i = 0; i < sLen; i++) {
		string[i] = descriptorString[i];
	}

	return (newIndex + 1);
}

USBStringDescriptor* usb_get_string_descriptor(int index) {
	if(index == 0) {
		return (USBStringDescriptor*) firstStringDescriptor;
	} else {
		return stringDescriptors[index - 1];
	}
}

static void releaseStringDescriptors() {
	int8_t i;

	if(stringDescriptors == NULL) {
		return;
	}

	for(i = 0; i < numStringDescriptors; i++) {
		free(stringDescriptors[i]);
	}

	free(stringDescriptors);

	numStringDescriptors = 0;
	stringDescriptors = NULL;
}

int usb_shutdown() {
#ifndef CONFIG_IPHONE_4
	power_ctrl(POWER_USB, ON);
#endif

	clock_gate_switch(USB_OTGCLOCKGATE, ON);
	clock_gate_switch(USB_PHYCLOCKGATE, ON);

	SET_REG(USB + PCGCCTL, GET_REG(USB + PCGCCTL) | PCGCCTL_OFF); // reset link
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_FORCESUSPEND | OPHYPWR_PLLPOWERDOWN
		| OPHYPWR_XOPOWERDOWN | OPHYPWR_ANALOGPOWERDOWN | OPHYPWR_UNKNOWNPOWERDOWN); // power down phy

	// reset phy/link
#ifdef CONFIG_IPHONE_4
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) | ORSTCON_PHYSWRESET);
#else
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) | ORSTCON_LINKSWRESET | ORSTCON_PHYLINKSWRESET);
#endif
	udelay(USB_RESET_DELAYUS);	// wait a millisecond for the changes to stick

#ifdef CONFIG_IPHONE_4
	// reset unknown register
	SET_REG(USB_PHY + OPHYUNK1, GET_REG(USB_PHY + OPHYUNK1) & (~OPHYUNK1_STOP_MASK));
#endif

	clock_gate_switch(USB_OTGCLOCKGATE, OFF);
	clock_gate_switch(USB_PHYCLOCKGATE, OFF);

#ifndef CONFIG_IPHONE_4
	power_ctrl(POWER_USB, OFF);
#endif

	releaseConfigurations();
	releaseStringDescriptors();

	return 0;
}

static void change_state(USBState new_state) {
	bufferPrintf("USB state change: %d -> %d\r\n", usb_state, new_state);
	usb_state = new_state;
	if(usb_state == USBConfigured) {
		// TODO: set to host powered
	}
}

USBSpeed usb_get_speed() {
	switch(DSTS_GET_SPEED(GET_REG(USB + DSTS))) {
		case USB_HIGHSPEED:
			return USBHighSpeed;
		case USB_FULLSPEED:
		case USB_FULLSPEED_48_MHZ:
			return USBFullSpeed;
		case USB_LOWSPEED:
			return USBLowSpeed;
	}

	return USBLowSpeed;
}

static void sendInNak(void) {
	SET_REG(USB + GINTSTS, GINTMSK_INNAK);
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SGNPINNAK);
	while ((GET_REG(USB + GINTSTS) & GINTMSK_INNAK) != GINTMSK_INNAK);
	SET_REG(USB + GINTSTS, GINTMSK_INNAK);
}

static void disableEndpointIn(uint8_t endpoint) {
	if (InEPRegs[endpoint].control & USB_EPCON_ENABLE) {
		sendInNak();

		InEPRegs[endpoint].control |= USB_EPCON_SETNAK;
		while ((InEPRegs[endpoint].interrupt & USB_EPINT_INEPNakEff) != USB_EPINT_INEPNakEff);
		InEPRegs[endpoint].control |= USB_EPCON_SETNAK | USB_EPCON_DISABLE;
		while ((InEPRegs[endpoint].interrupt & USB_EPINT_EPDisbld) != USB_EPINT_EPDisbld);
		InEPRegs[endpoint].interrupt = InEPRegs[endpoint].interrupt;

		uint32_t packetsSent;
		if (endpoint == USB_CONTROLEP) {
			packetsSent = USB_EPXFERSZ_PKTCNT_BIT_0(InEPRegs[endpoint].transferSize);
		} else {
			packetsSent = USB_EPXFERSZ_PKTCNT(InEPRegs[endpoint].transferSize);
		}

		// discard anything remaining in the current transfer for the endpoint
		if (packetsSent) {
			endpointTransferInfos[endpoint].in.currentTransfer->bytesTransferred +=
				(endpointTransferInfos[endpoint].in.currentTransferPacketsLeft - packetsSent) *
				endpointTransferInfos[endpoint].in.maxPacketSize;
		} else {
			endpointTransferInfos[endpoint].in.currentTransfer->bytesTransferred +=
				endpointTransferInfos[endpoint].in.currentPacketSize;
		}

		flushTxFifo(endpointTransferInfos[endpoint].in.txFifo);

		SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_CGNPINNAK);
	}
}

static void flushTxFifo(uint32_t txFifo) {
	SET_REG(USB + GRSTCTL, GET_REG(USB + GRSTCTL) & ~GRSTCTL_TXFNUM_MASK);
	SET_REG(USB + GRSTCTL, (GET_REG(USB + GRSTCTL) & ~GRSTCTL_TXFNUM_MASK) | 
		GRSTCTL_UNKN1 | (txFifo << GRSTCTL_UNKN_TX_FIFO_SHIFT));
	while (GET_REG(USB + GRSTCTL) & GRSTCTL_TXFFLSH);
}

static void setStall(uint8_t endpoint, USBDirection direction, OnOff onoff) {
	if (endpoint >= USB_NUM_ENDPOINTS) {
		return;
	}

	if (onoff == ON) {
		// set the stall bit for the endpoint
		if (direction == USBIn) {
			InEPRegs[endpoint].control |= USB_EPCON_STALL;
		} else if (direction == USBOut) {
			OutEPRegs[endpoint].control |= USB_EPCON_STALL;
		}
	} else {
		// remove the stall bit for the endpoint
		if (endpoint == USB_CONTROLEP) {
			return;
		}
		if (direction == USBIn) {
			InEPRegs[endpoint].control &= ~USB_EPCON_STALL;
		} else if (direction == USBOut) {
			OutEPRegs[endpoint].control &= ~USB_EPCON_STALL;
		}
	}
}

static void clearEndpointConfiguration(uint8_t endpoint, USBDirection direction) {
	USBEndpointTransferInfo * endpointTransferInfo;
	if (direction == USBIn) {
		InEPRegs[endpoint].control = USB_EPCON_NONE;
		InEPRegs[endpoint].transferSize = USB_EPXFERSZ_NONE;
		InEPRegs[endpoint].interrupt = USB_EPINT_ALL;
		InEPRegs[endpoint].dmaAddress = NULL;
		endpointTransferInfo = &(endpointTransferInfos[endpoint].in);
	} else if (direction == USBOut) {
		OutEPRegs[endpoint].control = USB_EPCON_NONE;
		OutEPRegs[endpoint].transferSize = USB_EPXFERSZ_NONE;
		OutEPRegs[endpoint].interrupt = USB_EPINT_ALL;
		OutEPRegs[endpoint].dmaAddress = NULL;
		endpointTransferInfo = &(endpointTransferInfos[endpoint].out);
	}

	if (endpointTransferInfo->txFifo) {
		txFifosUsed[endpointTransferInfo->txFifo - 1] = FALSE;
	}

	memset(endpointTransferInfo, 0, sizeof(USBEndpointTransferInfo));
}

static void shutdownEndpoint(uint8_t endpoint, USBDirection direction) {
	disableEndpoint(endpoint, direction);
	clearEndpointConfiguration(endpoint, direction);
}

static void disableEndpoint(uint8_t endpoint, USBDirection direction) {
	USBEndpointTransferInfo * endpointTransferInfo;
	if (direction == USBIn) {
		if (OutEPRegs[endpoint].control & USB_EPCON_ENABLE) {
			if (hwDedicatedFifoEnabled) {
				disableEndpointIn(endpoint);
			} else {
				//TODO: S5L8900 only
			}
		}
		endpointTransferInfo = &(endpointTransferInfos[endpoint].in);
	} else {
		if (OutEPRegs[endpoint].control & USB_EPCON_ENABLE) {
			// send an out nak
			SET_REG(USB + GINTSTS, GINTMSK_OUTNAK);
			SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SGOUTNAK);
			while ((GET_REG(USB + GINTSTS) & GINTMSK_OUTNAK) != GINTMSK_OUTNAK);
			SET_REG(USB + GINTSTS, GINTMSK_OUTNAK);

			// disable the endpoint
			OutEPRegs[endpoint].control |= USB_EPCON_SETNAK | USB_EPCON_DISABLE;
			while ((OutEPRegs[endpoint].interrupt & USB_EPINT_EPDisbld) != USB_EPINT_EPDisbld);
			OutEPRegs[endpoint].interrupt = OutEPRegs[endpoint].interrupt;
			SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_CGOUTNAK);
		}
		endpointTransferInfo = &(endpointTransferInfos[endpoint].out);
	}

	endpointTransferInfo->currentPacketSize = 0;
	endpointTransferInfo->currentTransferPacketsLeft = 0;
	if (endpointTransferInfo->currentTransfer) {
		USBTransfer * transfer = endpointTransferInfo->currentTransfer;
		USBTransfer * nextTransfer = transfer;
		endpointTransferInfo->currentTransfer = NULL;
		endpointTransferInfo->lastTransfer = NULL;
		while (transfer) {
			nextTransfer = transfer->next;
			transfer->status = USBTransferFailed;
			handleTransferCompleted(transfer);
			transfer = nextTransfer;
		}
	}
}

void usb_setup_endpoint(uint8_t endpoint, USBDirection direction, USBTransferType type, int maxPacketSize, int token) {
	USBEndpointTransferInfo * endpointTransferInfo;
	if (direction == USBIn) {
		endpointTransferInfo = &(endpointTransferInfos[endpoint].in);
	} else {
		endpointTransferInfo = &(endpointTransferInfos[endpoint].out);
	}

	endpointTransferInfo->maxPacketSize = maxPacketSize;
	endpointTransferInfo->type = type;
	endpointTransferInfo->token = token;

	if (type == USBBulk || type == USBInterrupt) {
		volatile uint32_t * controlReg;
		uint32_t txFifo = 0;
		uint32_t endpointDaintMask;
		if (direction == USBIn) {
			controlReg = &(InEPRegs[endpoint].control);
			*controlReg = USB_EPCON_NONE;
			endpointDaintMask = 1 << (endpoint + DAINT_IN_SHIFT);
			if (hwDedicatedFifoEnabled) {
				// find an unused tx fifo for our endpoint and claim it
				while (txFifo != USB_NUM_TX_FIFOS) {
					if (txFifosUsed[txFifo] == FALSE) {
						txFifosUsed[txFifo] = TRUE;
						break;
					}
					txFifo++;
				}
				endpointTransferInfo->txFifo = txFifo + 1;		// 1-based
				SET_REG(USB + DIEPTXF(txFifo), nextDptxfsizStartAddr | (DPTXFSIZ_DEPTH << FIFO_DEPTH_SHIFT));
				nextDptxfsizStartAddr += DPTXFSIZ_DEPTH;
			} else {
				//TODO: S5L8900 only
			}
		} else {
			controlReg = &(OutEPRegs[endpoint].control);
			*controlReg = USB_EPCON_NONE;
			endpointDaintMask = 1 << (endpoint + DAINT_OUT_SHIFT);
		}
		*controlReg = maxPacketSize | ((type & USB_EPCON_TYPE_MASK) << USB_EPCON_TYPE_SHIFT)
			| USB_EPCON_ACTIVE | USB_EPCON_SET0PID | USB_EPCON_SETNAK
			| (txFifo ? (((txFifo + 1) & USB_EPCON_TXFNUM_MASK) << USB_EPCON_TXFNUM_SHIFT) : 0);
			/*TODO | S5L8900unkn */
		SET_REG(USB + DAINTMSK, GET_REG(USB + DAINTMSK) | endpointDaintMask);
	}
}

static void send(uint8_t endpoint) {
	USBTransfer * transfer = endpointTransferInfos[endpoint].in.currentTransfer;

	uint32_t packetSize;
	if (endpoint == USB_CONTROLEP) {
		packetSize = USB_CONTROLEP_MAX_TRANSFER_SIZE;
	} else {
		packetSize = hwMaxPacketSize;
	}
	if (packetSize > transfer->size - transfer->bytesTransferred) {
		packetSize = transfer->size - transfer->bytesTransferred;
	}

	CleanAndInvalidateCPUDataCache();

	InEPRegs[endpoint].dmaAddress = transfer->buffer + transfer->bytesTransferred;

	uint32_t maxPacketSize = endpointTransferInfos[endpoint].in.maxPacketSize;
	uint32_t packetCount;
	if (packetSize == 0) {
		packetCount = 1;
	} else {
		packetCount = (packetSize + maxPacketSize - 1) / maxPacketSize;
		if (packetCount > hwMaxPacketCount) {
			packetCount = hwMaxPacketCount;
			packetSize = hwMaxPacketCount * maxPacketSize;
		}
	}

	InEPRegs[endpoint].transferSize = ((packetCount & DEPTSIZ_PKTCNT_MASK) << DEPTSIZ_PKTCNT_SHIFT)
		| (packetSize & DEPTSIZ_XFERSIZ_MASK) | ((USB_MULTICOUNT & DIEPTSIZ_MC_MASK) << DIEPTSIZ_MC_SHIFT);
	InEPRegs[endpoint].control |= USB_EPCON_CLEARNAK | USB_EPCON_ENABLE;

	endpointTransferInfos[endpoint].in.currentPacketSize = packetSize;
	endpointTransferInfos[endpoint].in.currentTransferPacketsLeft = packetCount;
}

static void receive(uint8_t endpoint) {
	USBTransfer * transfer = endpointTransferInfos[endpoint].out.currentTransfer;

	uint32_t packetSize = transfer->size - transfer->bytesTransferred;
	if (packetSize > hwMaxPacketSize) {
		packetSize = hwMaxPacketSize;
	}

	CleanAndInvalidateCPUDataCache();

	OutEPRegs[endpoint].dmaAddress = transfer->buffer + transfer->bytesTransferred;

	uint32_t maxPacketSize = endpointTransferInfos[endpoint].out.maxPacketSize;
	uint32_t packetCount;
	if (packetSize == 0) {
		packetCount = 1;
	} else {
		packetCount = (packetSize + maxPacketSize - 1) / packetSize;
		if (packetCount > hwMaxPacketCount) {
			packetCount = hwMaxPacketCount;
			packetSize = hwMaxPacketCount * maxPacketSize;
		}
	}

	OutEPRegs[endpoint].transferSize = ((packetCount & DEPTSIZ_PKTCNT_MASK) << DEPTSIZ_PKTCNT_SHIFT)
		| (packetSize & DEPTSIZ_XFERSIZ_MASK) | ((USB_MULTICOUNT & DIEPTSIZ_MC_MASK) << DIEPTSIZ_MC_SHIFT);
	OutEPRegs[endpoint].control |= USB_EPCON_CLEARNAK | USB_EPCON_ENABLE;

	endpointTransferInfos[endpoint].out.currentPacketSize = packetSize;
	endpointTransferInfos[endpoint].out.currentTransferPacketsLeft = packetCount;
}

static void receiveControl(void) {
	OutEPRegs[USB_CONTROLEP].transferSize = ((USB_SETUP_PACKETS_AT_A_TIME & DOEPTSIZ0_SUPCNT_MASK) << DOEPTSIZ0_SUPCNT_SHIFT)
		| ((USB_SETUP_PACKETS_AT_A_TIME & DOEPTSIZ0_PKTCNT_MASK) << DEPTSIZ_PKTCNT_SHIFT)
		| (USB_CONTROLEP_MAX_TRANSFER_SIZE & DEPTSIZ0_XFERSIZ_MASK);

	CleanAndInvalidateCPUDataCache();

	OutEPRegs[USB_CONTROLEP].dmaAddress = controlRecvBuffer;

	OutEPRegs[USB_CONTROLEP].control |= USB_EPCON_CLEARNAK | USB_EPCON_ENABLE;
}

static void handleTransferCompleted(USBTransfer * transfer) {
	if (transfer->handler) {
		transfer->handler(transfer);
	}
	free(transfer);
}

uint16_t getPacketSizeFromSpeed(void) {
	if (usb_get_speed() == USBHighSpeed) {
		return 512;
	} else {
		return 64;
	}
}

static void setAddress(uint8_t address) {
	SET_REG(USB + DCFG, (GET_REG(USB + DCFG) & ~DCFG_DEVICEADDRMSK)
		| ((address & DCFG_DEVICEADDR_UNSHIFTED_MASK) << DCFG_DEVICEADDR_SHIFT));
}

static void handleEndpointInInterrupt(uint8_t endpoint) {
	// get and clear the interrupt register
	uint32_t status = InEPRegs[endpoint].interrupt;
	InEPRegs[endpoint].interrupt = status;

	USBEndpointTransferInfo * endpointInfo = &(endpointTransferInfos[endpoint].in);

	if (status & USB_EPINT_XferCompl) {
		endpointInfo->currentTransfer->bytesTransferred += endpointInfo->currentPacketSize;
		endpointInfo->currentTransferPacketsLeft = 0;
		if (endpointInfo->currentTransfer->bytesTransferred >= endpointInfo->currentTransfer->size) {
			handleEndpointTransferCompleted(endpoint, USBIn);
		} else {
			// the current transfer is still in progress, so send more of it
			send(endpoint);
		}
	}

	if (status & USB_EPINT_TimeOUT) {
		if (hwDedicatedFifoEnabled) {
			sendInNak();
			disableEndpointIn(endpoint);
			flushTxFifo(endpointInfo->txFifo);
			SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_CGNPINNAK);
			send(endpoint);
		} else {
			//TODO: S5L8900
		}
	}

	if (status & USB_EPINT_AHBErr) {
		// something really bad happened...
		uartPrintf("USB AHB error!");
		panic();
	}
}


static void handleEndpointOutInterrupt(uint8_t endpoint) {
	// get and clear the interrupt register
	uint32_t status = OutEPRegs[endpoint].interrupt;
	udelay(USB_INTERRUPT_CLEAR_DELAYUS);
	OutEPRegs[endpoint].interrupt = status;

	USBEndpointTransferInfo * endpointInfo = &(endpointTransferInfos[endpoint].out);

	if (status & USB_EPINT_XferCompl) {
		uint32_t bytesReceived = endpointInfo->currentPacketSize - (OutEPRegs[endpoint].transferSize & DEPTSIZ_XFERSIZ_MASK);
		endpointInfo->currentTransfer->bytesTransferred += bytesReceived;

		uint32_t extraBytes = bytesReceived % endpointInfo->maxPacketSize;
		if ((endpointInfo->currentPacketSize > bytesReceived && extraBytes != 0) || endpointInfo->currentPacketSize == 0) {
			// Something screwed up happened, so the transfer is over (either received a partial packet when it should
			// have been full or nothing left). iBoot treats this as a successful transfer...
			handleEndpointTransferCompleted(endpoint, USBOut);
		} else {
			// have some bytes left to transfer
			uint32_t packetsReceived = endpointInfo->currentTransferPacketsLeft - ((OutEPRegs[endpoint].transferSize
				>> DEPTSIZ_PKTCNT_SHIFT) & DEPTSIZ_PKTCNT_MASK) - 1;
			if ((extraBytes != 0) || (packetsReceived != (bytesReceived / endpointInfo->maxPacketSize))) {
				// partial transfer
				if (endpointInfo->currentTransfer->bytesTransferred >= endpointInfo->currentTransfer->size) {
					handleEndpointTransferCompleted(endpoint, USBOut);
				} else {
					receive(endpoint);
				}
			} else {
				// transfer complete
				handleEndpointTransferCompleted(endpoint, USBOut);
			}
		}
	}

	if (status & USB_EPINT_AHBErr) {
		// something really bad happened...
		uartPrintf("USB AHB error!");
		panic();
	}
}

static void handleEndpointTransferCompleted(uint8_t endpoint, USBDirection direction) {
	USBEndpointTransferInfo * endpointInfo;	
	if (direction == USBIn) {
		endpointInfo = &(endpointTransferInfos[endpoint].in);
	} else {
		endpointInfo = &(endpointTransferInfos[endpoint].out);
	}

	endpointInfo->currentTransferPacketsLeft = 0;
	endpointInfo->currentPacketSize = 0;
	USBTransfer * completedTransfer = endpointInfo->currentTransfer;

	// advance the transfer queue
	if (endpointInfo->currentTransfer == endpointInfo->lastTransfer) {
		endpointInfo->currentTransfer = NULL;
		endpointInfo->lastTransfer = NULL;
	} else {
		endpointInfo->currentTransfer = completedTransfer->next;
		completedTransfer->next = NULL;
		if (direction == USBIn) {
			send(endpoint);
		} else {
			receive(endpoint);
		}
	}

	// process the completed transfer
	completedTransfer->status = USBTransferSuccess;
	handleTransferCompleted(completedTransfer);
}
