// sdk-nrf / zigbee / light_bulb / zb_dimmable_light.h
// https://github.com/nrfconnect/sdk-nrf/blob/main/samples/zigbee/light_bulb/include/zb_dimmable_light.h
// I excluded level control in last _EP

// -- Declare Cluster List

#define ZB_DECLARE_ULTRASOUND_DISTANCE_CLUSTER_LIST( \
	cluster_list_name, \
	basic_attr_list, \
	identify_attr_list, \
    groups_attr_list, \
    scenes_attr_list, \
	analog_input_attr_list) \
	zb_zcl_cluster_desc_t cluster_list_name[] = \
	{ \
		ZB_ZCL_CLUSTER_DESC( \
			ZB_ZCL_CLUSTER_ID_IDENTIFY, \
			ZB_ZCL_ARRAY_SIZE(identify_attr_list, zb_zcl_attr_t), \
			(identify_attr_list), \
			ZB_ZCL_CLUSTER_SERVER_ROLE, \
			ZB_ZCL_MANUF_CODE_INVALID \
		),\
		ZB_ZCL_CLUSTER_DESC( \
			ZB_ZCL_CLUSTER_ID_BASIC, \
			ZB_ZCL_ARRAY_SIZE(basic_attr_list, zb_zcl_attr_t), \
			(basic_attr_list), \
			ZB_ZCL_CLUSTER_SERVER_ROLE, \
			ZB_ZCL_MANUF_CODE_INVALID \
		),\
        ZB_ZCL_CLUSTER_DESC( \
            ZB_ZCL_CLUSTER_ID_GROUPS, \
            ZB_ZCL_ARRAY_SIZE(groups_attr_list, zb_zcl_attr_t), \
            (groups_attr_list), \
            ZB_ZCL_CLUSTER_SERVER_ROLE, \
            ZB_ZCL_MANUF_CODE_INVALID \
        ),\
        ZB_ZCL_CLUSTER_DESC( \
            ZB_ZCL_CLUSTER_ID_SCENES, \
            ZB_ZCL_ARRAY_SIZE(scenes_attr_list, zb_zcl_attr_t), \
            (scenes_attr_list), \
            ZB_ZCL_CLUSTER_SERVER_ROLE, \
            ZB_ZCL_MANUF_CODE_INVALID \
        ),\
		ZB_ZCL_CLUSTER_DESC( \
			ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, \
			ZB_ZCL_ARRAY_SIZE(analog_input_attr_list, zb_zcl_attr_t), \
			(analog_input_attr_list), \
			ZB_ZCL_CLUSTER_SERVER_ROLE, \
			ZB_ZCL_MANUF_CODE_INVALID \
		),\
	}

// -- Declare Simple Descriptor for device

#define ZB_ULTRASOUND_DISTANCE_DEVICE_ID 0x0101
#define ZB_DEVICE_VER_ULTRASOUND_DISTANCE 1

#define ZB_ZCL_DECLARE_HA_ULTRASOUND_DISTANCE_SIMPLE_DESC( \
        ep_name, \
        ep_id, \
        in_clust_num, \
        out_clust_num) \
	ZB_DECLARE_SIMPLE_DESC(in_clust_num, out_clust_num); \
	ZB_AF_SIMPLE_DESC_TYPE(in_clust_num, out_clust_num) simple_desc_##ep_name = \
	{ \
		ep_id, \
		ZB_AF_HA_PROFILE_ID, \
		ZB_ULTRASOUND_DISTANCE_DEVICE_ID, \
		ZB_DEVICE_VER_ULTRASOUND_DISTANCE, \
		0, \
		in_clust_num, \
		out_clust_num, \
		{ \
			ZB_ZCL_CLUSTER_ID_BASIC, \
			ZB_ZCL_CLUSTER_ID_IDENTIFY, \
            ZB_ZCL_CLUSTER_ID_GROUPS, \
            ZB_ZCL_CLUSTER_ID_SCENES, \
			ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, \
		} \
	}

// -- Declar Endpoint for device

#define ZB_ULTRASOUND_DISTANCE_IN_CLUSTER_NUM 5
#define ZB_ULTRASOUND_DISTANCE_OUT_CLUSTER_NUM 0
#define ZB_ULTRASOUND_DISTANCE_REPORT_ATTR_COUNT (ZB_ZCL_ANALOG_INPUT_REPORT_ATTR_COUNT)
#define ZB_ULTRASOUND_DISTANCE_CVC_ATTR_COUNT 1

#define ZB_DECLARE_ULTRASOUND_DISTANCE_EP(ep_name, ep_id, cluster_list) \
	ZB_ZCL_DECLARE_HA_ULTRASOUND_DISTANCE_SIMPLE_DESC( \
        ep_name, \
        ep_id, \
        ZB_ULTRASOUND_DISTANCE_IN_CLUSTER_NUM, \
        ZB_ULTRASOUND_DISTANCE_OUT_CLUSTER_NUM); \
    \
	ZBOSS_DEVICE_DECLARE_REPORTING_CTX( \
        reporting_info## ep_name, \
		ZB_ULTRASOUND_DISTANCE_REPORT_ATTR_COUNT); \
    \
	ZB_AF_DECLARE_ENDPOINT_DESC(ep_name, ep_id, ZB_AF_HA_PROFILE_ID, \
		0, \
		NULL, \
		ZB_ZCL_ARRAY_SIZE(cluster_list, zb_zcl_cluster_desc_t), \
		cluster_list, \
		(zb_af_simple_desc_1_1_t *)&simple_desc_## ep_name, \
		ZB_ULTRASOUND_DISTANCE_REPORT_ATTR_COUNT, \
		reporting_info## ep_name, \
		0, NULL);
			
