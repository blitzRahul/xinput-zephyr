/*
 * Copyright (c) 2016-2019 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for WebUSB enabled custom class driver.
 *
 * Sample app for WebUSB enabled custom class driver. The received
 * data is echoed back to the WebUSB based application running in
 * the browser at host.
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/msos_desc.h>

#include "webusb.h"




/* Predefined response to control commands related to MS OS 1.0 descriptors
 * Please note that this code only defines "extended compat ID OS feature
 * descriptors" and not "extended properties OS features descriptors"
 */
#define MSOS_STRING_LENGTH	18
static struct string_desc {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bString[MSOS_STRING_LENGTH];

} __packed msos1_string_descriptor = {
	.bLength = MSOS_STRING_LENGTH,
	.bDescriptorType = USB_DESC_STRING,
	/* Signature MSFT100 */
	.bString = {
		'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,
		'1', 0x00, '0', 0x00, '0', 0x00,
		0x17, /* Vendor Code, used for a control request */
		0x00, /* Padding byte for VendorCode looks like UTF16 */
	},
};

static const uint8_t msos1_compatid_descriptor[] = {
	/* See https://github.com/pbatard/libwdi/wiki/WCID-Devices */
	/* MS OS 1.0 header section */
	0x28, 0x00, 0x00, 0x00, /* Descriptor size (40 bytes)          */
	0x00, 0x01,             /* Version 1.00                        */
	0x04, 0x00,             /* Type: Extended compat ID descriptor */
	0x01,                   /* Number of function sections         */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       /* reserved    */

	/* MS OS 1.0 function section */
	0x02,     /* Index of interface this section applies to. */
	0x01,     /* reserved */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'X',  'U',  'S',  'B',  '1',  '0',  0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* reserved */
};


/**
 * @brief Custom handler for standard requests in
 *        order to catch the request and return the
 *        WebUSB Platform Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail
 */
int custom_handle_req(struct usb_setup_packet *pSetup,
		      int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(pSetup)) {
		LOG_ERR("reqtype to host not supported");
		return -ENOTSUP;
	}

	if (USB_GET_DESCRIPTOR_TYPE(pSetup->wValue) == USB_DESC_STRING &&
	    USB_GET_DESCRIPTOR_INDEX(pSetup->wValue) == 0xEE) {
		*data = (uint8_t *)(&msos1_string_descriptor);
		*len = sizeof(msos1_string_descriptor);

		LOG_DBG("Get MS OS Descriptor v1 string");

		return 0;
	}

	LOG_ERR("Returning -EINVAL");
	return -EINVAL;
}

/**
 * @brief Handler called for vendor specific commands. This includes
 *        WebUSB allowed origins and MS OS 1.0 and 2.0 descriptors.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int vendor_handle_req(struct usb_setup_packet *pSetup,
		      int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(pSetup)) {
		return -ENOTSUP;
	}

	 if (pSetup->bRequest == 0x17 && pSetup->wIndex == 0x04) {
		/* Get MS OS 1.0 Descriptors request */
		/* 0x04 means "Extended compat ID".
		 * Use 0x05 instead for "Extended properties".
		 */
		*data = (uint8_t *)(&msos1_compatid_descriptor);
		*len = sizeof(msos1_compatid_descriptor);

		LOG_DBG("Get MS OS Descriptors CompatibleID");

		return 0;
	}

	return -ENOTSUP;
}

/* Custom and Vendor request handlers */
static struct webusb_req_handlers req_handlers = {
	.custom_handler = custom_handle_req,
	.vendor_handler = vendor_handle_req,
};

int main(void)
{
	int ret;

	LOG_DBG("");


	/* Set the custom and vendor request handlers */
	webusb_register_request_handlers(&req_handlers);

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}
	return 0;
}
