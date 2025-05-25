

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(xinput);

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <usb_descriptor.h>

#include "xinput.h"

/* Max packet size for Bulk endpoints */
#if defined(CONFIG_USB_DC_HAS_HS_SUPPORT)
#define XINPUT_BULK_EP_MPS		512
#else
#define XINPUT_BULK_EP_MPS		32
#endif

/* Number of interfaces */
#define XINPUT_NUM_ITF			0x01
/* Number of Endpoints in the custom interface */
#define XINPUT_NUM_EP			0x02

#define XINPUT_IN_EP_IDX		0
#define XINPUT_OUT_EP_IDX		1

static struct xinput_req_handlers *req_handlers;
static struct xinput_in_struct{
	uint8_t report_id;
	uint8_t report_size;
	uint8_t buttons_0;
	uint8_t buttons_1;
	uint8_t lz;
	uint8_t rz;
	int16_t lx;
	int16_t ly;
	int16_t rx;
	int16_t ry;
	uint8_t reserved[6];
} __packed xinput_data = {
	.report_id   = 0,
	.report_size = 20,
	.buttons_0   = 0,
	.buttons_1   = 0,
	.lz          = 0,
	.rz          = 0,
	.lx          = 0,
	.ly          = 0,
	.rx          = 0,
	.ry          = 0,
	.reserved    = {0, 0, 0, 0, 0, 0}
};


uint8_t rx_buf[XINPUT_BULK_EP_MPS];

#define INITIALIZER_IF(num_ep)				\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = num_ep,				\
		.bInterfaceClass = 0xFF,				\
		.bInterfaceSubClass = 0x5D,				\
		.bInterfaceProtocol = 0x01,				\
		.iInterface = 0,					\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, interval)			\
	{								\
		.bLength = 0x07,		\
		.bDescriptorType = USB_DESC_ENDPOINT,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = interval,					\
	}

//0x10, 0x21, 0x10, 0x01, 0x01, 0x24, 0x81, 0x14, 0x03, 0x00, 0x03, 0x13, 0x02, 0x00, 0x03, 0x00,
USBD_CLASS_DESCR_DEFINE(primary, 0) struct {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct xbox_undefined_descriptor vndr;
	struct usb_ep_descriptor if0_out_ep;
	
} __packed xinput_desc = {
	.if0 = INITIALIZER_IF(0x02),
	.vndr = {
		.bLength = 0x10,
		.bDescriptorType = 0x21,
		.f1 = 0x10,
		.f2 = 0x01,
		.f3 = 0x01,
		.f4 = 0x24,
		.f5 = 0x81,
		.f6 = 0x14,
		.f7 = 0x03,
		.f8 = 0x00,
		.f9 = 0x03,
		.f10 = 0x13,
		.f11 = 0x01,
		.f12 = 0x00,
		.f13 = 0x03,
		.f14 = 0x00
	},
	.if0_in_ep = INITIALIZER_IF_EP(0x81, USB_DC_EP_INTERRUPT,
				       32, 0x04),
	.if0_out_ep = INITIALIZER_IF_EP(0x01, USB_DC_EP_INTERRUPT,
					32, 0x08),

};

/**
 * @brief Custom handler for standard requests in order to
 *        catch the request and return the Xinput Platform
 *        Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int xinput_custom_handle_req(struct usb_setup_packet *pSetup,
			     int32_t *len, uint8_t **data)
{
	LOG_DBG("");

	/* Call the callback */
	if (req_handlers && req_handlers->custom_handler) {
		return req_handlers->custom_handler(pSetup, len, data);
	}

	return -EINVAL;
}

/**
 * @brief Handler called for Xinput vendor specific commands.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int xinput_vendor_handle_req(struct usb_setup_packet *pSetup,
			     int32_t *len, uint8_t **data)
{
	/* Call the callback */
	if (req_handlers && req_handlers->vendor_handler) {
		return req_handlers->vendor_handler(pSetup, len, data);
	}

	return -EINVAL;
}

/**
 * @brief Register Custom and Vendor request callbacks
 *
 * This function registers Custom and Vendor request callbacks
 * for handling the device requests.
 *
 * @param [in] handlers Pointer to Xinput request handlers structure
 */
void xinput_register_request_handlers(struct xinput_req_handlers *handlers)
{
	req_handlers = handlers;
}

void xinput_out_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	LOG_DBG("Out");
}

static int ctr=0;
 void xinput_in_cb(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	if(ctr==50){
		xinput_data.buttons_1 = 0b00010000;
		xinput_data.lx = -32000;
		xinput_data.ry = -32000;
		xinput_data.lz=255;
	}
	if(ctr==100){
		xinput_data.buttons_1 = 0x00;
		xinput_data.lx = 32000;
		xinput_data.ry = 32000;
		xinput_data.lz=0;
		ctr=0;
	}
	ctr+=1;
	if (usb_write(ep, (const uint8_t *)&xinput_data, sizeof xinput_data, NULL)) {
		LOG_INF("data IN success");
	}
}
/* Describe EndPoints configuration */
static struct usb_ep_cfg_data xinput_ep_data[] = {
	{
		.ep_cb = xinput_in_cb,
		.ep_addr = 0x81
	},
	{
		.ep_cb	= xinput_out_cb,
		.ep_addr = 0x01
	}
};

/**
 * @brief Callback used to know the USB connection status
 *
 * @param status USB device status code.
 */
static void xinput_dev_status_cb(struct usb_cfg_data *cfg,
				 enum usb_dc_status_code status,
				 const uint8_t *param)
{
	ARG_UNUSED(param);
	ARG_UNUSED(cfg);

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_ERROR:
		LOG_DBG("USB device error");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB device reset detected");
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB device connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_INF("USB device configured");
		xinput_in_cb(xinput_ep_data[0].ep_addr, 0);
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB device suspended");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB device resumed");
		break;
	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state");
		break;
	}
}





USBD_DEFINE_CFG_DATA(xinput_config) = {
	.usb_device_description = NULL,
	.interface_descriptor = &xinput_desc.if0,
	.cb_usb_status = xinput_dev_status_cb,
	.interface = {
		.class_handler = NULL,
		.custom_handler = xinput_custom_handle_req,
		.vendor_handler = xinput_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(xinput_ep_data),
	.endpoint = xinput_ep_data
};
