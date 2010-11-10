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

static Boolean usb_inited;

static USBState gUsbState;
static uint8_t usb_speed;
static uint8_t usb_max_packet_size;
static int usb_num_endpoints;
static USBFIFOMode usb_fifo_mode = FIFOOther;

static USBEndpointBidirHandlerInfo endpoint_handlers[USB_NUM_ENDPOINTS];
static uint32_t inInterruptStatus[USB_NUM_ENDPOINTS];
static uint32_t outInterruptStatus[USB_NUM_ENDPOINTS];
static USBMessageQueue *usb_message_queue[USB_NUM_ENDPOINTS];

static int usb_ep_queue_first = 0xFF;
static int usb_ep_queue_last = 0xFF;
static int usb_fifos_used = 0;
static int usb_fifo_start = PERIODIC_TX_FIFO_STARTADDR;
static int usb_global_in_nak = 0;
static int usb_global_out_nak = 0;

USBEPRegisters* InEPRegs;
USBEPRegisters* OutEPRegs;

static USBDeviceDescriptor deviceDescriptor;
static USBDeviceQualifierDescriptor deviceQualifierDescriptor;

static uint8_t numStringDescriptors;
static USBStringDescriptor** stringDescriptors;
static USBFirstStringDescriptor* firstStringDescriptor;

static USBConfiguration* configurations;

static uint8_t* controlSendBuffer = NULL;
static uint8_t* controlRecvBuffer = NULL;

static USBEnumerateHandler enumerateHandler = NULL;
static USBStartHandler startHandler = NULL;
static USBSetupHandler setupHandler = NULL;

static void usbIRQHandler(uint32_t token);

static void initializeDescriptors();

static uint8_t addConfiguration(uint8_t bConfigurationValue, uint8_t iConfiguration, uint8_t selfPowered, uint8_t remoteWakeup, uint16_t maxPower);

static void endConfiguration(USBConfiguration* configuration);

USBInterface* usb_add_interface(USBConfiguration* configuration, uint8_t interface, uint8_t bAlternateSetting, uint8_t bInterfaceClass, uint8_t bInterfaceSubClass, uint8_t bInterfaceProtocol, uint8_t iInterface);

static uint8_t addEndpointDescriptor(USBInterface* interface, uint8_t endpoint, USBDirection direction, USBTransferType transferType, USBSynchronisationType syncType, USBUsageType usageType, uint16_t wMaxPacketSize, uint8_t bInterval);

static void releaseConfigurations();

uint8_t usb_add_string_descriptor(const char* descriptorString);
static void releaseStringDescriptors();
static uint16_t packetsizeFromSpeed(uint8_t speed_id);

void usb_send_control(void* buffer, int bufferLen);
void usb_receive_control(void* buffer, int bufferLen);

static int resetUSB();
static void getEndpointInterruptStatuses();
static void callEndpointHandlers();
static uint32_t getConfigurationTree(int i, uint8_t speed_id, void* buffer);
static void setConfiguration(int i);

static void handleTxInterrupts(int endpoint);
static void handleRxInterrupts(int endpoint);

static void clearEPMessages(int _ep);

USBState usb_state()
{
	return gUsbState;
}

int usb_setup() {
	int i;

	// This is not relevant to the hardware,
	// and usb_setup is called when setting up a new
	// USB protocol. So we should reset the EP
	// handlers here! -- Ricky26
	memset(endpoint_handlers, 0, sizeof(endpoint_handlers));
	startHandler = NULL;
	enumerateHandler = NULL;
	setupHandler = NULL;

	if(usb_inited) {
		return 0;
	}

#ifndef CONFIG_IPOD2G
	// Power on hardware
	power_ctrl(POWER_USB, ON);
	udelay(USB_START_DELAYUS);
#endif

	InEPRegs = (USBEPRegisters*)(USB + USB_INREGS);
	OutEPRegs = (USBEPRegisters*)(USB + USB_OUTREGS);

	change_state(USBStart);

	// Initialize our data structures
	memset(usb_message_queue, 0, sizeof(usb_message_queue));

	// Set up the hardware
	clock_gate_switch(USB_OTGCLOCKGATE, ON);
	clock_gate_switch(USB_PHYCLOCKGATE, ON);
	clock_gate_switch(EDRAM_CLOCKGATE, ON);

	// Generate a soft disconnect on host
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SFTDISCONNECT);
	udelay(USB_SFTDISCONNECT_DELAYUS);

	// power on OTG
	SET_REG(USB + USB_ONOFF, GET_REG(USB + USB_ONOFF) & (~USB_ONOFF_OFF));
	udelay(USB_ONOFFSTART_DELAYUS);

#if defined(USB_SETUP_PHY)
	// power on PHY
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_POWERON);
	udelay(USB_PHYPWRPOWERON_DELAYUS);

#if defined(CONFIG_IPOD2G)
	// select clock
	uint32_t phyClockBits;
	switch (clock_get_frequency(FrequencyBaseUsbPhy))
	{
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
	SET_REG(USB_PHY + OPHYCLK, (GET_REG(USB_PHY + OPHYCLK) & OPHYCLK_CLKSEL_MASK) | phyClockBits);
#else
	// select clock
	SET_REG(USB_PHY + OPHYCLK, (GET_REG(USB_PHY + OPHYCLK) & ~OPHYCLK_CLKSEL_MASK) | OPHYCLK_CLKSEL_48MHZ);
#endif

	// reset phy
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) | ORSTCON_PHYSWRESET);
	udelay(USB_RESET2_DELAYUS);
	SET_REG(USB_PHY + ORSTCON, GET_REG(USB_PHY + ORSTCON) & (~ORSTCON_PHYSWRESET));
	udelay(USB_RESET_DELAYUS);
#endif

	SET_REG(USB + GRSTCTL, GRSTCTL_CORESOFTRESET);

	// wait until reset takes
	while((GET_REG(USB + GRSTCTL) & GRSTCTL_CORESOFTRESET) == GRSTCTL_CORESOFTRESET);

	// wait until reset completes
	while((GET_REG(USB + GRSTCTL) & ~GRSTCTL_AHBIDLE) != 0);

	udelay(USB_RESETWAITFINISH_DELAYUS);

	if(GET_REG(USB+GHWCFG4) & GHWCFG4_DED_FIFO_EN)
		usb_fifo_mode = FIFODedicated;
	else
		usb_fifo_mode = FIFOShared;
	bufferPrintf("USB: FIFO Mode %d.\n", usb_fifo_mode);

	int hwcfg2 = GET_REG(USB+GHWCFG2);
	int num_eps = (hwcfg2 >> GHWCFG2_NUM_ENDPOINTS_SHIFT) & GHWCFG2_NUM_ENDPOINTS_MASK;
	bufferPrintf("USB: %d endpoints.\n", num_eps);
	if(num_eps > USB_NUM_ENDPOINTS)
	{
		bufferPrintf("USB: Only using %d EPs, as that is all OIB was compiled to use.\n", USB_NUM_ENDPOINTS);
		num_eps = USB_NUM_ENDPOINTS;
	}

	usb_num_endpoints = num_eps;

	// flag all interrupts as positive, maybe to disable them

	// Set 7th EP? This is what iBoot does
	InEPRegs[USB_NUM_ENDPOINTS].interrupt = USB_EPINT_INEPNakEff | USB_EPINT_INTknEPMis | USB_EPINT_INTknTXFEmp
		| USB_EPINT_TimeOUT | USB_EPINT_AHBErr | USB_EPINT_EPDisbld | USB_EPINT_XferCompl;
	OutEPRegs[USB_NUM_ENDPOINTS].interrupt = USB_EPINT_OUTTknEPDis
		| USB_EPINT_SetUp | USB_EPINT_AHBErr | USB_EPINT_EPDisbld | USB_EPINT_XferCompl;

	for(i = 0; i < USB_NUM_ENDPOINTS; i++) {
		InEPRegs[i].interrupt = USB_EPINT_INEPNakEff | USB_EPINT_INTknEPMis | USB_EPINT_INTknTXFEmp
			| USB_EPINT_TimeOUT | USB_EPINT_AHBErr | USB_EPINT_EPDisbld | USB_EPINT_XferCompl;
		OutEPRegs[i].interrupt = USB_EPINT_OUTTknEPDis
			| USB_EPINT_SetUp | USB_EPINT_AHBErr | USB_EPINT_EPDisbld | USB_EPINT_XferCompl;

		InEPRegs[i].control = 0;
		OutEPRegs[i].control = 0;
	}

	// disable all interrupts until endpoint descriptors and configuration structures have been setup
	SET_REG(USB + GINTMSK, GINTMSK_NONE);
	SET_REG(USB + DIEPMSK, USB_EPINT_NONE);
	SET_REG(USB + DOEPMSK, USB_EPINT_NONE);

	// allow host to reconnect
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) & (~DCTL_SFTDISCONNECT));
	udelay(USB_SFTCONNECT_DELAYUS);

	bufferPrintf("USB: Hardware Configuration\n"
		"    HWCFG1 = 0x%08x\n"
		"    HWCFG2 = 0x%08x\n"
		"    HWCFG3 = 0x%08x\n"
		"    HWCFG4 = 0x%08x\n",
		GET_REG(USB+GHWCFG1),
		GET_REG(USB+GHWCFG2),
		GET_REG(USB+GHWCFG3),
		GET_REG(USB+GHWCFG4));

	interrupt_install(USB_INTERRUPT, usbIRQHandler, 0);

	usb_inited = TRUE;

	return 0;
}

int usb_start(USBEnumerateHandler hEnumerate, USBStartHandler hStart) {
	enumerateHandler = hEnumerate;
	startHandler = hStart;

	initializeDescriptors();

	if(controlSendBuffer == NULL)
		controlSendBuffer = memalign(DMA_ALIGN, CONTROL_SEND_BUFFER_LEN);

	if(controlRecvBuffer == NULL)
		controlRecvBuffer = memalign(DMA_ALIGN, CONTROL_RECV_BUFFER_LEN);

	SET_REG(USB + GAHBCFG, GAHBCFG_DMAEN | GAHBCFG_BSTLEN_INCR8 | GAHBCFG_MASKINT);
	SET_REG(USB + GUSBCFG, GUSBCFG_PHYIF16BIT | GUSBCFG_SRPENABLE | GUSBCFG_HNPENABLE | ((USB_TURNAROUND & GUSBCFG_TURNAROUND_MASK) << GUSBCFG_TURNAROUND_SHIFT));

	SET_REG(USB + DCFG, DCFG_HISPEED | DCFG_NZSTSOUTHSHK); // some random setting. See specs
	SET_REG(USB + DCFG, GET_REG(USB + DCFG) & ~(DCFG_DEVICEADDRMSK));

	InEPRegs[0].control = USB_EPCON_ACTIVE;
	OutEPRegs[0].control = USB_EPCON_ACTIVE;

	SET_REG(USB + GRXFSIZ, RX_FIFO_DEPTH);
	SET_REG(USB + GNPTXFSIZ, (TX_FIFO_DEPTH << FIFO_DEPTH_SHIFT) | TX_FIFO_STARTADDR);

	int i;
	for(i = 0; i < USB_NUM_ENDPOINTS; i++) {
		InEPRegs[i].interrupt = USB_EPINT_INEPNakEff | USB_EPINT_INTknEPMis | USB_EPINT_INTknTXFEmp
			| USB_EPINT_TimeOUT | USB_EPINT_AHBErr | USB_EPINT_EPDisbld | USB_EPINT_XferCompl;
		OutEPRegs[i].interrupt = USB_EPINT_OUTTknEPDis
			| USB_EPINT_SetUp | USB_EPINT_AHBErr | USB_EPINT_EPDisbld | USB_EPINT_XferCompl;
	
		InEPRegs[i].control = (InEPRegs[i].control & ~(USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT));
	}

	SET_REG(USB + GINTMSK, GINTMSK_OTG | GINTMSK_SUSPEND | GINTMSK_RESET | GINTMSK_ENUMDONE | GINTMSK_INEP | GINTMSK_OEP | GINTMSK_DISCONNECT | GINTMSK_EPMIS);
	SET_REG(USB + DAINTMSK, DAINTMSK_ALL);

	SET_REG(USB + DOEPMSK, USB_EPINT_XferCompl | USB_EPINT_SetUp | USB_EPINT_Back2BackSetup);
	SET_REG(USB + DIEPMSK, USB_EPINT_XferCompl | USB_EPINT_AHBErr | USB_EPINT_TimeOUT);

	InEPRegs[0].interrupt = USB_EPINT_ALL;
	OutEPRegs[0].interrupt = USB_EPINT_ALL;

	SET_REG(USB + DCTL, DCTL_PROGRAMDONE + DCTL_CGOUTNAK + DCTL_CGNPINNAK); // This clears active EP count
	udelay(USB_PROGRAMDONE_DELAYUS);
	SET_REG(USB + GOTGCTL, GET_REG(USB + GOTGCTL) | GOTGCTL_SESSIONREQUEST);

	usb_enable_endpoint(0, USBBiDir, USBControl, 0x80);
	usb_receive_control(controlRecvBuffer, sizeof(USBSetupPacket));

	change_state(USBPowered);

	interrupt_enable(USB_INTERRUPT);

	bufferPrintf("USB: EP Directions\n");
	int guah = GET_REG(USB+GHWCFG1);
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
	{
		USBDirection dir = (USBDirection)(2-(guah & 0x3));
		guah >>= 2;

		switch(dir)
		{
			case USBIn:
				bufferPrintf("%d: IN\n", i);
				break;

			case USBOut:
				bufferPrintf("%d: OUT\n", i);
				break;

			case USBBiDir:
				bufferPrintf("%d: BI\n", i);
				break;
		}
	}

	return 0;
}

static void usb_set_global_out_nak()
{
	usb_global_out_nak++;
	
	if(usb_global_out_nak > 1)
		return;

	//bufferPrintf("USB: SGOUTNAK\n");

	SET_REG(USB + GINTSTS, GINTMSK_GOUTNAKEFF);
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SGOUTNAK);
	while ((GET_REG(USB + GINTSTS) & GINTMSK_GOUTNAKEFF) != GINTMSK_GOUTNAKEFF);
	SET_REG(USB + GINTSTS, GINTMSK_GOUTNAKEFF);
}

static void usb_clear_global_out_nak()
{
	usb_global_out_nak--;

	if(usb_global_out_nak <= 0)
	{
		usb_global_out_nak = 0;

		//bufferPrintf("USB: CGOUTNAK\n");

		SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_CGOUTNAK);
	}
}

static void usb_set_global_in_nak()
{
	usb_global_in_nak++;

	if(usb_global_in_nak > 1)
		return;

	//bufferPrintf("USB: SGNPINNAK\n");

	SET_REG(USB + GINTSTS, GINTMSK_GINNAKEFF);
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SGNPINNAK);
	while ((GET_REG(USB + GINTSTS) & GINTMSK_GINNAKEFF) != GINTMSK_GINNAKEFF);
	SET_REG(USB + GINTSTS, GINTMSK_GINNAKEFF);
}

static void usb_clear_global_in_nak()
{
	usb_global_in_nak--;

	if(usb_global_in_nak <= 0)
	{
		usb_global_in_nak = 0;

		//bufferPrintf("USB: CGNPINNAK\n");

		SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_CGNPINNAK);
	}
}

static int usb_epmis()
{
	return (GET_REG(USB+DCFG) >> DCFG_ACTIVE_EP_COUNT_SHIFT) & DCFG_ACTIVE_EP_COUNT_MASK;
}

static void usb_set_epmis(int _amt)
{
	SET_REG(USB+DCFG, (GET_REG(USB+DCFG) &~ (DCFG_ACTIVE_EP_COUNT_MASK << DCFG_ACTIVE_EP_COUNT_SHIFT)) | (_amt << DCFG_ACTIVE_EP_COUNT_SHIFT));
}

static void usb_mod_epmis(int _amt)
{
	usb_set_epmis(usb_epmis() + _amt);
}

static void usb_flush_fifo(int _fifo)
{
	bufferPrintf("USB: Flushing %d.\n", _fifo);

	// Wait until FIFOs aren't being accessed
	while((GET_REG(USB + GRSTCTL) & GRSTCTL_AHBIDLE) == 0);

	// Flush FIFOs
	SET_REG(USB+GRSTCTL, ((_fifo & GRSTCTL_TXFFNUM_MASK) << GRSTCTL_TXFFNUM_SHIFT) | GRSTCTL_TXFFLUSH);

	// Wait for FIFOs to be flushed
	while(GET_REG(USB+GRSTCTL) & GRSTCTL_TXFFLUSH);
}

static void usb_flush_all_fifos()
{
	usb_flush_fifo(0x10);
}

static void usb_add_ep_to_queue(int _ep)
{
	if(usb_ep_queue_first == 0xFF)
	{
		usb_ep_queue_first = _ep;
		usb_ep_queue_last = _ep;

		InEPRegs[_ep].control = (InEPRegs[_ep].control &~ (USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT))
			| ((_ep & USB_EPCON_NEXTEP_MASK) << USB_EPCON_NEXTEP_SHIFT);
	}
	else
	{
		InEPRegs[_ep].control = (InEPRegs[_ep].control &~ (USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT))
				| ((usb_ep_queue_last & USB_EPCON_NEXTEP_MASK) << USB_EPCON_NEXTEP_SHIFT);
		InEPRegs[usb_ep_queue_first].control = (InEPRegs[usb_ep_queue_first].control &~ (USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT))
				| ((_ep & USB_EPCON_NEXTEP_MASK) << USB_EPCON_NEXTEP_SHIFT);

		usb_ep_queue_last = _ep;
	}

	usb_mod_epmis(1);	

	int i;
	bufferPrintf("USB: loop = ");
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
		bufferPrintf("%d ", (InEPRegs[i].control >> USB_EPCON_NEXTEP_SHIFT) & USB_EPCON_NEXTEP_MASK);
	bufferPrintf("\n");
}

static void usb_claim_fifo(int _ep)
{
	int i = 1;
	for(; i < USB_NUM_FIFOS; i++)
	{
		if((usb_fifos_used & (1 << (i-1))) == 0)
		{
			// We found an unused fifo.
			usb_fifos_used |= (1 << (i-1)); // Claim it
			int fstart = usb_fifo_start;
			usb_fifo_start += PERIODIC_TX_FIFO_DEPTH;

			SET_REG(USB+DIEPTXF(i), fstart | (PERIODIC_TX_FIFO_DEPTH << FIFO_DEPTH_SHIFT));

			InEPRegs[_ep].control = (InEPRegs[_ep].control & ~(USB_EPCON_TXFNUM_MASK << USB_EPCON_TXFNUM_SHIFT))
									| ((i & USB_EPCON_TXFNUM_MASK) << USB_EPCON_TXFNUM_SHIFT);
		}
	}
}

void usb_enable_endpoint(int _ep, USBDirection _dir, USBTransferType _type, int _mps)
{
	if(_ep == USB_CONTROLEP)
	{
		InEPRegs[_ep].control = USB_EPCON_ACTIVE; // MPS = default, type = control, nextep = 0
		OutEPRegs[_ep].control = USB_EPCON_ACTIVE; // MPS = default, type = control, nextep = 0

		if(usb_fifo_mode == FIFODedicated)
			usb_claim_fifo(_ep);
		else if(usb_fifo_mode == FIFOShared)
			usb_add_ep_to_queue(_ep);
		
		SET_REG(USB + DAINTMSK, GET_REG(USB + DAINTMSK) | ((1 << USB_CONTROLEP) << DAINTMSK_OUT_SHIFT) | ((1 << USB_CONTROLEP) << DAINTMSK_IN_SHIFT));
	}
	else
	{
		if(_dir != USBOut)
		{
			// IN EP

			InEPRegs[_ep].control = (_mps & USB_EPCON_MPS_MASK)
				| ((_type & USB_EPCON_TYPE_MASK) << USB_EPCON_TYPE_SHIFT);

			if(usb_fifo_mode == FIFODedicated)
			{
				// TODO: Allocate FIFO
			}
			else if(usb_fifo_mode == FIFOShared)
				usb_add_ep_to_queue(_ep);

			InEPRegs[_ep].control |= USB_EPCON_ACTIVE | USB_EPCON_SETD0PID | USB_EPCON_SETNAK;

			SET_REG(USB+DAINTMSK, GET_REG(USB+DAINTMSK) | (1 << (DAINTMSK_IN_SHIFT + _ep)));
		}

		if(_dir != USBIn)
		{
			// OUT EP
			
			OutEPRegs[_ep].control = (_mps & USB_EPCON_MPS_MASK)
				| ((_type & USB_EPCON_TYPE_MASK) << USB_EPCON_TYPE_SHIFT)
				| USB_EPCON_ACTIVE | USB_EPCON_SETD0PID | USB_EPCON_SETNAK;

			SET_REG(USB+DAINTMSK, GET_REG(USB+DAINTMSK) | (1 << (DAINTMSK_OUT_SHIFT+_ep)));
		}
	}
}

static void usb_remove_ep_from_queue(int _ep)
{
	if(_ep == usb_ep_queue_first)
	{
		if(_ep == usb_ep_queue_last)
		{
			// We're the last living EP!
			usb_ep_queue_first = 0xFF;
			usb_ep_queue_last = 0xFF;
		}
		else
		{
			int i;
			for(i = 0; i < USB_NUM_ENDPOINTS; i++)
			{
				int tn = (InEPRegs[i].control >> USB_EPCON_NEXTEP_SHIFT) & USB_EPCON_NEXTEP_MASK;
				if(tn == _ep)
					break;
			}

			if(i < USB_NUM_ENDPOINTS)
			{
				usb_ep_queue_first = i;
				InEPRegs[i].control = (InEPRegs[i].control &~ (USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT))
					| ((usb_ep_queue_last & USB_EPCON_NEXTEP_MASK) << USB_EPCON_NEXTEP_SHIFT);
			}
			else
				usb_ep_queue_first = 0xFF;
		}
	}
	else if(_ep == usb_ep_queue_last)
	{
			int next = (InEPRegs[_ep].control >> USB_EPCON_NEXTEP_SHIFT) & USB_EPCON_NEXTEP_MASK;
			InEPRegs[usb_ep_queue_first].control = (InEPRegs[usb_ep_queue_first].control &~ (USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT))
					| ((next & USB_EPCON_NEXTEP_MASK) << USB_EPCON_NEXTEP_SHIFT);
			usb_ep_queue_last = next;
	}
	else
	{
		int next = (InEPRegs[_ep].control >> USB_EPCON_NEXTEP_SHIFT) & USB_EPCON_NEXTEP_MASK;

		int i;
		for(i = 0; i < USB_NUM_ENDPOINTS; i++)
		{
				int tn = (InEPRegs[i].control >> USB_EPCON_NEXTEP_SHIFT) & USB_EPCON_NEXTEP_MASK;
				if(tn == _ep)
				{
					InEPRegs[i].control = (InEPRegs[i].control &~ (USB_EPCON_NEXTEP_MASK << USB_EPCON_NEXTEP_SHIFT))
						| ((next & USB_EPCON_NEXTEP_MASK) << USB_EPCON_NEXTEP_SHIFT);
				}
		}
	}

	usb_mod_epmis(-1);

	int i;
	bufferPrintf("USB: loop = ");
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
		bufferPrintf("%d ", (InEPRegs[i].control >> USB_EPCON_NEXTEP_SHIFT) & USB_EPCON_NEXTEP_MASK);
	bufferPrintf("\n");
}

static void usb_release_fifo(int _ep)
{
	int fifo = (InEPRegs[_ep].control >> USB_EPCON_TXFNUM_SHIFT) & USB_EPCON_TXFNUM_MASK;
	if(fifo > 0)
	{
		usb_fifos_used &= ~(1 << (fifo-1));
		InEPRegs[_ep].control &= ~(USB_EPCON_TXFNUM_MASK << USB_EPCON_TXFNUM_SHIFT);
	}
}

void usb_disable_endpoint(int _ep)
{
	clearEPMessages(_ep);

	if(InEPRegs[_ep].control & USB_EPCON_ENABLE) // Is IN EP
	{
		bufferPrintf("USB: Disabling IN EP %d.\n", _ep);
		
		usb_set_global_in_nak();

		int fnum = (InEPRegs[_ep].control >> USB_EPCON_TXFNUM_SHIFT) & USB_EPCON_TXFNUM_MASK;

		if(usb_fifo_mode == FIFODedicated)
			usb_release_fifo(_ep);
		else if(usb_fifo_mode == FIFOShared)
			usb_remove_ep_from_queue(_ep);

		InEPRegs[_ep].control |= USB_EPCON_SETNAK;
		while(!(InEPRegs[_ep].interrupt & USB_EPINT_INEPNakEff));
		InEPRegs[_ep].control |= USB_EPCON_DISABLE;
		while(!(InEPRegs[_ep].interrupt & USB_EPINT_EPDisbld));
		InEPRegs[_ep].interrupt = InEPRegs[_ep].interrupt;

		InEPRegs[_ep].control = USB_EPCON_SETNAK;

		usb_flush_fifo(fnum);

		usb_clear_global_in_nak();
	}

	if(OutEPRegs[_ep].control & USB_EPCON_ENABLE) // Is OUT EP
	{
		bufferPrintf("USB: Disabling OUT EP %d.\n", _ep);

		usb_set_global_out_nak();

		OutEPRegs[_ep].control = USB_EPCON_DISABLE | USB_EPCON_SETNAK;
		//while(!(OutEPRegs[_ep].interrupt & USB_EPINT_EPDisbld));
		//OutEPRegs[_ep].interrupt = OutEPRegs[_ep].interrupt;

		usb_clear_global_out_nak();
	}
}

static void usb_cancel_endpoint(int _ep)
{
	if(InEPRegs[_ep].control & USB_EPCON_ENABLE) // Is IN EP
	{
		bufferPrintf("USB: Disabling IN EP %d.\n", _ep);
		
		usb_set_global_in_nak();

		int fnum = (InEPRegs[_ep].control >> USB_EPCON_TXFNUM_SHIFT) & USB_EPCON_TXFNUM_MASK;

		InEPRegs[_ep].control |= USB_EPCON_SETNAK;
		while(!(InEPRegs[_ep].interrupt & USB_EPINT_INEPNakEff));
		InEPRegs[_ep].control |= USB_EPCON_DISABLE;
		while(!(InEPRegs[_ep].interrupt & USB_EPINT_EPDisbld));
		InEPRegs[_ep].interrupt = InEPRegs[_ep].interrupt;

		InEPRegs[_ep].control &= ~USB_EPCON_DISABLE;

		usb_flush_fifo(fnum);

		usb_clear_global_in_nak();
	}

	if(OutEPRegs[_ep].control & USB_EPCON_ENABLE) // Is OUT EP
	{
		bufferPrintf("USB: Disabling OUT EP %d.\n", _ep);

		usb_set_global_out_nak();

		OutEPRegs[_ep].control = /*USB_EPCON_DISABLE |*/ USB_EPCON_SETNAK;
		//while(!(OutEPRegs[_ep].interrupt & USB_EPINT_EPDisbld));
		//OutEPRegs[_ep].interrupt = OutEPRegs[_ep].interrupt;

		usb_clear_global_out_nak();
	}
}

static void usb_disable_all_endpoints()
{
	int i;
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
	{
		if((InEPRegs[i].control & USB_EPCON_ENABLE) | (OutEPRegs[i].control & USB_EPCON_ENABLE))
			usb_disable_endpoint(i);
	}
}

static void usb_disable_in_endpoints()
{
	int i;
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
	{
		USBMessageQueue *q = usb_message_queue[i];

		if(!q)
			continue;

		if(q->dir != USBIn)
			continue;

		usb_cancel_endpoint(i);
	}
}

static void usb_txrx(int endpoint, USBDirection direction, void* buffer, int bufferLen)
{
	int packetLength;
	int daint;
	USBEPRegisters *regs;

	// Setup register blocks, and the interrupt mask. -- Ricky26
	CleanAndInvalidateCPUDataCache();
	if(direction == USBOut)
	{
		daint = 1 << (DAINTMSK_OUT_SHIFT + endpoint);
		regs = &OutEPRegs[endpoint];
	}
	else
	{
		daint = 1 << (DAINTMSK_IN_SHIFT + endpoint);
		regs = &InEPRegs[endpoint];

		if(GNPTXFSTS_GET_TXQSPCAVAIL(GET_REG(USB + GNPTXFSTS)) == 0) {
			// no space available
			bufferPrintf("USB: FIFO full. Can't send!\n");
			return;
		}
	}

	SET_REG(USB+DAINTMSK, GET_REG(USB+DAINTMSK) | daint);
	regs->dmaAddress = buffer;
	packetLength = regs->control & USB_EPCON_MPS_MASK;

	// Setup TXSIZ
	if(endpoint == USB_CONTROLEP)
	{
		if(direction == USBOut)
		{
			regs->transferSize = ((USB_SETUP_PACKETS_AT_A_TIME & DOEPTSIZ0_SUPCNT_MASK) << DOEPTSIZ0_SUPCNT_SHIFT)
				| ((USB_SETUP_PACKETS_AT_A_TIME & DOEPTSIZ0_PKTCNT_MASK) << DEPTSIZ_PKTCNT_SHIFT) | (bufferLen & DEPTSIZ0_XFERSIZ_MASK);
		}
		else
			regs->transferSize = (1 << DEPTSIZ_PKTCNT_SHIFT) | (bufferLen & DEPTSIZ0_XFERSIZ_MASK) | ((USB_MULTICOUNT & DIEPTSIZ_MC_MASK) << DIEPTSIZ_MC_SHIFT);
	}
	else
	{
		// divide our buffer into packets. Apple uses fancy bitwise arithmetic while we call huge libgcc integer arithmetic functions
		// for the sake of code readability. Will this matter?
		int packetCount = bufferLen / packetLength;
		if((bufferLen % packetLength) != 0 || packetLength == 0)
			++packetCount;

		regs->transferSize = ((packetCount & DEPTSIZ_PKTCNT_MASK) << DEPTSIZ_PKTCNT_SHIFT) | (bufferLen & DEPTSIZ_XFERSIZ_MASK);
		
		if(direction == USBIn)
			regs->transferSize |= ((USB_MULTICOUNT & DIEPTSIZ_MC_MASK) << DIEPTSIZ_MC_SHIFT);
	}

	regs->interrupt = 0xffffffff;

	regs->control |= USB_EPCON_ENABLE | USB_EPCON_CLEARNAK; // Start the transfer
}

static int resetUSB()
{
	SET_REG(USB + DCFG, GET_REG(USB + DCFG) & ~DCFG_DEVICEADDRMSK);
	SET_REG(USB+DCFG, (GET_REG(USB+DCFG) &~ (DCFG_ACTIVE_EP_COUNT_MASK << DCFG_ACTIVE_EP_COUNT_SHIFT)));

	usb_disable_all_endpoints();
	usb_flush_all_fifos();

	usb_fifo_start = PERIODIC_TX_FIFO_STARTADDR;

	int i;
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
		clearEPMessages(i);

	usb_global_in_nak = 0;
	usb_global_out_nak = 0;
	usb_clear_global_in_nak();
	usb_clear_global_out_nak();

	bufferPrintf("USB: EP Directions\n");
	int guah = GET_REG(USB+GHWCFG1);
	for(i = 0; i < USB_NUM_ENDPOINTS; i++)
	{
		USBDirection dir = (USBDirection)(2 - (guah & 0x3));
		guah >>= 2;

		char *sdir = "?";
		switch(dir)
		{
			case USBIn:
				sdir = "in";
				break;

			case USBOut:
				sdir = "out";
				break;

			case USBBiDir:
				sdir = "bidir";
				break;
		}
		
		bufferPrintf("EP%d: %s (0x%08x/0x%08x)\n", i, sdir, InEPRegs[i].control, OutEPRegs[i].control);
	}

	SET_REG(USB + DOEPMSK, USB_EPINT_XferCompl | USB_EPINT_SetUp | USB_EPINT_Back2BackSetup);
	SET_REG(USB + DIEPMSK, USB_EPINT_XferCompl | USB_EPINT_AHBErr | USB_EPINT_TimeOUT);

	usb_enable_endpoint(0, USBBiDir, USBControl, 0x80);
	usb_receive_control(controlRecvBuffer, sizeof(USBSetupPacket));
	return 0;
}

static void getEndpointInterruptStatuses() {
	// To not mess up the interrupt controller, we can only read the interrupt status once per interrupt, so we need to cache them here
	int endpoint;
	for(endpoint = 0; endpoint < USB_NUM_ENDPOINTS; endpoint++)
	{
		inInterruptStatus[endpoint] = InEPRegs[endpoint].interrupt;
		outInterruptStatus[endpoint] = OutEPRegs[endpoint].interrupt;
	}
}

static int continueMessageQueue(int _ep)
{
	USBMessageQueue *q = usb_message_queue[_ep];
	if(q != NULL)
	{
		//if(q->dir == USBIn) //_ep != 0)
		//	bufferPrintf("USB: txrx 0x%08x, %d, %d, %d, %d\n", q, _ep, q->dir, q->data, q->dataLen);
		
		usb_txrx(_ep, q->dir, q->data, q->dataLen);
		return 1;
	}

	return 0;
}

static int clearMessage(int _ep)
{
	USBMessageQueue *q;
   
	EnterCriticalSection();
	q = usb_message_queue[_ep];
	if(q != NULL)
		usb_message_queue[_ep] = q->next;
	LeaveCriticalSection();

	if(q == NULL)
		return 0;

	free(q);

	return 1;
}

static void clearEPMessages(int _ep)
{
	while(clearMessage(_ep));
}

static int advanceMessageQueue(int _ep)
{
	//if(_ep > 0)
	//	bufferPrintf("USB: advance message queue %d.\n", _ep);

	if(clearMessage(_ep))
		return continueMessageQueue(_ep);

	return 0;
}

static void queueMessage(int _ep, USBDirection _dir, char *_data, size_t _dataLen)
{
	USBMessageQueue *t;
	USBMessageQueue *q;

	//bufferPrintf("USB: q %d %d %d %d %d\n", _ep, _dir, _type, _data, _dataLen);
		
	q = malloc(sizeof(USBMessageQueue));
	q->next = NULL;
	q->dir = _dir;
	q->data = _data;
	q->dataLen = _dataLen;

	EnterCriticalSection();
	t = usb_message_queue[_ep];
	if(t == NULL)
	{
		usb_message_queue[_ep] = q;
		LeaveCriticalSection();

		continueMessageQueue(_ep);
	}
	else
	{
		while(t->next != NULL)
			t = t->next;

		t->next = q;

		LeaveCriticalSection();
	}
}

static int isSetupPhaseDone() {
	uint32_t status = GET_REG(USB + DAINTMSK);
	int isDone = FALSE;

	if((status & ((1 << USB_CONTROLEP) << DAINTMSK_OUT_SHIFT)) == ((1 << USB_CONTROLEP) << DAINTMSK_OUT_SHIFT)) {
		if((outInterruptStatus[USB_CONTROLEP] & USB_EPINT_SetUp) == USB_EPINT_SetUp) {
			isDone = TRUE;

			//bufferPrintf("Setup\n");

			advanceMessageQueue(USB_CONTROLEP);
		}
	}

	// clear interrupt
	OutEPRegs[USB_CONTROLEP].interrupt = USB_EPINT_SetUp;

	return isDone;
}

static void callEndpointHandlers() {
	uint32_t status = GET_REG(USB + DAINTMSK);

	int endpoint;
	int in = 1 << DAINTMSK_IN_SHIFT;
	int out = 1 << DAINTMSK_OUT_SHIFT;
	for(endpoint = 0; endpoint < USB_NUM_ENDPOINTS; endpoint++)
	{
		if(status & out)
		{
			if((outInterruptStatus[endpoint] & USB_EPINT_XferCompl) == USB_EPINT_XferCompl) {
				if(endpoint_handlers[endpoint].out.handler != NULL) {
					// Calculate actual amount sent
					USBMessageQueue *q = usb_message_queue[endpoint];
					uint32_t left = OutEPRegs[endpoint].transferSize & DEPTSIZ_XFERSIZ_MASK;
					uint32_t sent = q->dataLen;
					if(left > sent)
					{
						bufferPrintf("USB: Bad things! %d (0x%08x), %d (0x%08x)!\n", left, left, sent, sent);
					}
					
					//if(endpoint != 0)
					//	bufferPrintf("USB: xc 0x%08x, %d, %d, %d, %d, %d\n", q, endpoint, q->dir, q->type, q->data, q->dataLen);

					endpoint_handlers[endpoint].out.handler(endpoint_handlers[endpoint].out.token, sent-left);
				}
			}
		}

		if(status & in)
		{
			if((inInterruptStatus[endpoint] & USB_EPINT_XferCompl) == USB_EPINT_XferCompl) {
				if(endpoint_handlers[endpoint].in.handler != NULL) {
					// Calculate actual amount sent
					USBMessageQueue *q = usb_message_queue[endpoint];
					uint32_t left = InEPRegs[endpoint].transferSize & DEPTSIZ_XFERSIZ_MASK;
					uint32_t sent = q->dataLen;
					if(left > sent)
					{
						bufferPrintf("USB: Bad things! %d (0x%08x), %d (0x%08x)!\n", left, left, sent, sent);
					}
					
					//if(endpoint != 0)
					//	bufferPrintf("USB: xc 0x%08x, %d, %d, %d, %d, %d\n", q, endpoint, q->dir, q->type, q->data, q->dataLen);

					endpoint_handlers[endpoint].in.handler(endpoint_handlers[endpoint].in.token, sent-left);
				}
			}
		}

		if(status & out)
			handleRxInterrupts(endpoint);

		if(status & in)
			handleTxInterrupts(endpoint);

		in <<= 1;
		out <<= 1;
	}
}

static void handleTxInterrupts(int endpoint) {
	if(!inInterruptStatus[endpoint]) {
		return;
	}

	//uartPrintf("\tendpoint %d has an TX interrupt\r\n", endpoint);

	// clear pending interrupts
	/*if((inInterruptStatus[endpoint] & USB_EPINT_INEPNakEff) == USB_EPINT_INEPNakEff) {
		InEPRegs[endpoint].interrupt = USB_EPINT_INEPNakEff;
		//uartPrintf("\t\tUSB_EPINT_INEPNakEff\r\n");
		bufferPrintf("in ep nak eff %d\n", endpoint);
	}*/

	/*if((inInterruptStatus[endpoint] & USB_EPINT_INTknEPMis) == USB_EPINT_INTknEPMis) {
		InEPRegs[endpoint].interrupt = USB_EPINT_INTknEPMis;
		//uartPrintf("\t\tUSB_EPINT_INTknEPMis\r\n");
		bufferPrintf("USB: EP Token Mismatch! (Got %d, expected %d.)\n", endpoint, usb_currently_sending);

		// clear the corresponding core interrupt
		SET_REG(USB + GINTSTS, GET_REG(USB + GINTSTS) | GINTMSK_EPMIS);
	}*/

	if((inInterruptStatus[endpoint] & USB_EPINT_INTknTXFEmp) == USB_EPINT_INTknTXFEmp) {
		InEPRegs[endpoint].interrupt = USB_EPINT_INTknTXFEmp;
		//uartPrintf("\t\tUSB_EPINT_INTknTXFEmp\r\n");
		//bufferPrintf("in tkn tx fifo empty %d\n", endpoint);
	}

	if((inInterruptStatus[endpoint] & USB_EPINT_TimeOUT) == USB_EPINT_TimeOUT) {
		InEPRegs[endpoint].interrupt = USB_EPINT_TimeOUT;
		//uartPrintf("\t\tUSB_EPINT_TimeOUT\r\n");
		bufferPrintf("in timeout %d\n", endpoint);

		advanceMessageQueue(endpoint);
	}

	if((inInterruptStatus[endpoint] & USB_EPINT_AHBErr) == USB_EPINT_AHBErr) {
		InEPRegs[endpoint].interrupt = USB_EPINT_AHBErr;
		//uartPrintf("\t\tUSB_EPINT_AHBErr\r\n");
		bufferPrintf("in ahberr\n");
	}

	/*if((inInterruptStatus[endpoint] & USB_EPINT_EPDisbld) == USB_EPINT_EPDisbld) {
		InEPRegs[endpoint].interrupt = USB_EPINT_EPDisbld;
		//uartPrintf("\t\tUSB_EPINT_EPDisbldr\n");
		bufferPrintf("in ep disabled %d\n", endpoint);
	}*/

	if((inInterruptStatus[endpoint] & USB_EPINT_XferCompl) == USB_EPINT_XferCompl) {
		USBMessageQueue *q = usb_message_queue[endpoint];
		InEPRegs[endpoint].interrupt = USB_EPINT_XferCompl;

		//uartPrintf("\t\tUSB_EPINT_XferCompl\n");
		//bufferPrintf("in xfercompl %d\n", endpoint);
		
		// Flush token queue, as this EP is clearly functioning fine.
		//SET_REG(USB+GRSTCTL, GET_REG(USB+GRSTCTL) | GRSTCTL_TKNFLUSH);
		
		if(q && q->dir == USBIn)
			advanceMessageQueue(endpoint);
	}
	
}

static void handleRxInterrupts(int endpoint) {
	if(!outInterruptStatus[endpoint]) {
		return;
	}

	//uartPrintf("\tendpoint %d has an RX interrupt\r\n", endpoint);
		
	if((outInterruptStatus[endpoint] & USB_EPINT_Back2BackSetup) == USB_EPINT_Back2BackSetup) {
		OutEPRegs[endpoint].interrupt = USB_EPINT_Back2BackSetup;
		bufferPrintf("out b2bsetup\n");

		//advanceMessageQueue(endpoint);
	}

	if(outInterruptStatus[endpoint] & USB_EPINT_OUTTknEPDis) {
		OutEPRegs[endpoint].interrupt = USB_EPINT_OUTTknEPDis;
		if(endpoint > 0)
			bufferPrintf("out %d tnk ep dis\n", endpoint);

		//advanceMessageQueue(endpoint);
	}

	if((outInterruptStatus[endpoint] & USB_EPINT_AHBErr) == USB_EPINT_AHBErr) {
		OutEPRegs[endpoint].interrupt = USB_EPINT_AHBErr;
		bufferPrintf("out ahberr\n");
	}

	if((outInterruptStatus[endpoint] & USB_EPINT_EPDisbld) == USB_EPINT_EPDisbld) {
		OutEPRegs[endpoint].interrupt = USB_EPINT_EPDisbld;
		bufferPrintf("out ep disabled\n");
	}

	if((outInterruptStatus[endpoint] & USB_EPINT_XferCompl) == USB_EPINT_XferCompl) {
		USBMessageQueue *q = usb_message_queue[endpoint];
		OutEPRegs[endpoint].interrupt = USB_EPINT_XferCompl;

		//bufferPrintf("out xfercompl %d\n", endpoint);

		if(endpoint == 0)
		{
			// Presumably this is an ACK. Receive another control message.
			usb_receive_control(controlRecvBuffer, sizeof(USBSetupPacket));
		}
		
		if(q && q->dir == USBOut)
			advanceMessageQueue(endpoint);
	}
}

void usb_send_control(void* buffer, int bufferLen) {
	queueMessage(USB_CONTROLEP, USBIn, buffer, bufferLen);
}

void usb_receive_control(void* buffer, int bufferLen) {
	queueMessage(USB_CONTROLEP, USBOut, buffer, bufferLen);
}

static void stallControl(void) {
	InEPRegs[USB_CONTROLEP].control |= USB_EPCON_STALL;
}

void usb_send_bulk(uint8_t endpoint, void* buffer, int bufferLen)
{
	// TODO: Check type? : P -- Ricky26

	queueMessage(endpoint, USBIn, buffer, bufferLen);
}

void usb_send_interrupt(uint8_t endpoint, void* buffer, int bufferLen)
{
	// TODO: Check type? : P -- Ricky26

	queueMessage(endpoint, USBIn, buffer, bufferLen);
}

void usb_receive_bulk(uint8_t endpoint, void* buffer, int bufferLen)
{
	// TODO: Check type? : P -- Ricky26

	queueMessage(endpoint, USBOut, buffer, bufferLen);
}

void usb_receive_interrupt(uint8_t endpoint, void* buffer, int bufferLen)
{
	// TODO: Check type? : P -- Ricky26

	queueMessage(endpoint, USBOut, buffer, bufferLen);
}

static void usbIRQHandler(uint32_t token)
{
	// we need to mask because GINTSTS is set for a particular interrupt even if it's masked in GINTMSK (GINTMSK just prevents an interrupt being generated)
	uint32_t status = GET_REG(USB + GINTSTS) & GET_REG(USB + GINTMSK);
	int process = FALSE;

	//uartPrintf("<begin interrupt: %x>\r\n", status);

	if(status) {
		process = TRUE;
	}

	while(process) {
		if((status & GINTMSK_OTG) == GINTMSK_OTG) {
			// acknowledge OTG interrupt (these bits are all R_SS_WC which means Write Clear, a write of 1 clears the bits)
			SET_REG(USB + GOTGINT, GET_REG(USB + GOTGINT));

			// acknowledge interrupt (this bit is actually RO, but should've been cleared when we cleared GOTGINT. Still, iBoot pokes it as if it was WC, so we will too)
			SET_REG(USB + GINTSTS, GINTMSK_OTG);

			process = TRUE;
		} else {
			// we only care about OTG
			process = FALSE;
		}

		if((status & GINTMSK_RESET) == GINTMSK_RESET) {
			bufferPrintf("USB: reset detected\r\n");
			change_state(USBPowered);

			int retval = resetUSB();

			SET_REG(USB + GINTSTS, GINTMSK_RESET);

			if(retval) {
				bufferPrintf("USB: listening for further usb events\r\n");
				return;	
			}

			process = TRUE;
		}

		if(((status & GINTMSK_INEP) == GINTMSK_INEP) || ((status & GINTMSK_OEP) == GINTMSK_OEP)) {
			// aha, got something on one of the endpoints. Now the real fun begins
			
			getEndpointInterruptStatuses();

			if(isSetupPhaseDone()) {
				// recall our earlier usb_receive_control calls. We now should have 8 bytes of goodness in controlRecvBuffer.
				USBSetupPacket* setupPacket = (USBSetupPacket*) controlRecvBuffer;

				uint16_t length;
				uint32_t totalLength;
				USBStringDescriptor* strDesc;
				if(USBSetupPacketRequestTypeType(setupPacket->bmRequestType) != USBSetupPacketVendor) {
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
									totalLength = getConfigurationTree(setupPacket->wValue & 0xFF, usb_speed, controlSendBuffer);
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
									stall = TRUE;
							}

							if(gUsbState < USBError) {
								if(stall)
								{
									if(setupHandler)
										if(setupHandler(setupPacket))
											break;

									bufferPrintf("Unknown descriptor request: %d\r\n", setupPacket->wValue >> 8);
									stallControl();
								}
								else
									usb_send_control(controlSendBuffer, length);
							}

							break;

						case USB_SET_ADDRESS:
							usb_speed = DSTS_GET_SPEED(GET_REG(USB + DSTS));
							usb_max_packet_size = packetsizeFromSpeed(usb_speed);
							SET_REG(USB + DCFG, (GET_REG(USB + DCFG) & ~DCFG_DEVICEADDRMSK)
								| ((setupPacket->wValue & DCFG_DEVICEADDR_UNSHIFTED_MASK) << DCFG_DEVICEADDR_SHIFT));

							// send an acknowledgement
							usb_send_control(controlSendBuffer, 0);

							if(gUsbState < USBError) {
								change_state(USBAddress);
							}
							break;

						case USB_SET_INTERFACE:
							// send an acknowledgement
							usb_send_control(controlSendBuffer, 0);
							break;

						case USB_GET_STATUS:
							// FIXME: iBoot doesn't really care about this status
							*((uint16_t*) controlSendBuffer) = 0;
							usb_send_control(controlSendBuffer, sizeof(uint16_t));
							break;

						case USB_GET_CONFIGURATION:
							// FIXME: iBoot just puts out a debug message on console for this request.
							break;

						case USB_SET_CONFIGURATION:
							setConfiguration(0);
							// send an acknowledgment
							usb_send_control(controlSendBuffer, 0);

							if(gUsbState < USBError) {
								change_state(USBConfigured);
								startHandler();
							}
							break;
						default:

							if(setupHandler)
							{
								if(setupHandler(setupPacket))
									break;
							}

							// Unknown Request
							{	
								int i;
								uint8_t *pktDump = (uint8_t*)setupPacket;
								bufferPrintf("Bad USB setup packet: ");
								for(i = 0; i < sizeof(USBSetupPacket); i++)
								{
									bufferPrintf("0x%08x ", pktDump[i]);
								}
								bufferPrintf("\n");
							}

							stallControl();
							break;
					}
				}
				else
				{
					do
					{
						if(setupHandler)
						{
							if(setupHandler(setupPacket))
								break;
						}

						// Unknown Request
						{	
							int i;
							uint8_t *pktDump = (uint8_t*)setupPacket;
							bufferPrintf("Bad USB setup packet: ");
							for(i = 0; i < sizeof(USBSetupPacket); i++)
							{
								bufferPrintf("0x%08x ", pktDump[i]);
							}
							bufferPrintf("\n");
						}

						stallControl();
					}
					while(0);
				}

				// get the next SETUP packet
				usb_receive_control(controlRecvBuffer, sizeof(USBSetupPacket));
			}

			//uartPrintf("\t<begin callEndpointHandlers>\r\n");
			callEndpointHandlers();
			//uartPrintf("\t<end callEndpointHandlers>\r\n");

			process = TRUE;
		}

		if(status & GINTMSK_ENUMDONE)
		{
			SET_REG(USB+GINTSTS, GINTMSK_ENUMDONE);

			change_state(USBEnumerated);

			process = TRUE;
		}

		if((status & GINTMSK_SOF) == GINTMSK_SOF) {
			SET_REG(USB + GINTSTS, GINTMSK_SOF);
			process = TRUE;
		}

		if((status & GINTMSK_SUSPEND) == GINTMSK_SUSPEND) {
			SET_REG(USB + GINTSTS, GINTMSK_SUSPEND);
			process = TRUE;
		}

		if(status & GINTMSK_EPMIS)
		{
			SET_REG(USB + GINTSTS, GINTMSK_EPMIS);

			//bufferPrintf("USB: EP Mismatch.\n");

			usb_set_global_in_nak();
			usb_disable_in_endpoints();
			usb_clear_global_in_nak();

			int t1 = GET_REG(USB + DTKNQR1);
			int t2 = GET_REG(USB + DTKNQR2);
			int t3 = GET_REG(USB + DTKNQR3);
			int t4 = GET_REG(USB + DTKNQR4);

			int tkndepth = (GET_REG(USB+GHWCFG2) >> GHWCFG2_TKNDEPTH_SHIFT) & GHWCFG2_TKNDEPTH_MASK;
			int numtokens = t1 & 0xf;
			if(tkndepth < numtokens)
				numtokens = tkndepth;

			//bufferPrintf("USB: Requeing %d requests.\n", numtokens);

			int doneEPs = 0;
			int i;
			for(i = 0; i < numtokens; i++)
			{
				int ep;

				if(i < 6)
					ep = GET_BITS(t1, 8 + (i*4), 4);
				else if(i < 14)
					ep = GET_BITS(t2, (i-6)*4, 4);
				else if(i < 21)
					ep = GET_BITS(t3, (i-14)*4, 4);
				else if(i < 30)
					ep = GET_BITS(t4, (i-21)*4, 4);
				else
					break;

				if(doneEPs & (1 << ep))
					break;

				//bufferPrintf("USB: Requeing %d: 0x%08x.\n", ep, usb_message_queue[ep]);

				continueMessageQueue(ep);

				doneEPs |= (1 << ep);
			}

			for(i = 0; i < USB_NUM_ENDPOINTS; i++)
			{
				if(doneEPs & (1 << i))
					continue;

				USBMessageQueue *q = usb_message_queue[i];
				if(!q)
					continue;

				if(q->dir != USBIn)
					continue;

				//bufferPrintf("USB: Requing unrequested ep %d\n", i);

				continueMessageQueue(i);
			}

			process = TRUE;
		}

		status = GET_REG(USB + GINTSTS) & GET_REG(USB + GINTMSK);

		break;
	}

	//uartPrintf("<end interrupt>\r\n");

}

static void create_descriptors() {
	if(configurations == NULL) {
		deviceDescriptor.bLength = sizeof(USBDeviceDescriptor);
		deviceDescriptor.bDescriptorType = USBDeviceDescriptorType;
		deviceDescriptor.bcdUSB = USB_2_0;
		deviceDescriptor.bDeviceClass = 2;
		deviceDescriptor.bDeviceSubClass = 0;
		deviceDescriptor.bDeviceProtocol = 0;
		deviceDescriptor.bMaxPacketSize = USB_MAX_PACKETSIZE;
		deviceDescriptor.idVendor = 0x525;
		deviceDescriptor.idProduct = PRODUCT_IPHONE;
		deviceDescriptor.bcdDevice = DEVICE_IPHONE;
		deviceDescriptor.iManufacturer = usb_add_string_descriptor("Apple Inc.");
		deviceDescriptor.iProduct = usb_add_string_descriptor("Apple Mobile Device (OpenIBoot Mode)");
		deviceDescriptor.iSerialNumber = usb_add_string_descriptor("");
		deviceDescriptor.bNumConfigurations = 0;

		deviceQualifierDescriptor.bDescriptorType = USBDeviceDescriptorType;
		deviceQualifierDescriptor.bcdUSB = USB_2_0;
		deviceQualifierDescriptor.bDeviceClass = 2;
		deviceQualifierDescriptor.bDeviceSubClass = 0;
		deviceQualifierDescriptor.bDeviceProtocol = 0;
		deviceDescriptor.bMaxPacketSize = USB_MAX_PACKETSIZE;
		deviceDescriptor.bNumConfigurations = 0;

		addConfiguration(1, usb_add_string_descriptor("OpenIBoot Mode Configuration"), 0, 0, 500);
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
			int endpoint = configurations[i].interfaces[j].endpointDescriptors[k].bEndpointAddress & 0x7f; // Was 0x3, why the fuck that was, I'll never know. -- Ricky26
			if(configurations[i].interfaces[j].endpointDescriptors[k].bEndpointAddress & 0x80)
			{
				InEPRegs[endpoint].control |= USB_EPCON_SETD0PID;
			}
			else
			{
				OutEPRegs[endpoint].control |= USB_EPCON_SETD0PID;
			}
		}
	}

	//usb_flush_all_fifos();
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
	configurations[0].descriptor.bNumInterfaces = 0;

	/*if(index == 0 && configurations[0].interfaces == NULL) {
		USBInterface* interface = usb_add_interface(&configurations[0], 0, 0,
			OPENIBOOT_INTERFACE_CLASS, OPENIBOOT_INTERFACE_SUBCLASS, OPENIBOOT_INTERFACE_PROTOCOL, usb_add_string_descriptor("IF0"));

		enumerateHandler(interface);
		endConfiguration(&configurations[0]);
	}*/

	enumerateHandler(&configurations[0]);
	endConfiguration(&configurations[0]);

	return &configurations[index].descriptor;
}

void usb_add_endpoint(USBInterface* interface, int endpoint, USBDirection direction, USBTransferType transferType) {
	if(transferType == USBInterrupt) {
		if(usb_speed == USB_HIGHSPEED)
			addEndpointDescriptor(interface, endpoint, direction, transferType, USBNoSynchronization, USBDataEndpoint, packetsizeFromSpeed(usb_speed), 9);
		else
			addEndpointDescriptor(interface, endpoint, direction, transferType, USBNoSynchronization, USBDataEndpoint, packetsizeFromSpeed(usb_speed), 32);
	} else {
		addEndpointDescriptor(interface, endpoint, direction, transferType, USBNoSynchronization, USBDataEndpoint, packetsizeFromSpeed(usb_speed), 0);
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

USBInterface* usb_add_interface(USBConfiguration* configuration, uint8_t bInterfaceNumber, uint8_t bAlternateSetting, uint8_t bInterfaceClass, uint8_t bInterfaceSubClass, uint8_t bInterfaceProtocol, uint8_t iInterface) {
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

uint8_t usb_add_string_descriptor(const char* descriptorString) {
	uint8_t newIndex;

	for(newIndex = 0; newIndex < numStringDescriptors; newIndex++)
	{
		int i = 0;
		uint16_t *oldStr = (uint16_t*)stringDescriptors[newIndex]->bString;

		if(strlen(descriptorString) != stringDescriptors[newIndex]->bLength)
			continue;

		for(;i < stringDescriptors[newIndex]->bLength; i++)
			if(oldStr[i] != descriptorString[i])
				continue;
	}

	newIndex = numStringDescriptors;
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

static uint16_t packetsizeFromSpeed(uint8_t speed_id) {
	switch(speed_id) {
		case USB_HIGHSPEED:
			return 512;
		case USB_FULLSPEED:
		case USB_FULLSPEED_48_MHZ:
			return 64;
		case USB_LOWSPEED:
			return 32;
		default:
			return -1;
	}
}

int usb_install_setup_handler(USBSetupHandler handler)
{
	setupHandler = handler;
	return 0;
}

int usb_install_ep_handler(int endpoint, USBDirection direction, USBEndpointHandler handler, uint32_t token) {
	if(endpoint >= USB_NUM_ENDPOINTS || endpoint < 0) {
		return -1;
	}

	if(direction == USBIn) {
		endpoint_handlers[endpoint].in.handler = handler;
		endpoint_handlers[endpoint].in.token = token;
	} else if(direction == USBOut) {
		endpoint_handlers[endpoint].out.handler = handler;
		endpoint_handlers[endpoint].out.token = token;
	} else {
		return -1; // can only register IN or OUt directions
	}

	return 0;
}

int usb_shutdown()
{
#ifndef CONFIG_IPOD2G
	power_ctrl(POWER_USB, ON);
#endif

	clock_gate_switch(USB_OTGCLOCKGATE, ON);
	clock_gate_switch(USB_PHYCLOCKGATE, ON);

	SET_REG(USB + USB_ONOFF, GET_REG(USB + USB_ONOFF) | USB_ONOFF_OFF); // reset link
	SET_REG(USB_PHY + OPHYPWR, OPHYPWR_FORCESUSPEND | OPHYPWR_PLLPOWERDOWN
		| OPHYPWR_XOPOWERDOWN | OPHYPWR_ANALOGPOWERDOWN | OPHYPWR_UNKNOWNPOWERDOWN); // power down phy

	SET_REG(USB_PHY + ORSTCON, ORSTCON_PHYSWRESET | ORSTCON_LINKSWRESET | ORSTCON_PHYLINKSWRESET); // reset phy/link

	udelay(USB_RESET_DELAYUS);	// wait a millisecond for the changes to stick
	
	SET_REG(USB + DCTL, GET_REG(USB + DCTL) | DCTL_SFTDISCONNECT);

	clock_gate_switch(USB_OTGCLOCKGATE, OFF);
	clock_gate_switch(USB_PHYCLOCKGATE, OFF);
	power_ctrl(POWER_USB, OFF);

	releaseConfigurations();
	releaseStringDescriptors();

	return 0;
}

static char *state_names[] = {
	"start",
	"powered",
	"enumerated",
	"address",
	"configured",
};

static void change_state(USBState new_state) {
	bufferPrintf("USB: State change: %s -> %s\r\n", state_names[gUsbState], state_names[new_state]);
	gUsbState = new_state;
	if(gUsbState == USBConfigured) {
		// TODO: set to host powered
	}
}

USBSpeed usb_get_speed() {
	switch(usb_speed) {
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
