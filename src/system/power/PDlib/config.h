#ifndef USB_PD_CONFIG
#define USB_PD_CONFIG

// 1 - 3. 0 is disabled
#define PD_DEBUG_LEVEL 0

/*
 * Provide common runtime layer code (tasks, hooks ...)
 * You want this unless you are doing a really tiny firmware.
 */
#undef CONFIG_COMMON_RUNTIME

// automatically select highest power profile
// TODO most of the implementation is commented out
#define CONFIG_CHARGE_MANAGER

/* Support v1.1 type-C connection state machine */
#define CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP

/* Support for USB type-c vconn. Not needed for captive cables. */
// our hardware  does not support it.
#undef CONFIG_USBC_VCONN

/* Support VCONN swap */
#undef CONFIG_USBC_VCONN_SWAP

/* Support for USB PD alternate mode */
#undef CONFIG_USB_PD_ALT_MODE

/* Define if this board can act as a dual-role PD port (source and sink) */
#define CONFIG_USB_PD_DUAL_ROLE

/* Define if this board can used TCPC-controlled DRP toggle */
#undef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE

/* Simple DFP, such as power adapter, will not send discovery VDM on connect */
#undef CONFIG_USB_PD_SIMPLE_DFP

/*
 * Choose one of the following TCPMs (type-C port manager) to manage TCPC. The
 * TCPM stub is used to make direct function calls to TCPC when TCPC is on
 * the same MCU. The TCPCI TCPM uses the standard TCPCI i2c interface to TCPC.
 */
#undef CONFIG_USB_PD_TCPM_STUB
#undef CONFIG_USB_PD_TCPM_TCPCI
#define CONFIG_USB_PD_TCPM_FUSB302 // the chip we use
#undef CONFIG_USB_PD_TCPM_ITE_ON_CHIP
#undef CONFIG_USB_PD_TCPM_ANX3429
#undef CONFIG_USB_PD_TCPM_ANX740X
#undef CONFIG_USB_PD_TCPM_ANX741X
#undef CONFIG_USB_PD_TCPM_ANX7447
#undef CONFIG_USB_PD_TCPM_ANX7688
#undef CONFIG_USB_PD_TCPM_NCT38XX
#undef CONFIG_USB_PD_TCPM_MT6370
#undef CONFIG_USB_PD_TCPM_TUSB422
#undef CONFIG_USB_PD_TCPM_RAA489000
#undef CONFIG_USB_PD_TCPM_RT1715
#undef CONFIG_USB_PD_TCPM_FUSB307
#undef CONFIG_USB_PD_TCPM_STM32GX

// Enable to prefer higher voltage profiles in case of power profile tie
#define PD_PREFER_HIGH_VOLTAGE
#undef PD_PREFER_LOW_VOLTAGE
/*
 * Override the pull-up value when only zero or one port is actively sourcing
 * current and we can advertise more current than what is defined by
 * `CONFIG_USB_PD_PULLUP`.
 * Should be defined with one of the tcpc_rp_value.
 */
#undef CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT

/* Support for USB type-c superspeed mux */
#undef CONFIG_USBC_SS_MUX

/* Use this option to enable Try.SRC mode for Dual Role devices */
#undef CONFIG_USB_PD_TRY_SRC

/* Set the default minimum battery percentage for Try.Src to be enabled */
#define CONFIG_USB_PD_TRY_SRC_MIN_BATT_SOC 5

/*
 * The TCPM must know whether VBUS is present in order to make proper state
 * transitions. In addition, charge_manager must know about VBUS presence in
 * order to make charging decisions. VBUS state can be determined by various
 * methods:
 * - Some TCPCs can detect and report the presence of VBUS.
 * - In some configurations, charger ICs can report the presence of VBUS.
 * - On some boards, dedicated VBUS interrupt pins are available.
 * - Some power path controllers (PPC) can report the presence of VBUS.
 *
 * Exactly one of these should be defined for all boards that run the PD
 * state machine.
 */
#define CONFIG_USB_PD_VBUS_DETECT_TCPC
#undef CONFIG_USB_PD_VBUS_DETECT_CHARGER
#undef CONFIG_USB_PD_VBUS_DETECT_GPIO
#undef CONFIG_USB_PD_VBUS_DETECT_PPC
#undef CONFIG_USB_PD_VBUS_DETECT_NONE

/*
 * PD Rev2.0 functionality is enabled by default. Defining this macro
 * enables PD Rev3.0 functionality.
 */
#undef CONFIG_USB_PD_REV30

/* Default pull-up value on the USB-C ports when they are used as source. */
#define CONFIG_USB_PD_PULLUP TYPEC_RP_1A5

/* Initial DRP / toggle policy */
#define CONFIG_USB_PD_INITIAL_DRP_STATE PD_DRP_TOGGLE_OFF

/*
 * USB Product ID. Each platform (e.g. baseboard set) should have a single
 * VID/PID combination. If there is a big enough change within a platform,
 * then we can differentiate USB topologies by varying the HW version field
 * in the Sink and Source Capabilities Extended messages.
 *
 * To reserve a new PID, use go/usb.
 */
#define CONFIG_USB_PID 0x1209 // here, default

/* Use TCPC module (type-C port controller) */
#undef CONFIG_USB_PD_TCPC

/* Board provides specific TCPC init function */
#undef CONFIG_USB_PD_TCPC_BOARD_INIT

/* Enable TCPC to enter low power mode */
#undef CONFIG_USB_PD_TCPC_LOW_POWER

/* Save power by waking up on VBUS rather than polling CC */
#undef CONFIG_USB_PD_LOW_POWER

/* Define if this board, operating as a sink, can give power back to a source */
#undef CONFIG_USB_PD_GIVE_BACK

/* Dynamic USB PD source capability */
#undef CONFIG_USB_PD_DYNAMIC_SRC_CAP

/*****************************************************************************/
/*
 * Define CONFIG_USB_PD_VBUS_MEASURE_TCPC if the tcpc on the board supports
 * VBUS measurement.
 */
#if defined(CONFIG_USB_PD_TCPM_FUSB302)
// vbus measurment
#define CONFIG_USB_PD_VBUS_MEASURE_TCPC
// only support 2.0
#undef CONFIG_USB_PD_REV30
#endif

/* Override PD_ROLE_DEFAULT in usb_pd.h */
#define PD_ROLE_DEFAULT() (PD_ROLE_SINK)

#endif
