/*
 * flaucheky.c -- the USB Mass Storage Class device
 *
 * Copyright (C) 2013, 2015, 2016  Free Software Initiative of Japan
 * Author: NIIBE Yutaka <gniibe@fsij.org>
 *
 * This file is a part of Fraucheky, making sure to have GNU GPL on a
 * USB thumb drive
 *
 * Fraucheky is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * Fraucheky is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include "usb_lld.h"
#include "config.h"

/* MSC BULK_IN, BULK_OUT */
/* EP6: 64-byte, 64-byte */
#define ENDP6_TXADDR        (0x180)
#define ENDP6_RXADDR        (0x1c0)

#define USB_INITIAL_FEATURE 0x80   /* bmAttributes: bus powered */

#define MSC_GET_MAX_LUN_COMMAND        0xFE
#define MSC_MASS_STORAGE_RESET_COMMAND 0xFF

/* USB Standard Device Descriptor */
static const uint8_t device_desc[] = {
  18,   /* bLength */
  DEVICE_DESCRIPTOR,     /* bDescriptorType */
  0x10, 0x01,   /* bcdUSB = 1.1 */
  0x00,   /* bDeviceClass: 0 means deferred to interface */
  0x00,   /* bDeviceSubClass */
  0x00,   /* bDeviceProtocol */
  0x40,   /* bMaxPacketSize0 */
#include "fraucheky-vid-pid-ver.c.inc"
  1, /* Index of string descriptor describing manufacturer */
  2, /* Index of string descriptor describing product */
  3, /* Index of string descriptor describing the device's serial number */
  1  /* bNumConfigurations */
};

#define MSC_TOTAL_LENGTH (9+7+7)

/* Configuation Descriptor */
static const uint8_t config_desc[] = {
  9,			         /* bLength: Configuation Descriptor size */
  CONFIG_DESCRIPTOR,             /* bDescriptorType: Configuration */
  (9+9+7+7), 0x00,               /* wTotalLength:no of returned bytes */
  1,				 /* bNumInterfaces: */
  0x01,                          /* bConfigurationValue: Configuration value */
  0x00,				 /* iConfiguration.  */
  USB_INITIAL_FEATURE,		 /* bmAttributes*/
  50,				 /* MaxPower 100 mA */

  /* Interface Descriptor.*/
  9,			         /* bLength: Interface Descriptor size */
  INTERFACE_DESCRIPTOR,          /* bDescriptorType: Interface         */
  0,				 /* bInterfaceNumber.                  */
  0x00,				 /* bAlternateSetting.                 */
  0x02,				 /* bNumEndpoints.                     */
  0x08,				 /* bInterfaceClass (Mass Stprage).    */
  0x06,				 /* bInterfaceSubClass (SCSI
				    transparent command set, MSCO
				    chapter 2).                        */
  0x50,				 /* bInterfaceProtocol (Bulk-Only
				    Mass Storage, MSCO chapter 3).     */
  0x00,				 /* iInterface.                        */
  /* Endpoint Descriptor.*/
  7,			         /* bLength: Endpoint Descriptor size  */
  ENDPOINT_DESCRIPTOR,   	 /* bDescriptorType: Endpoint          */
  0x86,				 /* bEndpointAddress: (IN6)            */
  0x02,				 /* bmAttributes (Bulk).               */
  0x40, 0x00,			 /* wMaxPacketSize.                    */
  0x00,				 /* bInterval (ignored for bulk).      */
  /* Endpoint Descriptor.*/
  7,			         /* bLength: Endpoint Descriptor size  */
  ENDPOINT_DESCRIPTOR,   	 /* bDescriptorType: Endpoint          */
  0x06,				 /* bEndpointAddress: (OUT6)           */
  0x02,				 /* bmAttributes (Bulk).               */
  0x40, 0x00,			 /* wMaxPacketSize.                    */
  0x00,				 /* bInterval (ignored for bulk).      */
};


/* USB String Descriptors */
static const uint8_t string_lang_id[] = {
  4,				/* bLength */
  STRING_DESCRIPTOR,
  0x09, 0x04			/* LangID = 0x0409: US-English */
};

#include "fraucheky-usb-strings.c.inc"

static const uint8_t string_serial[] = {
  8*2+2,			/* bLength */
  STRING_DESCRIPTOR,    	/* bDescriptorType */
  /* Serial number: "FSIJ-0.0" */
  'F', 0, 'S', 0, 'I', 0, 'J', 0, '-', 0, '0', 0, '.', 0, '0', 0,
};


struct desc { const uint8_t *desc; uint16_t size; };

static const struct desc string_descriptors[] = {
  {string_lang_id, sizeof (string_lang_id)},
  {string_vendor, sizeof (string_vendor)},
  {string_product, sizeof (string_product)},
  {string_serial, sizeof (string_serial)},
};

void
fraucheky_setup_endpoints_for_interface (int stop)
{
  if (!stop)
    usb_lld_setup_endpoint (ENDP6, EP_BULK, 0, ENDP6_RXADDR, ENDP6_TXADDR, 64);
  else
    {
      usb_lld_stall_tx (ENDP6);
      usb_lld_stall_rx (ENDP6);
    }
}

int
fraucheky_setup (uint8_t req, uint8_t req_no, struct req_args *arg)
{
  static const uint8_t lun_table[] = { 0, 0, 0, 0, };

  if (USB_SETUP_GET (req))
    {
      if (req_no == MSC_GET_MAX_LUN_COMMAND)
	return usb_lld_reply_request (lun_table, sizeof (lun_table), arg);
    }
  else
    if (req_no == MSC_MASS_STORAGE_RESET_COMMAND)
      /* Should call resetting MSC thread, something like msc_reset () */
      return USB_SUCCESS;

  return USB_UNSUPPORT;
}

int
fraucheky_get_descriptor (uint8_t rcp, uint8_t desc_type, uint8_t desc_index,
			  struct req_args *arg)
{
  if (rcp == DEVICE_RECIPIENT)
    {
      if (desc_type == DEVICE_DESCRIPTOR && arg->index == 0)
	return usb_lld_reply_request (device_desc, sizeof (device_desc), arg);
      else if (desc_type == CONFIG_DESCRIPTOR && arg->index == 0)
	return usb_lld_reply_request (config_desc, sizeof (config_desc), arg);
      else if (desc_type == STRING_DESCRIPTOR)
	{
	  if (desc_index >= sizeof (string_descriptors) / sizeof (struct desc)
	      || !((arg->index == 0 && desc_index == 0)
		   || arg->index == 0x0409))
	    /* We only provide string in English.  */
	    return USB_UNSUPPORT;

	  return usb_lld_reply_request (string_descriptors[desc_index].desc,
					string_descriptors[desc_index].size,
					arg);
	}
    }

  return USB_UNSUPPORT;
}
