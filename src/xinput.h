

#ifndef __XINPUT_H__
#define __XINPUT_H__



#define MS_OS_DESCRIPTOR \
    0x12,                    /* Length */\
    0x03,                    /* Descriptor type: string */\
    'M' , 0x00, 'S' , 0x00,  /* Signature */\
    'F' , 0x00, 'T' , 0x00,  \
    '1' , 0x00, '0' , 0x00,  \
    '0' , 0x00,              \
    0x17,            /* Vendor code */\
    0x00                     /* Padding */

#define MS_OS_COMPATIDS(len, itfs) \
    len , 0x00, 0x00, 0x00,  /* Length */\
    0x00, 0x01,              /* Version */\
    0x04, 0x00,              /* Descriptor type: Compatibility ID */\
    itfs, 0x00,              /* Sections */\
    0x00, 0x00, 0x00, 0x00,  /* Reserved */\
    0x00, 0x00               /* Reserved */\

#define MS_OS_COMPATIDS_WINUSB \
    ITF_WEBUSB,              /* Interface index */\
    0x01,                    /* Reserved */\
    'W' , 'I' , 'N' , 'U' ,  /* Compat ID */\
    'S' , 'B' , 0x00, 0x00,  \
    0x00, 0x00, 0x00, 0x00,  /* Sub-compat ID. */\
    0x00, 0x00, 0x00, 0x00,  \
    0x00, 0x00, 0x00, 0x00,  /* Reserved */\
    0x00, 0x00               /* Reserved */\

#define MS_OS_COMPATIDS_XUSB \
    ITF_XINPUT,              /* Interface index */\
    0x01,                    /* Reserved */\
    'X' , 'U' , 'S' , 'B',   /* Compat ID */\
    '1' , '0' , 0x00, 0x00,  \
    0x00, 0x00, 0x00, 0x00,  /* Sub-compat ID. */\
    0x00, 0x00, 0x00, 0x00,  \
    0x00, 0x00, 0x00, 0x00,  /* Reserved */\
    0x00, 0x00               /* Reserved */

#define MS_OS_COMPATIDS_ALL \
    MS_OS_COMPATIDS(64,2), \
    MS_OS_COMPATIDS_WINUSB, \
    MS_OS_COMPATIDS_XUSB

#define MS_OS_COMPATIDS_GENERIC \
    MS_OS_COMPATIDS(40, 1), \
    MS_OS_COMPATIDS_WINUSB







/**
 * Xinput request handlers
 */
struct xinput_req_handlers {
	/* Handler for Xinput Vendor specific commands */
	usb_request_handler vendor_handler;
	/**
	 * The custom request handler gets a first chance at handling
	 * the request before it is handed over to the 'chapter 9' request
	 * handler
	 */
	usb_request_handler custom_handler;
};

/**
 * @brief Register Custom and Vendor request callbacks
 *
 * Function to register Custom and Vendor request callbacks
 * for handling requests.
 *
 * @param [in] handlers Pointer to Xinput request handlers structure
 */
void xinput_register_request_handlers(struct xinput_req_handlers *handlers);

#endif /* __XINPUT_H__ */
