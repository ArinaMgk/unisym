// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Build Options
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_USB_Options
#define _INCPP_Device_USB_Options

// Enable xHCI bulk streams, stream contexts, and per-stream transfer rings.
#ifndef _OPT_BUS_USB_xHCI_Streams
#define _OPT_BUS_USB_xHCI_Streams 0
#endif

// Enable xHCI isochronous transfers and ESIT scheduling.
#ifndef _OPT_BUS_USB_xHCI_Isochronous
#define _OPT_BUS_USB_xHCI_Isochronous 0
#endif

// Enable xHCI U1/U2/U3, LTM, suspend, resume, and remote-wakeup policy.
#ifndef _OPT_BUS_USB_xHCI_LinkPower
#define _OPT_BUS_USB_xHCI_LinkPower 0
#endif

// Enable multiple xHCI interrupters, MSI-X routing, and additional event rings.
#ifndef _OPT_BUS_USB_xHCI_MultiInterrupter
#define _OPT_BUS_USB_xHCI_MultiInterrupter 0
#endif

// Enable USB 3.2 lane counts, asymmetric rates, and aggregate link bandwidth.
#ifndef _OPT_BUS_USB_xHCI_USB3v2_Lanes
#define _OPT_BUS_USB_xHCI_USB3v2_Lanes 0
#endif

// Enable BOS requests and USB device capability parsing.
#ifndef _OPT_BUS_USB_BOS
#define _OPT_BUS_USB_BOS 1
#endif

// Enable USB hub class enumeration and downstream-port management.
#ifndef _OPT_BUS_USB_Hub
#define _OPT_BUS_USB_Hub 1
#endif

// Enable CLEAR_TT_BUFFER recovery for full/low-speed devices behind high-speed hubs.
#ifndef _OPT_BUS_USB_TTRecovery
#define _OPT_BUS_USB_TTRecovery 1
#endif

#endif
