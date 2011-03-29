#include "openiboot.h"
#include "util.h"
#include "hardware/clcd.h"
#include "hardware/mipi_dsim.h"
#include "timer.h"
#include "i2c.h"
#include "tasks.h"
#include "gpio.h"
#include "lcd.h"
#include "mipi_dsim.h"
#include "openiboot-asmhelpers.h"
#include "pmu.h"
#include "commands.h"

static GammaTableDescriptor PinotGammaTables[] = {
#if defined(CONFIG_A4)
// iPhone 4
	{0x970548, 0xFFFF7F,
		{{0x74CA62, 0x1DD0, 0x4D374D0D, 0x40D000D3, 0x3434003, 0xD00D, 0, 0x7000070, 0xD0001C0, 0xD3743400, 0xD0D34D34, 0xC01C0034, 0x34D0D701, 0x1C00000D, 0x4D0D0000, 0xC0700003, 0xC1, 0}},
		{{0x401DCA62, 0xDD000077, 0xDC771DDD, 0x40D0D0DD, 0x34343403, 0xD000D0, 0x70000000, 0x7007000, 0x3400070, 0xD0D34D00, 0x34343434, 0x71C1C000, 0x40034070, 0x1C71C1C3, 0x340D01C7, 0xC01C0000, 0xC1, 0}},
		{{0x1DCA62, 0x34000074, 0x4DDDDDDD, 0x37, 0xD000000, 0x70000000, 0x70007000, 0x701C0700, 0x1C0, 0x4DD00D00, 0xD0D34DD3, 0x70700034, 0xD34D1C1C, 0x1C3434, 0xD34D001C, 0x3434, 0, 0}},
	},
	{0xC20548, 0xFFFF7F,
		{{0xD071CA62, 0xDD34071D, 0xC71C771D, 0x340D34D, 0xD0, 0x71DC1C00, 0x7771DDC7, 0x7771D377, 0x71DDDC, 0xD0D07000, 0xD374D34D, 0xD0D34DDD, 0xD374D0D0, 0x1C0034D, 0x34001C07, 0x4DD34000, 0x37, 0}},
		{{0x4001CA62, 0xC74D001C, 0x71C07071, 0xC00D0377, 0xD0003401, 0xDC71C1C0, 0xDDC77771, 0xDDC774DD, 0x77771, 0x34000007, 0x4D34D34D, 0xD0D37, 0x77743400, 0x377, 0x40000007, 0x43434003, 3, 0}},
		{{0xCAC0A62, 0xC1D3401D, 0x1C00001, 0xD371C, 0x70, 0x71DC7070, 0xDDDC7777, 0x71DDDDDD, 0x71DDC7, 0xD3407000, 0x374DD34D, 0x3434DD, 0x374D0000, 0x700034D, 0xD00701C, 0, 0, 0}},
	},
// iPad
	{0xB30689, 0xFFFFBF,
		{{0x4CAC0A62, 0x71DDD000, 0x70034377, 0x701C0700, 0x1DC71C70, 0xDDDC7777, 0x77777771, 0xC77771DC, 0xC71DC71D, 0x1C070071, 0x1C0, 0x70, 0, 0x1C01C1D0, 0xC71C1C1C, 0xD001C771, 0x374, 0}},
		{{0x4CAC0A62, 0xC7137400, 0x70034DD, 0x701C1C70, 0xC771C71C, 0xDDDDDDDD, 0x77777771, 0x77771DC7, 0xC71DC71C, 0x1C070071, 0x1C0, 0, 0, 0x700774, 0x70701C07, 0xD0771C7, 0xDD, 0}},
		{{0x1341CA62, 0x77777400, 0x3437, 0xC001C000, 0x71C1C701, 0x1C71DDC7, 0xC71DC777, 0x71C71C71, 0x1C71C70, 0x7001C0, 0x40000000, 0x40000003, 0xC4000003, 0xC7070701, 0x4D3771C1, 0xDC7774D3, 0xDD, 0}}
	},
	{0x970689, 0xFFFFBF,
		{{0x4DCE22, 0x1C01C740, 0xC034D000, 0x1C0771C1, 0x1C0700, 0xD00000, 0x34D0340D, 0xD34D0D34, 0x71C03434, 0x1C71DDC7, 0x1C1C71C7, 0x700700, 0x10007007, 0x1C1C0000, 0xD37771C, 0x707740D, 0xC7, 0}},
		{{0xB04DCE22, 0x1C1C7432, 0xD1C00, 0x1DC771C7, 0x1C70701C, 0, 0xD0034034, 0xD0D0D0D0, 0x1CD34374, 0x771C771C, 0x71C71C1C, 0x1C001C00, 0x1C000, 0x734001D, 0xDDDC1C1C, 0x1C74D34, 0xC1C, 0}},
		{{0x1D0DCE22, 0x1C774D0, 0xDC7400, 0x70000, 0x40000000, 0x4340D003, 0x377434D3, 0xDDD374DD, 0x71C0D34, 0x1C1C1C77, 0x1C01C71C, 0x70007000, 0x747000, 0x77770070, 0x34377, 0, 0, 0}},
	},
	{0xE50689, 0xFFFFBF,
		{{0xD01DCA62, 0xD340001D, 0x3434, 0xD0D00000, 0x74DD3400, 0x34D34D37, 0x340D034, 0xD00, 0xD, 0x1C71C000, 0x1C7071C7, 0x4D34DC77, 0x701D34D3, 0xD0D1C000, 0xDDD34D34, 0x701C7774, 0xC0, 0}},
		{{0x741DCA62, 0x4D340007, 0x34D3, 0x34001C0, 0x4DD0D000, 0xD0D0D0D3, 0xD00, 0x7A400000, 0xD00000, 0xDC701C00, 0x71C71C71, 0xD37771C7, 0x7434D34, 0x4000001C, 0x713774D3, 0x1C71C, 0, 0}},
		{{0x1DCA62, 0xD3400071, 0xD0D03434, 0x4DD340D0, 0x71DC74D3, 0xD3777777, 0x40D3434D, 0xD00D03, 0x34034, 0x71C1C1C0, 0xC71C1C70, 0x4D34D371, 0x1C01C4D3, 0x40D001C0, 0x40340D03, 0x3403403, 0, 0}},
	},
// iPod Touch 4G
	{0xD10B08, 0xFFFF7F,
		{{0x71DDD122, 0x1001D00, 0x370C0D05, 0x74DD34D0, 0x70034D37, 0x374D0000, 0xDDD374DD, 0xD00D0374, 0, 0x70701C00, 0x1C070, 0x71C74000, 0x1C1C1C70, 0x1C007D3, 0x7000007, 0x7000, 0xC, 0}},
		{{0x35E82822, 0x1D001DDD, 0x4DD14747, 0xD374DF7A, 0x377771DD, 0xD000D, 0x434D3434, 0x40D0D0D3, 0x340003, 0, 0x1C007000, 0x74000000, 0x1C71DC77, 0xD31C1C, 0xD00001C, 0xD00, 0xC000, 0}},
		{{0x4D00D422, 0x10D07077, 0xDD045D7, 0xDDD37773, 0xD0DDDDDD, 0xDD000000, 0x71C771D, 0xD37771C7, 0xD00D0D34, 0, 0x1C0, 0x70713400, 0xC71DC71C, 0xC1C07034, 0x1C1, 0xD007, 0, 0}},
	},
	{0xC20B08, 0xFFFF7F,
		{{0xD0004022, 0x1C77401D, 0x71DD34, 0xC77774D0, 0x1C01C1, 0x771DC1C, 0xC070701C, 0x3771C1C1, 0xD034D34D, 0x40000340, 3, 0xD0000000, 0xDC713400, 0x1C34DD, 0xD000D00, 0x774D0000, 3, 0}},
		{{0x341C4022, 0x774001D, 0x1C4D0D7, 0x4DD0032B, 0x701C71DC, 0x777771C0, 0x71DC7713, 0xDDC771C7, 0xD0D3434D, 0x340340, 0x340000D, 0x34340034, 0x71DD0D34, 0xD3771C0, 0x1C71D034, 0x1CD31C77, 0xC0, 0}},
		{{0xD0004022, 0x1C4D071, 0x1C71340, 0xDD34D0D0, 0x71DC7771, 0x4DD37777, 0xDDD37777, 0xD374DDD, 0x34000, 0x700001C, 0x701C070, 0x1C0070, 0x74D0D34D, 0x37707070, 0xC7743434, 0x7741C1, 0xC, 0}},
	},
	{0xA40B08, 0xFFFF7F,
		{{0x4071CA62, 0x74D07077, 0x7070777, 0xD3771C1C, 0x71DDC000, 0x77, 0xD0000, 0x1C00000, 0xDC771C1C, 0xDDDDDDDD, 0x1C777774, 0x1D001C1C, 0x1C0, 0xC0003700, 0x1C070001, 0x77740070, 0x37, 0}},
		{{0x1DDCA62, 0xD001C774, 0x771DDD34, 0xD34DDDDC, 0x1DC71C00, 0x71C77, 0x1C000, 0x701C0000, 0x1C71C070, 0x71C771C7, 0xC071C71C, 0x1D000001, 0, 0xD0C1C0, 0xD0340340, 0x40003740, 0xC3, 0}},
		{{0x1DCA62, 0xD0000774, 0xC71C7774, 0x3777771, 0x777707A4, 0x1C77777, 0x70070, 0x71C07007, 0x7771C71C, 0x777774DC, 0x74DDDD37, 0x1C01C7, 0x34DD3410, 0x31E9034D, 0xD034034, 0x77434034, 0xDDDC, 0}},
	},
	{0x8A0B08, 0xFFFF7F,
		{{0xC4040422, 0x740075, 0x1C71DDD0, 0x7D0DDC7, 0x340340D3, 0x77777771, 0xD37777, 0x1C00000, 0xDDC70707, 0xDDDDDDDD, 0xDDDDDDDD, 0x1C001, 0xD00D000, 0xDDDDDDC7, 0xC007071D, 0x74000701, 3, 0}},
		{{0x72B10422, 0xC100132B, 0x71D0CA, 0x77007000, 0xD377003, 0x777774DD, 0x34374DD3, 0x1C0001C0, 0x1DC7071C, 0x37777777, 0xD374DDDD, 0xDDD3774D, 0x32B1D34, 0xC0700340, 0x341C1C71, 0xD000000, 0x37700, 0}},
		{{0x74040422, 0x71001D0, 0x71DDD340, 0x71C34D37, 3, 0xD03400, 0x340D0D, 0xC701C1C0, 0x37777771, 0x434D34DD, 0x4D34D0D3, 0xDDDD34D3, 0xD01DDDDD, 0x34D34, 0x3401C01C, 0x4504034, 0xC, 0}},
	},
#endif
#if defined(CONFIG_IPOD_TOUCH_2G)
// iPod Touch 2G -- Should not be in plat-a4
	{0x970387, 0xFFFFFF,
		{{0x4041D422, 0x4001DD07, 0xDDDC003, 0x1C0, 0xDDDDDC00, 0x7774, 0xDC701C00, 0x74DDC771, 0x704D34D3, 0x7000, 0, 0x1C000000, 0x1DDC7777, 0xD000D00, 0x34D371D, 0x8A5D034D, 0xEB42, 0}},
		{{0x7474422, 0x1C774074, 0x34D31C1C, 0x1C0, 0xDC770070, 0x4001DDDD, 0xC0000D03, 0xC1C70001, 0x71C71D, 0xD00000, 0x40D00034, 0xD00D03, 0xC74D0700, 0x771DC771, 0x3777, 0xE904AD1C, 0xCFAC, 0}},
		{{0x101D422, 0x1C7741D, 0x34C70000, 0x34000, 0xDDDC7000, 0x40001DDD, 0xC0700003, 0x37777071, 0x1DD34DD, 0x1C070707, 0x1C1C1C70, 0x71C70707, 0x74DDD377, 0x3470, 0x377774D0, 0x92804D0D, 0xE843, 0}},
	},
	{0xE50387, 0xFFFFFF,
		{{0x40441122, 0xD0D1C40, 0xC34CDC7D, 0x4DC71C71, 0x40D00D03, 0xD34D347, 0xD, 0xC71C71C0, 0x340D0D01, 0x40340340, 0xC0003403, 0x374DD371, 0x701C071D, 0xDC, 0xDC410000, 0xC0041DDD, 0xE8, 0}},
		{{0x1CAD1122, 0x435C72B1, 0x37DC0037, 0xDC71DC00, 0x343434DD, 0x74, 0xC001C00D, 0xDC71C701, 0xC71D001D, 0xC71DC771, 0x9377771D, 0xD0000DDE, 0xDDD34000, 0x71DDDD, 0x1D00007, 0xA771C000, 0x7A7037, 0}},
		{{0x40441122, 0x34340740, 0xDC3700, 0x4DC71C77, 0xD00003, 0x34D34D1D, 0xD, 0x71C71C70, 0xDDDDDD00, 0xDDDDDDDD, 0x70D374DD, 0x1DDDDDDC, 0x1C01C007, 0x40D03437, 0xD00003, 0x80157400, 0xE9E, 0}},
	},
	{0xC20385, 0xFFFFFF,
		{{0xB4044122, 0xDDDD0772, 0xC3700371, 0x71, 0x1C0000, 0x34000007, 0x70001C00, 0x7000, 0, 0x70000, 0xD0000000, 0x7000000, 0x1C0, 0x40343400, 0x1C707003, 0xDC4D0007, 0xE9C, 0}},
		{{0x74044122, 0x771D001D, 0xC0030037, 0x7001D, 0x7001C0, 0, 0xC70701C0, 0x1C01, 0x1C00, 0x1C001C0, 0xC01C01C0, 0x1C01, 0x1C00700, 0x7001C, 0xC0007007, 0x7D011001, 0x3A, 0}},
		{{0x1D744122, 0xDC74D1C4, 0xC00DD, 7, 0x1C0001C0, 0, 0x701C000, 0x7001C007, 0, 0x700007, 0x70001C0, 0x70701C00, 0x70070, 0x701C007, 0x71C1C1C, 0xCC4411C7, 0xC, 0}},
	},
	{0xD10384, 0xFFFFFF,
		{{0xB4110522, 0x3434772, 0xD374C030, 0x73777771, 0x1C771C, 0x4D343434, 0xD34DD37, 0x7A40000, 0x70000, 0x701C0070, 0xDDC71C70, 0x4343434D, 0xDDC77477, 0x71C707A4, 0xD0D371C, 0x71D34, 0xCDF4, 0}},
		{{0xB4110522, 0xC0D00132, 0x1C00C0D, 0xD34D0D34, 0x400D0000, 0x4D3434D3, 0x403434D3, 3, 0x1C000, 0x1C00000, 0x77070700, 0x1374D377, 0xD00D07, 0xD3771DDC, 0x771C71DD, 0xD000137, 0xF7, 0}},
		{{0x1110522, 0x371C7401, 0x10035F70, 0xD0D37003, 0x771C74D0, 0xDDDDDDDC, 0xD0034, 0x700000, 0xC1C07070, 0x71C71DC1, 0xDDDDDDDC, 0x3471DD, 0xD0D3000, 0xD0D0D, 0x40000340, 0xC0D00D03, 0, 0}},
	},
	{0x970787, 0xFFFFFF,
		{{0x40135022, 0x1D001D00, 0x71D3400, 0x71C07007, 0x771C0D37, 0x34D0D3, 0x71C00000, 0xD01DC71C, 0xC774D374, 0x71DDDDDD, 0xD0CD377, 0xC1DDD34D, 0xDC701C01, 0x37774DDD, 0x34D0D34D, 0x4040340D, 3, 0}},
		{{0xD1DD1022, 0x7401DD71, 0x74D00007, 0xD34DDDD3, 0xDDDC00, 0x4D34D340, 0x34003, 0x1C0070, 0x40D00000, 0xD034343, 0x7343434D, 0x71DC770, 0xD0343400, 0x7000001C, 0x1C01C000, 0xCC70000, 0, 0}},
		{{0x1D01022, 0x1D34710, 0x1C74D00, 0x701C0007, 0x7A40D31C, 0x1C00, 0x70D0034, 0xDC7001C0, 0x1C7771DD, 0, 0, 0xC0000D00, 0x77771DDD, 0x34340D0, 0x340, 0xA174D0D0, 0xC44, 0}},
	},
	{0xD10784, 0xFFFFFF,
		{{0x2B1D1022, 0x74013, 0x4D035C1D, 0x374DDD3, 0xDDC70D, 0xD03401C, 0xD374DD0D, 0x340D, 0, 0x1C001C, 0x700070, 0x1C0007, 0xD00032B0, 0xDDDDC700, 0x1C713774, 0x1DD071C7, 0xDC7, 0}},
		{{0x40135022, 0x74D71D07, 0xD34D0007, 0xD34DDD, 0xDC00DDC0, 1, 0x34D34D0D, 0xD000D, 0x70000000, 0, 0, 0x34000D0, 0x34DD374, 0x1C71DC70, 0x1DC771C7, 0x70707007, 0xC, 0}},
		{{0x741022, 0x1D35C4, 0xD0701C4D, 0x34000, 0xC00D31C7, 0xD3740001, 0x77771374, 0x340D377, 0xD, 0x734, 0x7000000, 0, 0x40D0D000, 0xDC771C73, 0x70000071, 0xD0340D0, 0, 0}},
	},
	{0xC20785, 0xFFFFFF,
		{{0x74740422, 0x1D00740, 0xC01C7740, 0xDC700001, 0x34DC734D, 0xC0000000, 0x7771C1C1, 0x774DD377, 0x71C71C1C, 0x1C71C71C, 0xC71C71C7, 0x701C071, 0xD0007, 0x7070000, 0x71C1C707, 0x70401C, 0xC, 0}},
		{{0x74740422, 0x74004D7, 0x1C777400, 0x371C7070, 0x34C700D, 0xC0000700, 0xC71C71C1, 0xDD3774DD, 0xC0701C1D, 0x1C0701C1, 0x70070070, 0x1C1C0700, 0xD00D070, 0x1C000000, 0x7070700, 0x704007, 0xC, 0}},
		{{0x40135022, 0xDD01DD07, 0x774D0001, 0x34DDDC77, 0x377000, 0x3400, 0xC0701C00, 0x77777771, 0x1C077, 0x7001C007, 0x7001C00, 0xC01C0070, 0xD0340001, 0x340, 0xC07001C0, 0x1C1D001, 0xC, 0}},
	},
	{0xC20784, 0xFFFFFF,
		{{0x1D01022, 0x1D0074, 0x341C1DD0, 0xD34D34D0, 0x70000034, 0x7070701C, 0x377771DC, 0x3400D00D, 0x70001C, 0xDC71C1C7, 0x7771DC71, 0x77771DDC, 0xD034007, 0x71C71C00, 0x1DC771C7, 0x701D077, 0xC, 0}},
		{{0x1D01022, 0x1C740010, 0xB0071D34, 0xD00032, 0x771C0000, 0x77774DD3, 0xD3777777, 0xD0034D, 0x1C0000, 0x701C0070, 0x1C0701C0, 0x1C1C1C1C, 0xDC74001C, 0x1C000DD, 0x7771C707, 0x7171DC, 0, 0}},
		{{0x7401022, 0x74D0774, 0x1C1DDD00, 0xD000D000, 0x71C1C000, 0x71C71DC7, 0x7777771C, 0x3400D037, 0x7000000, 0x70701C0, 0xC7070707, 0xC71C1C71, 0x4DD34071, 0xC0700003, 0xC1C1C1C1, 0x1C75C1, 0xC, 0}},
	},
#endif
};

static LCDInfo LCDInfoTable[] = {
// iPhone 4
	{"n90", 0xA, 0x30EC6A0, 0x146, 640, 0x47, 0x47, 0x49, 960, 0xC, 0xC, 0x10, 0, 0, 0, 0, 24, 3, 0x8391643},
// iPad
	{"k48", 0xA, 0x413B380, 0x84, 1024, 0x85, 0x85, 0x87, 768, 0xA, 0xA, 0xC, 0, 0, 0, 0, 18, 3, 0x644},
// iPod Touch 4G
//	{"n81", 0xA, 0x30EC6A0, 0x146, 640, 0x47, 0x47, 0x49, 960, 0xC, 0xC, 0x10, 0, 0, 0, 0, 24, 3, 0x35731643},
	{"n81", 0xA, 0x30EC6A0, 0x146, 640, 0x47, 0x47, 0x49, 960, 0xC, 0xC, 0x10, 0, 0, 0, 0, 24, 3, 0x8391643},
// iPod Touch 2G -- Should not be in plat-a4
	{"n72", 0xB, 0xA4CB80, 0xA3, 320, 0xC, 0xC, 0x10, 480, 6, 6, 8, 0, 0, 0, 0, 24, 3, 0x222},
// aTV 2G
	{"720p", 0xB, 0, 0xF3, 1280, 0xDC, 0x6E, 0x28, 720, 0x14, 5, 5, 0, 0, 0, 0, 24, 5, 0},
};

static uint32_t atv_values[] = {
	0x10044, 0x11048, 0x1204C, 0x13050, 0x14050, 0x14054, 0x15058, 0x1605C, 0x17060, 0x18064, 0x19068, 0x1A06C,
	0x1B06C, 0x1B070, 0x1C074, 0x1D078, 0x1E07C, 0x1F080, 0x20084, 0x21088, 0x22088, 0x2208C, 0x23090, 0x24094,
	0x25098, 0x2609C, 0x270A0, 0x280A4, 0x290A4, 0x290A8, 0x2A0AC, 0x2B0B0, 0x2C0B4, 0x2D0B8, 0x2E0BC, 0x2F0C0,
	0x300C4, 0x310C4, 0x310C8, 0x320CC, 0x330D0, 0x340D4, 0x350D8, 0x360DC, 0x370E0, 0x380E0, 0x380E4, 0x390E8,
	0x3A0EC, 0x3B0F0, 0x3C0F4, 0x3D0F8, 0x3E0FC, 0x3F0FC, 0x3F100, 0x40104, 0x41108, 0x4210C, 0x43110, 0x44114,
	0x45118, 0x46118, 0x4611C, 0x47120, 0x48124, 0x49128, 0x4A12C, 0x4B130, 0x4C134, 0x4D134, 0x4D138, 0x4E13C,
	0x4F140, 0x50144, 0x51148, 0x5214C, 0x53150, 0x54154, 0x55154, 0x55158, 0x5615C, 0x57160, 0x58164, 0x59168,
	0x5A16C, 0x5B170, 0x5C170, 0x5C174, 0x5D178, 0x5E17C, 0x5F180, 0x60184, 0x61188, 0x6218C, 0x6318C, 0x63190,
	0x64194, 0x65198, 0x6619C, 0x671A0, 0x681A4, 0x691A8, 0x6A1A8, 0x6A1AC, 0x6B1B0, 0x6C1B4, 0x6D1B8, 0x6E1BC,
	0x6F1C0, 0x701C4, 0x711C8, 0x721C8, 0x721CC, 0x731D0, 0x741D4, 0x751D8, 0x761DC, 0x771E0, 0x781E4, 0x791E4,
	0x791E8, 0x7A1EC, 0x7B1F0, 0x7C1F4, 0x7D1F8, 0x7E1FC, 0x7F200, 0x80200, 0x80204, 0x81208, 0x8220C, 0x83210,
	0x84214, 0x85218, 0x8621C, 0x8721C, 0x87220, 0x88224, 0x89228, 0x8A22C, 0x8B230, 0x8C234, 0x8D238, 0x8E238,
	0x8E23C, 0x8F240, 0x90244, 0x91248, 0x9224C, 0x93250, 0x94254, 0x95258, 0x96258, 0x9625C, 0x97260, 0x98264,
	0x99268, 0x9A26C, 0x9B270, 0x9C274, 0x9D274, 0x9D278, 0x9E27C, 0x9F280, 0xA0284, 0xA1288, 0xA228C, 0xA3290,
	0xA4290, 0xA4294, 0xA5298, 0xA629C, 0xA72A0, 0xA82A4, 0xA92A8, 0xAA2AC, 0xAB2AC, 0xAB2B0, 0xAC2B4, 0xAD2B8,
	0xAE2BC, 0xAF2C0, 0xB02C4, 0xB12C8, 0xB22CC, 0xB32CC, 0xB32D0, 0xB42D4, 0xB52D8, 0xB62DC, 0xB72E0, 0xB82E4,
	0xB92E8, 0xBA2E8, 0xBA2EC, 0xBB2F0, 0xBC2F4, 0xBD2F8, 0xBE2FC, 0xBF300, 0xC0304, 0xC1304, 0xC1308, 0xC230C,
	0xC3310, 0xC4314, 0xC5318, 0xC631C, 0xC7320, 0xC8320, 0xC8324, 0xC9328, 0xCA32C, 0xCB330, 0xCC334, 0xCD338,
	0xCE33C, 0xCF33C, 0xCF340, 0xD0344, 0xD1348, 0xD234C, 0xD3350, 0xD4354, 0xD5358, 0xD635C, 0xD735C, 0xD7360,
	0xD8364, 0xD9368, 0xDA36C, 0xDB370, 0xDC374, 0xDD378, 0xDE378, 0xDE37C, 0xDF380, 0xE0384, 0xE1388, 0xE238C,
	0xE3390, 0xE4394, 0xE5394, 0xE5398, 0xE639C, 0xE73A0, 0xE83A4, 0xE93A8, 0xEA3AC, 0xEB3B0, 0xEC3B0, 0xEC3B4,
	0xED3B8, 0xEE3BC, 0xEF3C0, 0xEF3C0
};

static const PMURegisterData backlightOffData = {0x68, 0x0};

static const PMURegisterData backlightData[] = {
	{0x66, 0xD4},
	{0x67, 0x0},
	{0x68, 0x15}
};

volatile uint32_t* CurFramebuffer;
uint32_t* FRAMEBUFFER;

static LCDInfo* LCDTable;
static uint32_t TimePerMillionFrames = 0;

static uint32_t dword_5FF3D0D0;

static int gammaVar1;
static int gammaVar2;
static uint8_t gammaVar3;

Window* currentWindow;

static int LCDTableunkn1is0xA = 0;
static int LCDTableunkn1is0xB = 0;

OnOff SyncFramebufferToDisplayActivated = OFF;
uint32_t framebufferLastFill = 0;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow);

void installGammaTables(int panelID, int maxi, uint32_t buffer1, uint32_t buffer2, uint32_t buffer3);
void setWindowBuffer(int window, uint32_t* buffer);
static Window* createWindow(int zero0, int zero2, int width, int height, ColorSpace colorSpace);
static void createFramebuffer(Framebuffer* framebuffer, uint32_t framebufferAddr, int width, int height, int lineWidth, ColorSpace colorSpace);
static void framebuffer_fill(Framebuffer* framebuffer, int x, int y, int width, int height, int fill);
static void hline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void hline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void vline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void vline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill);

void displaytime_sleep(uint32_t time) {
	task_sleep(time * TimePerMillionFrames / 1000);
}

void sub_5FF08870(uint8_t arg) {
	uint8_t v1;

	v1 = (1049-50*arg)/50;
	pmu_write_reg(0x6B, v1, 1);
	task_sleep(10);
	pmu_write_reg(0x6C, v1 + 4, 1);
}

void lcd_fill_switch(OnOff on_off, uint32_t color) {
	if (on_off == ON) {
		if (color)
			framebufferLastFill = color;

		framebuffer_fill(&currentWindow->framebuffer, 0, 0, currentWindow->framebuffer.width, currentWindow->framebuffer.height, color);
		if (LCDTableunkn1is0xA)
			SET_REG(0x89200050, GET_REG(0x89200050) | 1);
		if (LCDTableunkn1is0xB)
			SET_REG(0x89600000, 9);
	} else {
		if (SyncFramebufferToDisplayActivated == OFF) // It used to return, when on_off == SyncFramebufferToDisplayActivated == ON, too -- Bluerise
			return;
		if (LCDTableunkn1is0xA) {
			SET_REG(0x89200050, GET_REG(0x89200050) & (~1));
			while (!(GET_REG(0x89200050) & 2)) ;
		}
		if (LCDTableunkn1is0xB) {
			SET_REG(0x89600008, 1);
			while(GET_REG(0x89600008) & 1);
		}
	}
	SyncFramebufferToDisplayActivated = on_off;
}

void lcd_fill(uint32_t color) {
	lcd_fill_switch(ON, color);
}

void configureLCDClock(uint32_t unkn1, int zero0, int zero1, int zero2, int zero3, unsigned int divider) {
	uint32_t v6;
	uint32_t v7;
	uint32_t v8;
	uint32_t v10;

	// I actually hate this part. -- Bluerise
	// <CPICH> it's just result = r0 - r1*(r0/r1)
	// <CPICH> uint64_t r0, r1;

	if (unkn1 == 0xA) {
		v6 = 31;
		v7 = CalculatedFrequencyTable[3];
		v8 = CalculatedFrequencyTable[3] / divider;
		do
		{
			if (!(v8 % v6))
				break;
			--v6;
		}
		while (v6 != 1);
		SET_REG(0xBF100054, (GET_REG(0xBF100054) & 0xFFFFFFE0) | v6);
		v10 = v8 / v6;
		SET_REG(0xBF100094, (GET_REG(0xBF100094) & 0xFFFFFFE0) | v10);
//		unk_5FF3D078 = v7 / v6;
		dword_5FF3D0D0 = v7 / v6 / v10;
	}
}

int displaypipe_init() {
	int result = 0;
	uint32_t clcd_reg;
	uint32_t panelID;

	FRAMEBUFFER = malloc(0x800000);
	if (!FRAMEBUFFER)
		FRAMEBUFFER = (uint32_t*)CLCD_FRAMEBUFFER;

	memset((void*)FRAMEBUFFER, 0, 0x800000);

	bufferPrintf("displaypipe_init: framebuffer address: 0x%08x\n", (uint32_t)FRAMEBUFFER);

	if (!LCDTable)
		LCDTable = &LCDInfoTable[DISPLAYID];

	if (LCDTable->unkn1 == 0xA) {
		clock_gate_switch(0x12, ON);
		clock_gate_switch(0xF, ON);
		LCDTableunkn1is0xA = 1;
	} else if (LCDTable->unkn1 == 0xB) {
		clock_gate_switch(0x13, ON);
		clock_gate_switch(0xD, ON);
		LCDTableunkn1is0xB = 1;
	}
	SET_REG(CLCD + 0x104C, GET_REG(CLCD + 0x104C) | 0x10);
	SET_REG(CLCD + 0x104C, (GET_REG(CLCD + 0x104C) & 0xFFFFF8FF) | 0x100);
	SET_REG(CLCD + 0x104C, (GET_REG(CLCD + 0x104C) & 0xF800FFFF) | 0x4000000);
	SET_REG(CLCD + 0x1030, (LCDTable->width << 16) | LCDTable->height);
	if (CLCD == 0x89000000) {
		clcd_reg = 0x180;
	} else if (CLCD == 0x89100000) {
		clcd_reg = 0x1C0;
	} else {
		system_panic("displaypipe_init: unsupported displaypipe_base_addr: 0x%08lx\r\n", CLCD);
	}
	SET_REG(CLCD + 0x205C, (clcd_reg << 16) | 0x1F0);
	SET_REG(CLCD + 0x2060, 0x90);
	SET_REG(CLCD + 0x105C, 0x13880801);
	SET_REG(CLCD + 0x2064, 0xBFF00000);
	if (LCDTableunkn1is0xA) {
		SET_REG(0x89200000, 0x100);
		while (GET_REG(0x89200000) & 0x100);
		udelay(1);
		SET_REG(0x89200000, 4);
		SET_REG(0x89200004, 3);
		SET_REG(0x89200014, 0x80000001);
		SET_REG(0x89200018, 0x20408);
		if (LCDTable->bitsPerPixel <= 18)
			SET_REG(0x89200014, GET_REG(0x89200014) | 0x1110000);
		SET_REG(0x89200050, 0);
		SET_REG(0x89200054, (LCDTable->IVClk << VIDCON1_IVCLKSHIFT) | (LCDTable->IHSync << VIDCON1_IHSYNCSHIFT) | (LCDTable->IVSync << VIDCON1_IVSYNCSHIFT) | (LCDTable->IVDen << VIDCON1_IVDENSHIFT));
		SET_REG(0x89200058, ((LCDTable->verticalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT) | ((LCDTable->verticalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT) | ((LCDTable->verticalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
		SET_REG(0x8920005C, ((LCDTable->horizontalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT) | ((LCDTable->horizontalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT) | ((LCDTable->horizontalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
		SET_REG(0x89200060, ((LCDTable->width - 1) << VIDTCON2_HOZVALSHIFT) | ((LCDTable->height - 1) << VIDTCON2_LINEVALSHIFT));
		TimePerMillionFrames = 1000000
			* (LCDTable->verticalBackPorch
				+ LCDTable->verticalFrontPorch
				+ LCDTable->verticalSyncPulseWidth
				+ LCDTable->height)
			* (LCDTable->horizontalBackPorch
				+ LCDTable->horizontalFrontPorch
				+ LCDTable->horizontalSyncPulseWidth
				+ LCDTable->width)
			/ LCDTable->DrivingClockFrequency;
	}
	if (LCDTableunkn1is0xB) {
		SET_REG(0x89600008, 1);
		while(GET_REG(0x89600008) & 1);
		SET_REG(0x8960001C, 0);
		SET_REG(0x89600024, 0);
		SET_REG(0x89600050, LCDTable->width - 1);
		SET_REG(0x89600044, LCDTable->height - 1);
		SET_REG(0x89600004, 0x23);
		uint32_t some_value = GET_REG(0x89600054);
		SET_REG(0x89600054, some_value & 0xFFFE0F00);
		uint32_t i;
		for (i = 0; i <= 0x100; i++) {
			SET_REG(0x89600054, i | some_value | 0x14000);
			SET_REG(0x89600058, atv_values[i] & 0xFFFFF);
		}
		for (i = 0; i <= 0x100; i++) {
			SET_REG(0x89600054, i | some_value | 0x15000);
			SET_REG(0x89600058, atv_values[i] & 0xFFFFF);
		}
		for (i = 0; i <= 0x100; i++) {
			SET_REG(0x89600054, i | some_value | 0x16000);
			SET_REG(0x89600058, atv_values[i] & 0xFFFFF);
		}
		SET_REG(0x8960004C, 9);
	}

	ColorSpace colorSpace;
//XXX:	It normally grabs it from nvram var "display-color-space" as string. -- Bluerise
	if (LCDTable->bitsPerPixel <= 18)
		colorSpace = RGB565;
	else
		colorSpace = RGB888;

	currentWindow = createWindow(0, 0, LCDTable->width, LCDTable->height, colorSpace);
	if (!currentWindow)
		return -1;

//XXX:	It sets the framebuffer address into nvram var "framebuffer". -- Bluerise
//	nvram_setvar("framebuffer", currentWindow->framebuffer.buffer, 0);
	configureLCDClock(LCDTable->unkn1, 0, 0, 0, 0, LCDTable->DrivingClockFrequency);
	if (LCDTable->unkn17 == 3) {
		result = pinot_init(LCDTable, colorSpace, &panelID, currentWindow);
		if (result) {
			lcd_fill_switch(OFF, 0);
			LCDTable = 0;
			return result;
		}
	} else {
		panelID = 0;
	}

	if (SyncFramebufferToDisplayActivated == OFF)
		lcd_fill_switch(ON, framebufferLastFill);

	uint32_t* buffer1 = malloc(1028);
	uint32_t* buffer2 = malloc(1028);
	uint32_t* buffer3 = malloc(1028);
	uint32_t curBuf;
	buffer1[0] = 1;
	for (curBuf = 0; curBuf != 256; curBuf++) {
		if ((curBuf+1 % (256 >> (10 - (uint8_t)(LCDTable->bitsPerPixel / 3)))) == 1)
			buffer1[curBuf] = buffer1[curBuf] - 1;
		buffer1[curBuf+1] = buffer1[curBuf] + 4;
		buffer2[curBuf] = buffer1[curBuf];
		buffer3[curBuf] = buffer1[curBuf];
		curBuf++;
	}
	buffer3[256] = buffer1[256];
	buffer2[256] = buffer1[256];
	if (LCDTableunkn1is0xA) {
		installGammaTables(panelID, 257, (uint32_t)buffer1, (uint32_t)buffer2, (uint32_t)buffer3);
		setWindowBuffer(4, buffer1);
		setWindowBuffer(5, buffer2);
		setWindowBuffer(6, buffer3);
		SET_REG(0x8920002C, 1);
	}

	free(buffer1);
	free(buffer2);
	free(buffer3);

	CurFramebuffer = currentWindow->framebuffer.buffer;

	return result;
}

static uint8_t PanelIDInfo[5];
static uint32_t DotPitch;
static uint8_t dword_5FF3AE0C;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow) {
	uint32_t pinot_default_color = 0;
	uint32_t pinot_backlight_cal = 0;
	uint32_t pinot_panel_id = 0;

	bufferPrintf("pinot_init()\r\n");
	DotPitch = LCDTable->DotPitch;
#if defined(CONFIG_IPAD_1G)
	gpio_pin_output(0x1404, 0);
#else
	gpio_pin_output(0x206, 0);
#endif
	task_sleep(10);
	mipi_dsim_init(LCDTable);
#if defined(CONFIG_IPAD_1G)
	gpio_pin_output(0x1404, 1);
#else
	gpio_pin_output(0x206, 1);
#endif
	task_sleep(6);
	mipi_dsim_write_data(5, 0, 0);
	udelay(10);
	mipi_dsim_framebuffer_on_off(ON);
	task_sleep(25);
//XXX	If there's no PanelID it tries to grab it from nvram entry "pinot-panel-id". We don't have nvram yet. -- Bluerise
	if (!pinot_panel_id) {
		uint32_t read_length;
		read_length = 15;
		PanelIDInfo[0] = 0xB1; // -79
		if (mipi_dsim_read_write(0x14, &PanelIDInfo[0], &read_length) || read_length <= 2) {
			bufferPrintf("pinot_init(): read of pinot panel id failed\r\n");
		} else {
			pinot_panel_id = (PanelIDInfo[0] << 24) | (PanelIDInfo[1] << 16) | ((PanelIDInfo[3] & 0xF0) << 8) | ((PanelIDInfo[2] & 0xF8) << 4) | ((PanelIDInfo[3] & 0xF) << 3) | (PanelIDInfo[2] & 0x7);
			pinot_default_color = (PanelIDInfo[3] & 0x8) ? 0x0 : 0xFFFFFF;
			pinot_backlight_cal = PanelIDInfo[5];
			dword_5FF3AE0C = PanelIDInfo[4];
		}
	}
	bufferPrintf("pinot_init(): pinot_panel_id:      0x%08lx\r\n", pinot_panel_id);
	bufferPrintf("pinot_init(): pinot_default_color: 0x%08lx\r\n", pinot_default_color);
	bufferPrintf("pinot_init(): pinot_backlight_cal: 0x%08lx\r\n", pinot_backlight_cal);
	lcd_fill_switch(ON, pinot_default_color);
	udelay(100);
	if (!pinot_panel_id) {
		lcd_fill_switch(OFF, 0);
		mipi_dsim_quiesce();
		return -1;
	}

	mipi_dsim_write_data(5, 0x11, 0);
	displaytime_sleep(7);
	mipi_dsim_write_data(5, 0x29, 0);
	displaytime_sleep(7);
#if defined(CONFIG_IPHONE_4)
	gpio_switch(0x207, OFF);
#endif

	*panelID = pinot_panel_id;

#if !defined(CONFIG_IPAD_1G)
	if (!dword_5FF3AE0C)
		return dword_5FF3AE0C;

	sub_5FF08870(dword_5FF3AE0C);
#endif
	return 0;
}

void pinot_quiesce() {
	bufferPrintf("pinot_quiesce()\r\n");
	mipi_dsim_write_data(5, 0x28, 0);
	mipi_dsim_write_data(5, 0x10, 0);
	displaytime_sleep(6);
	mipi_dsim_framebuffer_on_off(OFF);
	mipi_dsim_quiesce();
	gpio_pin_output(0x206, 0);
	return;
}

static uint8_t installGammaTableHelper(uint8_t* table) {
	if(gammaVar2 == 0) {
		gammaVar3 = table[gammaVar1++];
	}

	int toRet = (gammaVar3 >> gammaVar2) & 0x3;
	gammaVar2 += 2;

	if(gammaVar2 == 8)
		gammaVar2 = 0;

	return toRet;
}

static void installGammaTable(int maxi, uint32_t curReg, uint8_t* table) {
        gammaVar1 = 0;
        gammaVar2 = 0;
        gammaVar3 = 0;

        uint8_t r4 = 0;
        uint16_t r6 = 0;
        uint8_t r8 = 0;

        SET_REG(curReg, 0x0);

        curReg += 4;

	int i;
	for (i = 1; i < maxi; i++) {
                switch(installGammaTableHelper(table)) {
                        case 0:
                                r4 = 0;
                                break;
                        case 1:
                                r4 = 1;
                                break;
                        case 2:
                                {
                                        uint8_t a = installGammaTableHelper(table);
                                        uint8_t b = installGammaTableHelper(table);
                                        r4 = a | (b << 2);
                                        if((r4 & (1 << 3)) != 0) {
                                                r4 |= 0xF0;
                                        }
                                }
                                break;
                        case 3:
                                r4 = 0xFF;
                                break;
                }

                if(i == 1) {
                        r4 = r4 + 8;
                }

                r8 += r4;
                r6 += r8;

                SET_REG(curReg, (uint32_t)r6);

                curReg += 4;
        }
}

void installGammaTables(int panelID, int maxi, uint32_t buffer1, uint32_t buffer2, uint32_t buffer3) {
	GammaTableDescriptor* curTable = &PinotGammaTables[0];

	int i;
	for(i = 0; i < (sizeof(PinotGammaTables)/sizeof(GammaTableDescriptor)); i++) {
		if((curTable->panelIDMask & panelID) == curTable->panelIDMatch) {
			bufferPrintf("Found Gamma table 0x%08lx / 0x%08lx\r\n", curTable->panelIDMatch, curTable->panelIDMask);
			installGammaTable(maxi, buffer1, (uint8_t*) curTable->table0.data);
			installGammaTable(maxi, buffer2, (uint8_t*) curTable->table1.data);
			installGammaTable(maxi, buffer3, (uint8_t*) curTable->table2.data);
			return;
		}
		curTable++;
	}
	bufferPrintf("No Gamma table found for display_id: 0x%08lx\r\n", panelID);
}

void setWindowBuffer(int window, uint32_t* buffer) {
	uint32_t size;
	uint32_t cur_size;
	uint32_t to_set;
	uint32_t window_bit = window << 12;
	if (window == 7)
		size = 225;
	else
		size = 256;

	SET_REG(0x89200034, window_bit | 0x10000);

	for (cur_size = 0; cur_size < size; cur_size++) {
		to_set = *buffer;
		if ((window - 4) <= 2)
			to_set = (*(buffer+1) & 0x3FF) | ((*buffer << 10) & 0xFFC00);
		SET_REG(0x89200038, to_set | (1 << 31));
		buffer++;
	}

	SET_REG(0x89200034, 0);
}

void framebuffer_fill(Framebuffer* framebuffer, int x, int y, int width, int height, int fill) {
	if(x >= framebuffer->width)
		return;

	if(y >= framebuffer->height)
		return;

	if((x + width) > framebuffer->width) {
		width = framebuffer->width - x;
	}

	int maxLine;
	if((y + height) > framebuffer->height) {
		maxLine = framebuffer->height;
	} else {
		maxLine = y + height;
	}

	int line;
	for(line = y; line < maxLine; line++) {
		framebuffer->hline(framebuffer, x, line, width, fill);
	}
}

static void hline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint32_t* line;

	fill = fill & 0xffffff;	// no alpha
	line = &framebuffer->buffer[line_no * framebuffer->lineWidth];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i] = fill;
	}
}

static void vline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint32_t* line;

	fill = fill & 0xffffff;	// no alpha
	line = &framebuffer->buffer[line_no];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i * framebuffer->lineWidth] = fill;
	}
}

static void hline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint16_t* line;
	uint16_t fill565;

	fill565= ((((fill >> 16) & 0xFF) >> 3) << 11) | ((((fill >> 8) & 0xFF) >> 2) << 5) | ((fill & 0xFF) >> 3);
	line = &((uint16_t*)framebuffer->buffer)[line_no * framebuffer->lineWidth];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i] = fill565;
	}
}

static void vline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint16_t* line;
	uint16_t fill565;

	fill565= ((((fill >> 16) & 0xFF) >> 3) << 11) | ((((fill >> 8) & 0xFF) >> 2) << 5) | ((fill & 0xFF) >> 3);
	line = &((uint16_t*)framebuffer->buffer)[line_no];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i * framebuffer->lineWidth] = fill565;
	}
}

static Window* createWindow(int zero0, int zero2, int width, int height, ColorSpace colorSpace) {
	uint32_t bitsPerPixel;
	uint32_t reg_bit;

	switch (colorSpace) {
		default:
		case RGB888:
			bitsPerPixel = 32;
			reg_bit = 0;
			break;
		case RGB565:
			bitsPerPixel = 16;
			reg_bit = 4;
			break;
	}

	Window* newWindow;
	newWindow = (Window*) malloc(sizeof(Window));
	newWindow->created = FALSE;
	newWindow->width = width;
	newWindow->height = height;
	newWindow->lineBytes = width * (bitsPerPixel / 8);

	createFramebuffer(&newWindow->framebuffer, (uint32_t)FRAMEBUFFER, width, height, width, colorSpace);

	SET_REG(CLCD + 0x4040, (reg_bit << 8) | 1);
	SET_REG(CLCD + 0x4044, (uint32_t)newWindow->framebuffer.buffer);
	SET_REG(CLCD + 0x4048, (newWindow->lineBytes & 0xFFFFFFC0) | 2);
	SET_REG(CLCD + 0x4050, 0);
	SET_REG(CLCD + 0x4060, (width << 16) | height);
	SET_REG(CLCD + 0x2040, 0xFFFF0202);
	SET_REG(CLCD + 0x404C, 1);
	SET_REG(CLCD + 0x4074, 0x200060);
	SET_REG(CLCD + 0x4078, 32);

	SET_REG(CLCD + 0x1038, GET_REG(CLCD + 0x1038) | 0x100);
	newWindow->created = TRUE;

	return newWindow;
}

static void createFramebuffer(Framebuffer* framebuffer, uint32_t framebufferAddr, int width, int height, int lineWidth, ColorSpace colorSpace) {
	framebuffer->buffer = (uint32_t*) framebufferAddr;
	framebuffer->width = width;
	framebuffer->height = height;
	framebuffer->lineWidth = lineWidth;
	framebuffer->colorSpace = colorSpace;
	if(colorSpace == RGB888)
	{
		framebuffer->hline = hline_rgb888;
		framebuffer->vline = vline_rgb888;
	} else
	{
		framebuffer->hline = hline_rgb565;
		framebuffer->vline = vline_rgb565;
	}
}

#if !defined(CONFIG_IPAD_1G)
void lcd_set_backlight_level(int level) {
	if (level == 0) {
		pmu_write_regs(&backlightOffData, 1);
	} else { 
		PMURegisterData myBacklightData[sizeof(backlightData)/sizeof(PMURegisterData)];

		memcpy(myBacklightData, backlightData, sizeof(myBacklightData));

		if (level <= LCD_MAX_BACKLIGHT) {
			int i;
			for (i = 0; i < (sizeof(myBacklightData)/sizeof(PMURegisterData)); i++) {
				if (myBacklightData[i].reg == LCD_BACKLIGHT_HIGH_REG) {
					myBacklightData[i].data = level >> LCD_BACKLIGHT_HIGH_SHIFT;
				} else if (myBacklightData[i].reg == LCD_BACKLIGHT_LOW_REG) {
					myBacklightData[i].data = level & LCD_BACKLIGHT_LOW_MASK;
				}
			}
		}
		pmu_write_regs(myBacklightData, sizeof(myBacklightData)/sizeof(PMURegisterData));
	}
}
#else
void lcd_set_backlight_level(int level) {
	uint32_t _arg = 0x1000;
	clock_gate_switch(0x3E, ON);
	SET_REG(0xBF600000, (((((((((uint64_t)TicksPerSec+1999999)*(uint64_t)0x431BDE83) >> 32) >> 19) - 1) & 0xFF) << 8) | 0x3));
	SET_REG(0xBF600018, ((_arg | 0x80) | (level & 0x7F )) | ((level & 0x780) << 1));
	SET_REG(0xBF600014, 3);
	while(!(GET_REG(0xBF600014) & 1));
	if(level)
		gpio_pin_output(0x1403, 1);
	else
		gpio_pin_output(0x1403, 0);
}
#endif

void cmd_backlight(int argc, char** argv) {
	if(argc < 2) {
		bufferPrintf("Usage: %s <0-%d>\r\n", argv[0], LCD_MAX_BACKLIGHT);
		return;
	}

	uint32_t level = parseNumber(argv[1]);
	lcd_set_backlight_level(level);
	bufferPrintf("backlight set to %d\r\n", level);
}
COMMAND("backlight", "set the backlight level", cmd_backlight);
