/* USB Standard Device Descriptor */
static const uint8_t device_desc[] = {
  18,   /* bLength */
  USB_DEVICE_DESCRIPTOR_TYPE,     /* bDescriptorType */
  0x10, 0x01,   /* bcdUSB = 1.1 */
  0x00,   /* bDeviceClass: 0 means deferred to interface */
  0x00,   /* bDeviceSubClass */
  0x00,   /* bDeviceProtocol */
  0x40,   /* bMaxPacketSize0 */
  0x4b, 0x23, /* idVendor  */
  0x04, 0x00, /* idProduct */
  0x00, 0x01, /* bcdDevice  */
  1, /* Index of string descriptor describing manufacturer */
  2, /* Index of string descriptor describing product */
  3, /* Index of string descriptor describing the device's serial number */
  0x01    /* bNumConfigurations */
};

#define MSC_TOTAL_LENGTH (9+7+7)

/* Configuation Descriptor */
static const uint8_t config_desc[] = {
  9,			         /* bLength: Configuation Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,    /* bDescriptorType: Configuration */
  (9+7+7), 0x00,                 /* wTotalLength:no of returned bytes */
  1,				 /* bNumInterfaces: */
  0x01,                          /* bConfigurationValue: Configuration value */
  0x00,				 /* iConfiguration.  */
  USB_INITIAL_FEATURE,		 /* bmAttributes*/
  50,				 /* MaxPower 100 mA */

  /* Interface Descriptor.*/
  9,			         /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE, /* bDescriptorType: Interface */
  MSC_INTERFACE_NO,		 /* bInterfaceNumber.                */
  0x00,				 /* bAlternateSetting.               */
  0x02,				 /* bNumEndpoints.                   */
  0x08,				 /* bInterfaceClass (Mass Stprage).  */
  0x06,				 /* bInterfaceSubClass (SCSI
				    transparent command set, MSCO
				    chapter 2).                      */
  0x50,				 /* bInterfaceProtocol (Bulk-Only
				    Mass Storage, MSCO chapter 3).  */
  0x00,				 /* iInterface.                      */
  /* Endpoint Descriptor.*/
  7,			         /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,	 /* bDescriptorType: Endpoint */
  0x86,				 /* bEndpointAddress: (IN6)   */
  0x02,				 /* bmAttributes (Bulk).             */
  0x40, 0x00,			 /* wMaxPacketSize.                  */
  0x00,				 /* bInterval (ignored for bulk).    */
  /* Endpoint Descriptor.*/
  7,			         /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,	 /* bDescriptorType: Endpoint */
  0x06,				 /* bEndpointAddress: (OUT6)    */
  0x02,				 /* bmAttributes (Bulk).             */
  0x40, 0x00,			 /* wMaxPacketSize.                  */
  0x00,         /* bInterval (ignored for bulk).    */
};


/* USB String Descriptors */
static const uint8_t string_lang_id[] = {
  4,				/* bLength */
  USB_STRING_DESCRIPTOR_TYPE,
  0x09, 0x04			/* LangID = 0x0409: US-English */
};

static const uint8_t string_vendor[] = {
  33*2+2,			/* bLength */
  USB_STRING_DESCRIPTOR_TYPE,	/* bDescriptorType */
  /* Manufacturer: "Free Software Initiative of Japan" */
  'F', 0, 'r', 0, 'e', 0, 'e', 0, ' ', 0, 'S', 0, 'o', 0, 'f', 0,
  't', 0, 'w', 0, 'a', 0, 'r', 0, 'e', 0, ' ', 0, 'I', 0, 'n', 0,
  'i', 0, 't', 0, 'i', 0, 'a', 0, 't', 0, 'i', 0, 'v', 0, 'e', 0,
  ' ', 0, 'o', 0, 'f', 0, ' ', 0, 'J', 0, 'a', 0, 'p', 0, 'a', 0,
  'n', 0,
};

static const uint8_t string_product[] = {
  9*2+2,			/* bLength */
  USB_STRING_DESCRIPTOR_TYPE,	/* bDescriptorType */
  /* Product name: "Fraucheky" */
  'F', 0, 'r', 0, 'a', 0, 'u', 0, 'c', 0, 'h', 0, 'e', 0, 'k', 0,
  'y', 0,
};

static const uint8_t string_serial[] = {
  8*2+2,			/* bLength */
  USB_STRING_DESCRIPTOR_TYPE,	/* bDescriptorType */
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
fraucheky_setup (uint8_t req, uint8_t req_no, uint16_t value, uint16_t len)
{
  static const uint8_t lun_table[] = { 0, 0, 0, 0, };

  (void)value;
  (void)len;
  if (USB_SETUP_GET (req))
    {
      if (req_no == MSC_GET_MAX_LUN_COMMAND)
	{
	  usb_lld_set_data_to_send (lun_table, sizeof (lun_table));
	  return USB_SUCCESS;
	}
    }
  else
    if (req_no == MSC_MASS_STORAGE_RESET_COMMAND)
      /* Should call resetting MSC thread, something like msc_reset () */
      return USB_SUCCESS;
}

int
fraucheky_get_descriptor (uint8_t rcp, uint8_t desc_type, uint8_t desc_index,
			  uint16_t index)
{
  if (rcp == DEVICE_RECIPIENT && index == 0)
    {
      if (desc_type == DEVICE_DESCRIPTOR)
	{
	  usb_lld_set_data_to_send (device_desc, sizeof (device_desc));
	  return USB_SUCCESS;
	}
      else if (desc_type == CONFIG_DESCRIPTOR)
	{
	  usb_lld_set_data_to_send (config_desc, sizeof (config_desc));
	  return USB_SUCCESS;
	}
      else if (desc_type == STRING_DESCRIPTOR)
	{
	  if (desc_index < sizeof (string_descriptors) / sizeof (struct desc))
	    {
	      usb_lld_set_data_to_send (string_descriptors[desc_index].desc,
					string_descriptors[desc_index].size);
	      return USB_SUCCESS;
	    }
	}
    }

  return USB_UNSUPPORT;
}
