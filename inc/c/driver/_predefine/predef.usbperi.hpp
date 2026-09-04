
// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: [Device.USB] Peripheral Stack Predefine
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#ifndef _INC_DEV_PREDEF_USBPERI
#define _INC_DEV_PREDEF_USBPERI

#if defined(_MCU_STM32H7x)

// ---- USB standard request / descriptor type codes ----
#define _USB_REQ_TYPE_STANDARD          0x00
#define _USB_REQ_TYPE_CLASS             0x20
#define _USB_REQ_TYPE_VENDOR            0x40
#define _USB_REQ_TYPE_MASK              0x60

#define _USB_REQ_RECIPIENT_DEVICE       0x00
#define _USB_REQ_RECIPIENT_INTERFACE    0x01
#define _USB_REQ_RECIPIENT_ENDPOINT     0x02
#define _USB_REQ_RECIPIENT_MASK         0x03

#define _USB_REQ_GET_STATUS             0x00
#define _USB_REQ_CLEAR_FEATURE          0x01
#define _USB_REQ_SET_FEATURE            0x03
#define _USB_REQ_SET_ADDRESS            0x05
#define _USB_REQ_GET_DESCRIPTOR         0x06
#define _USB_REQ_SET_DESCRIPTOR         0x07
#define _USB_REQ_GET_CONFIGURATION      0x08
#define _USB_REQ_SET_CONFIGURATION      0x09
#define _USB_REQ_GET_INTERFACE          0x0A
#define _USB_REQ_SET_INTERFACE          0x0B

#define _USB_DESC_TYPE_DEVICE           1
#define _USB_DESC_TYPE_CONFIGURATION    2
#define _USB_DESC_TYPE_STRING           3
#define _USB_DESC_TYPE_INTERFACE        4
#define _USB_DESC_TYPE_ENDPOINT         5
#define _USB_DESC_TYPE_DEVICE_QUALIFIER 6
#define _USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION 7

#define _USB_FEATURE_EP_HALT            0
#define _USB_FEATURE_REMOTE_WAKEUP      1

#define _USB_MAX_EP0_SIZE               64

// string descriptor indexes (AKA USBD_IDX_*)
#define _USBD_IDX_LANGID_STR            0x00
#define _USBD_IDX_MFC_STR               0x01
#define _USBD_IDX_PRODUCT_STR           0x02
#define _USBD_IDX_SERIAL_STR            0x03
#define _USBD_IDX_CONFIG_STR            0x04
#define _USBD_IDX_INTERFACE_STR         0x05

#endif // _MCU_STM32H7x

#endif // _INC_DEV_PREDEF_USBPERI
