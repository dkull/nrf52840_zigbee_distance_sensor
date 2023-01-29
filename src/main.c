/*
 * Written by Brandon Satrom.
 *
 * Copyright (c) 2022 Blues Inc. MIT License. Use of this source code is
 * governed by licenses granted by the copyright holder including that found in
 * the LICENSE file.
 */

#include <sys/printk.h>
#include <sys/util.h>
#include <string.h>

#include <usb/usb_device.h>
#include <drivers/uart.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>

#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>
#include <zigbee/zigbee_zcl_scenes.h>
#include <zb_nrf_platform.h>

#include "my_zb_zcl_analog_input.h"
#include "zb_ultrasound_distance.h"

// Edited remotely

LOG_MODULE_REGISTER(zigmee, LOG_LEVEL_DBG);

/*
 * Ensure that an overlay for USB serial has been defined.
 */
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
	    "Console device is not ACM CDC UART device");

/*
 * Defines
 */

#define ZB_ZCL_MIN_REPORTING_INTERVAL_DEFAULT 0x05
#define ZIGBEE_NETWORK_STATE_LED 0
#define LED0 DT_ALIAS(led0)
/* Device endpoint, used to receive ZCL commands. */
#define ULTRASOUND_DISTANCE_ENDPOINT 1
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0, gpios);

/* ESP tutorial */
#define IEEE_CHANNEL_MASK (1l << 13)  // channel 13
#define ERASE_PERSISTENT_CONFIG ZB_TRUE
/* END ESP tutorial */

/*
 * Zigbee setup
 */

/* Main application customizable context.
 * Stores all settings and static values.
 */

// ===== Declare Structures =====

typedef struct {
	zb_uint32_t present_value;
	zb_bool_t out_of_service;
	zb_uint8_t status_flags;
} my_zb_zcl_analog_input_basic_attrs;

typedef struct {
	zb_zcl_basic_attrs_ext_t basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
	zb_zcl_scenes_attrs_t scenes_attr;
	zb_zcl_groups_attrs_t groups_attr;
	my_zb_zcl_analog_input_basic_attrs analog_input_attr;
} zb_device_ctx;

static zb_device_ctx dev_ctx;

// ===== Declare Attributes =====

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(basic_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.app_version,
	&dev_ctx.basic_attr.stack_version,
	&dev_ctx.basic_attr.hw_version,
	&dev_ctx.basic_attr.mf_name,
	&dev_ctx.basic_attr.model_id,
	&dev_ctx.basic_attr.date_code,
	&dev_ctx.basic_attr.location_id,
	&dev_ctx.basic_attr.ph_env,
	&dev_ctx.basic_attr.sw_ver,
	&dev_ctx.basic_attr.power_source);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_GROUPS_ATTRIB_LIST(groups_attr_list,
	&dev_ctx.groups_attr.name_support);

ZB_ZCL_DECLARE_SCENES_ATTRIB_LIST(scenes_attr_list,
	&dev_ctx.scenes_attr.scene_count,
	&dev_ctx.scenes_attr.current_scene,
	&dev_ctx.scenes_attr.current_group,
	&dev_ctx.scenes_attr.scene_valid,
	&dev_ctx.scenes_attr.name_support);
	
ZB_ZCL_DECLARE_ANALOG_INPUT_ATTRIB_LIST(analog_input_attr_list,
    &dev_ctx.analog_input_attr.out_of_service,
    &dev_ctx.analog_input_attr.present_value,
    &dev_ctx.analog_input_attr.status_flags);

// ===== Declare Device =====

ZB_DECLARE_ULTRASOUND_DISTANCE_CLUSTER_LIST(ultrasound_distance_clusters,
	basic_attr_list,
	identify_attr_list,
    groups_attr_list,
    scenes_attr_list,
	analog_input_attr_list);

ZB_DECLARE_ULTRASOUND_DISTANCE_EP(
	ultrasound_distance_ep,
	ULTRASOUND_DISTANCE_ENDPOINT,
	ultrasound_distance_clusters);
	

ZBOSS_DECLARE_DEVICE_CTX_1_EP(ultrasound_distance_ctx,
	ultrasound_distance_ep);

// ===== Business functions =====

static void init_zigbee_cluster_attributes() {
	// "Basic" cluster attributes
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;
	ZB_ZCL_SET_STRING_VAL(
		dev_ctx.basic_attr.mf_name,
		"Tanel Liiv",
		ZB_ZCL_STRING_CONST_SIZE("Tanel Liiv"));
	ZB_ZCL_SET_STRING_VAL(
		dev_ctx.basic_attr.model_id,
		"tliiv.ultrasound_distance",
		ZB_ZCL_STRING_CONST_SIZE("tliiv.ultrasound_distance"));

	// "Identify" cluster attributes
	dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

	// Groups cluster attribs
	dev_ctx.groups_attr.name_support = 0;

	// Scenes cluster attribs
	dev_ctx.scenes_attr.name_support = 0;

	// Distance attributes
	dev_ctx.analog_input_attr.out_of_service = false;
	dev_ctx.analog_input_attr.present_value = 123.0;
	dev_ctx.analog_input_attr.status_flags = 0;
}

static void start_identifying() {
	if (ZB_JOINED()) {
		zb_ret_t zb_err_code = zb_bdb_finding_binding_target(
			ULTRASOUND_DISTANCE_ENDPOINT
		);	
		if (zb_err_code == RET_OK) {
			printk("Entered identify mode!\n");
		} else if (zb_err_code == RET_INVALID_STATE) {
			printk("Invalid state!\n");	
		} else {
			printk("Umm, something else\n");
		}
	} else {
		printk("Device not in a network, cannot identify\n");
	}
}

/*
 * @brief Function to handle identify notification events on the first endpoint.
 * @param  bufid  Unused parameter, required by ZBOSS scheduler API.
 */
static void identify_cb(zb_bufid_t bufid)
{
	printk("IdentityCB!\n");
	k_sleep(K_MSEC(1000));

	zb_ret_t zb_err_code;
	if (bufid) {
		// Schedule a self-scheduling function that will toggle the LED
		//ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	} else {
		// Cancel the toggling function alarm and turn off LED
		//zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);
		//dk_set_led(IDENTIFY_LED, 0);
	}
}

static void zcl_device_cb(zb_bufid_t bufid) {
	printk("DEVICE_DB\n");
}

void zboss_signal_handler(zb_bufid_t bufid) {
    ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
    if (bufid) {
        zb_buf_free(bufid);
    }
}

void zboss_signal_handler2(zb_bufid_t bufid) {

	// update network status led
	//zigbee_led_status_update(bufid, ZIGBEE_NETWORK_STATE_LED);

	// No application-specific behavior is required. Call default signal handler.
	//zigbee_default_signal_handler(bufid);
	
	zb_zdo_app_signal_hdr_t *hdr = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(bufid, &hdr);
	zb_ret_t              status = ZB_GET_APP_SIGNAL_STATUS(bufid);

	if (sig != ZB_COMMON_SIGNAL_CAN_SLEEP) {
		printk("ZBOSS_SIGNAL_HANDLER signal %d status %d\n", sig, status);
	}

	switch (sig){
    case ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY:
        printk("Restored config?\n");
        break;
	case ZB_ZDO_SIGNAL_SKIP_STARTUP:
		printk("Zigbee stack init\n");
		bdb_start_top_level_commissioning(ZB_BDB_INITIALIZATION);
		break;
	case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
		if (status == RET_OK) {
			printk("Start network steering\n");
			bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
		} else {
			printk("Failed to init Zigbee stack\n");
		}
		break;
	case ZB_BDB_SIGNAL_STEERING:
		if (status == RET_OK) {
			printk("Joined network!\n");
		} else {
			printk("Steering unsuccessful :(\n");
			ZB_SCHEDULE_APP_ALARM((zb_callback_t) bdb_start_top_level_commissioning,
					ZB_BDB_NETWORK_STEERING, ZB_TIME_ONE_SECOND);
		}
		break;
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
		printk("Re/Joined a network using NVRAM contents\n");
        zb_ret_t resp = zb_zcl_start_attr_reporting(
                ULTRASOUND_DISTANCE_ENDPOINT,
                ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
                ZB_ZCL_CLUSTER_SERVER_ROLE,
                ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID);
        if (resp == RET_OK) {
            printk("Reporting started!\n");
        } else {
            printk("Reporting failed to start\n");
        }
		break;
	case ZB_COMMON_SIGNAL_CAN_SLEEP:
		break;
	default:
		// print the unknown signal value
		printk("Unhandled signal: %d\n", sig);
		break;
	}

	k_sleep(K_MSEC(333));

	/* All callbacks should either reuse or free passed buffers.
	 * If bufid == 0, the buffer is invalid (not passed).
	 */
	if (bufid) {
		zb_buf_free(bufid);
	}
}

/*
 * None Zigbee stuff
 */

void zigbee_setup() {
	// -- Zigbee Stack
	
	/*ZB_INIT("avalanche_duck");
	zb_set_network_ed_role(IEEE_CHANNEL_MASK);
	zb_set_nvram_erase_at_start(ERASE_PERSISTENT_CONFIG);
	zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
	zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));*/

	zb_set_network_ed_role(ZB_TRANSCEIVER_ALL_CHANNELS_MASK);
	// -- HW Related

	ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb);
	ZB_AF_REGISTER_DEVICE_CTX(&ultrasound_distance_ctx);

	// set all initial values
	init_zigbee_cluster_attributes();

	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(ULTRASOUND_DISTANCE_ENDPOINT, identify_cb);
}

static zb_uint32_t i = 0;
void measure_distance() {
    //if (!system_param.connected) return;
    
    zb_zcl_set_attr_val(
        ULTRASOUND_DISTANCE_ENDPOINT,
        ZB_ZCL_CLUSTER_ID_BASIC,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_BASIC_HW_VERSION_ID,
        &i,
        ZB_FALSE);

    // -- Measure distance
    printk("Measure distance %d\n", i++);
    // integer to bytes
    zb_uint8_t *bytes = (zb_uint8_t *)&i;

    zb_zcl_status_t result = zb_zcl_set_attr_val(
        ULTRASOUND_DISTANCE_ENDPOINT,
        ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID,
        bytes,
        ZB_FALSE
    );
    if (result != ZB_ZCL_STATUS_SUCCESS) {
        printk("Failed to set attribute value: %d\n", result);
    }
    zb_zcl_mark_attr_for_reporting(
        ULTRASOUND_DISTANCE_ENDPOINT,
        ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID
    );
    zb_bool_t resp = zcl_is_attr_reported(
            ULTRASOUND_DISTANCE_ENDPOINT,
            ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
            ZB_ZCL_CLUSTER_SERVER_ROLE,
            ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID);
    if (resp) {
        printk("Attribute is reported\n");
    } else {
        printk("Attribute is not reported\n");
    }

}

void main(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

	/* Set ZigBee stack logging level and traffic dump subsystem. */
	//ZB_SET_TRACE_LEVEL(0b111111);
	//ZB_SET_TRACE_MASK(ZIGBEE_TRACE_MASK);
	//ZB_SET_TRAF_DUMP_OFF();

	// --- Configure USB Serial for Console output ---


	const struct device *usb_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;

	if (usb_enable(NULL)) {
		return;
	}

	uint32_t i = 0;
	while (!dtr) {
		uart_line_ctrl_get(usb_dev, UART_LINE_CTRL_DTR, &dtr);
		gpio_pin_set_raw(led.port, led.pin, i++ % 10 > 0 ? 0 : 1);
		k_sleep(K_MSEC(200));
	}

	//zcl_scenes_init();
	/*int err = settings_subsys_init();
	if (err) {
		printk("Failed to init settings!\n");
	}*/

	// --- Start ---

	printk("=| Welcome to Avalanche Duck NRF52840 Zigbee Endpoint v0.1|=\n");
	LOG_ERR("HI LOG WORKS!\n");

	zigbee_setup();
	zigbee_enable();

    zb_ret_t resp = zb_zcl_start_attr_reporting(
            ULTRASOUND_DISTANCE_ENDPOINT,
            ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
            ZB_ZCL_CLUSTER_SERVER_ROLE,
            ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID);
    if (resp == RET_OK) {
        printk("Reporting started!\n");
    } else {
        printk("Reporting failed to start\n");
    }

    resp = zb_zcl_start_attr_reporting(
            ULTRASOUND_DISTANCE_ENDPOINT,
            ZB_ZCL_CLUSTER_ID_BASIC,
            ZB_ZCL_CLUSTER_SERVER_ROLE,
            ZB_ZCL_ATTR_BASIC_HW_VERSION_ID);
    if (resp == RET_OK) {
        printk("Reporting started!\n");
    } else {
        printk("Reporting failed to start\n");
    }

	i = 1;
	while (1) {
		gpio_pin_set_raw(led.port, led.pin, i % 2);
        printk("Main loop %d\n", i++);
		if (i % 10 == 0) {
            zboss_main_loop_iteration();
            printk("measuring\n");
            measure_distance();
		}
		k_sleep(K_MSEC(1000));
	}
}
