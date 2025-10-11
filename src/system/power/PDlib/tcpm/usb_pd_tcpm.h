/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* USB Power delivery port management */

#ifndef __CROS_EC_USB_PD_TCPM_H
#define __CROS_EC_USB_PD_TCPM_H

#include "../config.h"
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* List of common error codes that can be returned */
  enum ec_error_list
  {
    /* Success - no error */
    EC_SUCCESS = 0,
    /* Unknown error */
    EC_ERROR_UNKNOWN = 1,
    /* Function not implemented yet */
    EC_ERROR_UNIMPLEMENTED = 2,
    /* Overflow error; too much input provided. */
    EC_ERROR_OVERFLOW = 3,
    /* Timeout */
    EC_ERROR_TIMEOUT = 4,
    /* Invalid argument */
    EC_ERROR_INVAL = 5,
    /* Already in use, or not ready yet */
    EC_ERROR_BUSY = 6,
    /* Access denied */
    EC_ERROR_ACCESS_DENIED = 7,
    /* Failed because component does not have power */
    EC_ERROR_NOT_POWERED = 8,
    /* Failed because component is not calibrated */
    EC_ERROR_NOT_CALIBRATED = 9,
    /* Failed because CRC error */
    EC_ERROR_CRC = 10,
    /* Invalid console command param (PARAMn means parameter n is bad) */
    EC_ERROR_PARAM1 = 11,
    EC_ERROR_PARAM2 = 12,
    EC_ERROR_PARAM3 = 13,
    EC_ERROR_PARAM4 = 14,
    EC_ERROR_PARAM5 = 15,
    EC_ERROR_PARAM6 = 16,
    EC_ERROR_PARAM7 = 17,
    EC_ERROR_PARAM8 = 18,
    EC_ERROR_PARAM9 = 19,
    /* Wrong number of params */
    EC_ERROR_PARAM_COUNT = 20,
    /* Interrupt event not handled */
    EC_ERROR_NOT_HANDLED = 21,
    /* Data has not changed */
    EC_ERROR_UNCHANGED = 22,
    /* Memory allocation */
    EC_ERROR_MEMORY_ALLOCATION = 23,

    /* Verified boot errors */
    EC_ERROR_VBOOT_SIGNATURE = 0x1000, /* 4096 */
    EC_ERROR_VBOOT_SIG_MAGIC = 0x1001,
    EC_ERROR_VBOOT_SIG_SIZE = 0x1002,
    EC_ERROR_VBOOT_SIG_ALGORITHM = 0x1003,
    EC_ERROR_VBOOT_HASH_ALGORITHM = 0x1004,
    EC_ERROR_VBOOT_SIG_OFFSET = 0x1005,
    EC_ERROR_VBOOT_DATA_SIZE = 0x1006,

    /* Verified boot key errors */
    EC_ERROR_VBOOT_KEY = 0x1100,
    EC_ERROR_VBOOT_KEY_MAGIC = 0x1101,
    EC_ERROR_VBOOT_KEY_SIZE = 0x1102,

    /* Verified boot data errors */
    EC_ERROR_VBOOT_DATA = 0x1200,
    EC_ERROR_VBOOT_DATA_VERIFY = 0x1201,

    /* Module-internal error codes may use this range.   */
    EC_ERROR_INTERNAL_FIRST = 0x10000,
    EC_ERROR_INTERNAL_LAST = 0x1FFFF
  };

  struct ec_response_pd_chip_info
  {
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_id;
    union
    {
      uint8_t fw_version_string[8];
      uint64_t fw_version_number;
    } __packed;
  };

/* Default retry count for transmitting */
#define PD_RETRY_COUNT 3

/* Time to wait for TCPC to complete transmit */
#define PD_T_TCPC_TX_TIMEOUT (100 * MSEC_US)

  enum usbpd_cc_pin
  {
    USBPD_CC_PIN_1,
    USBPD_CC_PIN_2,
  };

  /* Detected resistor values of port partner */
  enum tcpc_cc_voltage_status
  {
    TYPEC_CC_VOLT_OPEN = 0,
    TYPEC_CC_VOLT_RA = 1,     /* Port partner is applying Ra */
    TYPEC_CC_VOLT_RD = 2,     /* Port partner is applying Rd */
    TYPEC_CC_VOLT_RP_DEF = 5, /* Port partner is applying Rp (0.5A) */
    TYPEC_CC_VOLT_RP_1_5 = 6, /* Port partner is applying Rp (1.5A) */
    TYPEC_CC_VOLT_RP_3_0 = 7, /* Port partner is applying Rp (3.0A) */
  };

  /* Resistor types we apply on our side of the CC lines */
  enum tcpc_cc_pull
  {
    TYPEC_CC_RA = 0,
    TYPEC_CC_RP = 1,
    TYPEC_CC_RD = 2,
    TYPEC_CC_OPEN = 3,
    TYPEC_CC_RA_RD = 4, /* Powered cable with Sink */
  };

  /* Pull-up values we apply as a SRC to advertise different current limits */
  enum tcpc_rp_value
  {
    TYPEC_RP_USB = 0,
    TYPEC_RP_1A5 = 1,
    TYPEC_RP_3A0 = 2,
    TYPEC_RP_RESERVED = 3,
  };

  /* Possible port partner connections based on CC line states */
  enum pd_cc_states
  {
    PD_CC_NONE = 0, /* No port partner attached */

    /* From DFP perspective */
    PD_CC_UFP_NONE = 1,      /* No UFP accessory connected */
    PD_CC_UFP_AUDIO_ACC = 2, /* UFP Audio accessory connected */
    PD_CC_UFP_DEBUG_ACC = 3, /* UFP Debug accessory connected */
    PD_CC_UFP_ATTACHED = 4,  /* Plain UFP attached */

    /* From UFP perspective */
    PD_CC_DFP_ATTACHED = 5,  /* Plain DFP attached */
    PD_CC_DFP_DEBUG_ACC = 6, /* DFP debug accessory connected */
  };

  /* DRP (dual-role-power) setting */
  enum tcpc_drp
  {
    TYPEC_NO_DRP = 0,
    TYPEC_DRP = 1,
  };

  /*
   * Power role.
   *
   * Note this is also used for PD header creation, and values align to those in
   * the Power Delivery Specification Revision 3.0 (See
   * 6.2.1.1.4 Port Power Role).
   */
  enum pd_power_role
  {
    PD_ROLE_SINK = 0,
    PD_ROLE_SOURCE = 1
  };

  /*
   * Data role.
   *
   * Note this is also used for PD header creation, and the first two values
   * align to those in the Power Delivery Specification Revision 3.0 (See
   * 6.2.1.1.6 Port Data Role).
   */
  enum pd_data_role
  {
    PD_ROLE_UFP = 0,
    PD_ROLE_DFP = 1,
    PD_ROLE_DISCONNECTED = 2,
  };

  enum pd_vconn_role
  {
    PD_ROLE_VCONN_OFF = 0,
    PD_ROLE_VCONN_SRC = 1,
  };

  /*
   * Note: BIT(0) may be used to determine whether the polarity is CC1 or CC2,
   * regardless of whether a debug accessory is connected.
   */
  enum tcpc_cc_polarity
  {
    /*
     * _CCx: is used to indicate the polarity while not connected to
     * a Debug Accessory.  Only one CC line will assert a resistor and
     * the other will be open.
     */
    POLARITY_CC1 = 0,
    POLARITY_CC2 = 1,

    /*
     * _CCx_DTS is used to indicate the polarity while connected to a
     * SRC Debug Accessory.  Assert resistors on both lines.
     */
    POLARITY_CC1_DTS = 2,
    POLARITY_CC2_DTS = 3,

    /*
     * The current TCPC code relies on these specific POLARITY values.
     * Adding in a check to verify if the list grows for any reason
     * that this will give a hint that other places need to be
     * adjusted.
     */
    POLARITY_COUNT
  };

  /**
   * Returns whether the polarity without the DTS extension
   */
  static inline enum tcpc_cc_polarity polarity_rm_dts(enum tcpc_cc_polarity polarity)
  {
    static_assert(POLARITY_COUNT == 4);
    return (enum tcpc_cc_polarity)(polarity & (1 << 0));
  }

  enum tcpm_transmit_type
  {
    TCPC_TX_SOP = 0,
    TCPC_TX_SOP_PRIME = 1,
    TCPC_TX_SOP_PRIME_PRIME = 2,
    TCPC_TX_SOP_DEBUG_PRIME = 3,
    TCPC_TX_SOP_DEBUG_PRIME_PRIME = 4,
    TCPC_TX_HARD_RESET = 5,
    TCPC_TX_CABLE_RESET = 6,
    TCPC_TX_BIST_MODE_2 = 7
  };

  enum tcpc_transmit_complete
  {
    TCPC_TX_UNSET = -1,
    TCPC_TX_COMPLETE_SUCCESS = 0,
    TCPC_TX_COMPLETE_DISCARDED = 1,
    TCPC_TX_COMPLETE_FAILED = 2,
  };

  /*
   * USB-C PD Vbus levels
   *
   * Return true on Vbus check if Vbus is...
   */
  enum vbus_level
  {
    VBUS_SAFE0V,  /* less than vSafe0V max */
    VBUS_PRESENT, /* at least vSafe5V min */
    VBUS_REMOVED, /* less than vSinkDisconnect max */
  };

  struct tcpm_drv
  {
    /**
     * Initialize TCPM driver and wait for TCPC readiness.
     *
     * @param port Type-C port number
     *
     * @return EC_SUCCESS or error
     */
    int (*init)();

    /**
     * Release the TCPM hardware and disconnect the driver.
     * Only .init() can be called after .release().
     *
     * @return EC_SUCCESS or error
     */
    int (*release)();

    /**
     * Read the CC line status.
     *
     * @param cc1 pointer to CC status for CC1
     * @param cc2 pointer to CC status for CC2
     *
     * @return EC_SUCCESS or error
     */
    int (*get_cc)(enum tcpc_cc_voltage_status* cc1, enum tcpc_cc_voltage_status* cc2);

    /**
     * Check VBUS level
     *
     * @param level safe level voltage to check against
     *
     * @return False => VBUS not at level, True => VBUS at level
     */
    int (*get_vbus_level)(enum vbus_level level);

    /**
     * Get VBUS voltage
     *
     * @param port Type-C port number
     * @param vbus read VBUS voltage in mV
     *
     * @return EC_SUCCESS or error
     */
    int (*get_vbus_voltage)(int* vbus);

    /**
     * Set the value of the CC pull-up used when we are a source.
     *
     * @param rp One of enum tcpc_rp_value
     *
     * @return EC_SUCCESS or error
     */
    int (*select_rp_value)(int rp);

    /**
     * Set the CC pull resistor. This sets our role as either source or sink.
     *
     * @param pull One of enum tcpc_cc_pull
     *
     * @return EC_SUCCESS or error
     */
    int (*set_cc)(int pull);

    /**
     * Set polarity
     *
     * @param polarity port polarity
     *
     * @return EC_SUCCESS or error
     */
    int (*set_polarity)(enum tcpc_cc_polarity polarity);

#ifdef CONFIG_USB_PD_DECODE_SOP
    /**
     * Control receive of SOP' and SOP'' messages. This is provided
     * separately from set_vconn so that we can preemptively disable
     * receipt of SOP' messages during a VCONN swap, or disable during spans
     * when port partners may erroneously be sending cable messages.
     *
     * @param enable Enable SOP' and SOP'' messages
     *
     * @return EC_SUCCESS or error
     */
    int (*sop_prime_enable)(int enable);
#endif

    /**
     * Set Vconn.
     *
     * @param enable Enable/Disable Vconn
     *
     * @return EC_SUCCESS or error
     */
    int (*set_vconn)(int enable);

    /**
     * Set PD message header to use for goodCRC
     *
     * @param power_role Power role to use in header
     * @param data_role Data role to use in header
     *
     * @return EC_SUCCESS or error
     */
    int (*set_msg_header)(int power_role, int data_role);

    /**
     * Set RX enable flag
     *
     * @enable true for enable, false for disable
     *
     * @return EC_SUCCESS or error
     */
    int (*set_rx_enable)(int enable);

    /**
     * Read last received PD message.
     *
     * @param port Type-C port number
     * @param payload Pointer to location to copy payload of message
     * @param header of message
     *
     * @return EC_SUCCESS or error
     */
    int (*get_message)(uint32_t* payload, uint32_t* head);

    /**
     * Transmit PD message
     *
     * @param type Transmit type
     * @param header Packet header
     * @param cnt Number of bytes in payload
     * @param data Payload
     *
     * @return EC_SUCCESS or error
     */
    int (*transmit)(enum tcpm_transmit_type type, uint16_t header, const uint32_t* data);

    /**
     * TCPC is asserting alert
     *
     */
    void (*tcpc_alert)();

    /**
     * Discharge PD VBUS on src/sink disconnect & power role swap
     *
     * @param enable Discharge enable or disable
     */
    void (*tcpc_discharge_vbus)(int enable);

    /**
     * Auto Discharge Disconnect
     *
     * @param enable Auto Discharge enable or disable
     */
    void (*tcpc_enable_auto_discharge_disconnect)(int enable);

    /**
     * Manual control of TCPC DebugAccessory enable
     *
     * @param enable Debug Accessory enable or disable
     */
    int (*debug_accessory)(int enable);

    /**
     * Break debug connection, if TCPC requires specific commands to be run
     * in order to correctly exit a debug connection.
     *
     */
    int (*debug_detach)();

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
    /**
     * Enable TCPC auto DRP toggling.
     *
     *
     * @return EC_SUCCESS or error
     */
    int (*drp_toggle)();
#endif

    /**
     * Get firmware version.
     *
     * @param live Fetch live chip info or hard-coded + cached info
     * @param info Pointer to PD chip info; NULL to cache the info only
     *
     * @return EC_SUCCESS or error
     */
    int (*get_chip_info)(int live, struct ec_response_pd_chip_info** info);

#ifdef CONFIG_USBC_PPC
    /**
     * Request current sinking state of the TCPC
     * NOTE: this is most useful for PPCs that can not tell on their own
     *
     * @param is_sinking true for sinking, false for not
     *
     * @return EC_SUCCESS, EC_ERROR_UNIMPLEMENTED or error
     */
    int (*get_snk_ctrl)(int* sinking);

    /**
     * Send SinkVBUS or DisableSinkVBUS command
     *
     * @enable true for enable, false for disable
     *
     * @return EC_SUCCESS or error
     */
    int (*set_snk_ctrl)(int enable);

    /**
     * Request current sourcing state of the TCPC
     * NOTE: this is most useful for PPCs that can not tell on their own
     *
     * @param is_sourcing true for sourcing, false for not
     *
     * @return EC_SUCCESS, EC_ERROR_UNIMPLEMENTED or error
     */
    int (*get_src_ctrl)(int* sourcing);

    /**
     * Send SourceVBUS or DisableSourceVBUS command
     *
     * @enable true for enable, false for disable
     *
     * @return EC_SUCCESS or error
     */
    int (*set_src_ctrl)(int enable);
#endif

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
    /**
     * Instructs the TCPC to enter into low power mode.
     *
     * NOTE: Do no use tcpc_(read|write) style helper methods in this
     * function. You must use i2c_(read|write) directly.
     *
     *
     * @return EC_SUCCESS or error
     */
    int (*enter_low_power_mode)();
#endif

#ifdef CONFIG_USB_PD_FRS_TCPC
    /**
     * Enable/Disable TCPC FRS detection
     *
     * @param enable FRS enable (true) disable (false)
     *
     * @return EC_SUCCESS or error
     */
    int (*set_frs_enable)(int enable);
#endif

    /**
     * Handle TCPCI Faults
     *
     * @param port Type-C port number
     * @param fault TCPCI fault status value
     *
     * @return EC_SUCCESS or error
     */
    int (*handle_fault)(int fault);

#ifdef CONFIG_CMD_TCPC_DUMP
    /**
     * Dump TCPC registers
     */
    void (*dump_registers)();
#endif /* defined(CONFIG_CMD_TCPC_DUMP) */
  };

  enum tcpc_alert_polarity
  {
    TCPC_ALERT_ACTIVE_LOW,
    TCPC_ALERT_ACTIVE_HIGH,
  };

  struct tcpc_config_t
  {
    int i2c_host_port;
    int i2c_slave_addr;
    const struct tcpm_drv* drv;
    enum tcpc_alert_polarity pol;
  };

  /**
   * Returns whether the sink has detected a Rp resistor on the other side.
   */
  static inline int cc_is_rp(enum tcpc_cc_voltage_status cc)
  {
    return (cc == TYPEC_CC_VOLT_RP_DEF) || (cc == TYPEC_CC_VOLT_RP_1_5) || (cc == TYPEC_CC_VOLT_RP_3_0);
  }

  /**
   * Returns true if both CC lines are completely open.
   */
  static inline int cc_is_open(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
  {
    return cc1 == TYPEC_CC_VOLT_OPEN && cc2 == TYPEC_CC_VOLT_OPEN;
  }

  /**
   * Returns true if we detect the port partner is a snk debug accessory.
   */
  static inline int cc_is_snk_dbg_acc(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
  {
    return cc1 == TYPEC_CC_VOLT_RD && cc2 == TYPEC_CC_VOLT_RD;
  }

  static inline uint8_t board_get_src_dts_polarity()
  {
    /*
     * If the port in SRC DTS, the polarity is determined by the board,
     * i.e. what Rp impedance the CC lines are pulled. If this function
     * is not overridden, assume CC1 is primary.
     */
    return 0;
  }

  /**
   * Returns true if we detect the port partner is a src debug accessory.
   */
  static inline int cc_is_src_dbg_acc(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
  {
    return cc_is_rp(cc1) && cc_is_rp(cc2);
  }

  /**
   * Returns true if the port partner is an audio accessory.
   */
  static inline int cc_is_audio_acc(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
  {
    return cc1 == TYPEC_CC_VOLT_RA && cc2 == TYPEC_CC_VOLT_RA;
  }

  /**
   * Returns true if the port partner is presenting at least one Rd
   */
  static inline int cc_is_at_least_one_rd(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
  {
    return cc1 == TYPEC_CC_VOLT_RD || cc2 == TYPEC_CC_VOLT_RD;
  }

  /**
   * Returns true if the port partner is presenting Rd on only one CC line.
   */
  static inline int cc_is_only_one_rd(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
  {
    return cc_is_at_least_one_rd(cc1, cc2) && cc1 != cc2;
  }

  /**
   * Returns the PD_STATUS_TCPC_ALERT_* mask corresponding to the TCPC ports
   * that are currently asserting ALERT.
   *
   * @return     PD_STATUS_TCPC_ALERT_* mask.
   */
  uint16_t tcpc_get_alert_status(void);

  /**
   * Optional, set the TCPC power mode.
   *
   * @param port Type-C port number
   * @param mode 0: off/sleep, 1: on/awake
   */
  void board_set_tcpc_power_mode(int mode) __attribute__((weak));

  /**
   * Initialize TCPC.
   *
   * @param port Type-C port number
   */
  void tcpc_init();

  /**
   * TCPC is asserting alert
   *
   * @param port Type-C port number
   */
  void tcpc_alert_clear();

  /**
   * Run TCPC task once. This checks for incoming messages, processes
   * any outgoing messages, and reads CC lines.
   *
   * @param port Type-C port number
   * @param evt Event type that woke up this task
   */
  int tcpc_run(int evt);

  /**
   * Initialize board specific TCPC functions post TCPC initialization.
   *
   * @param port Type-C port number
   *
   * @return EC_SUCCESS or error
   */
  int board_tcpc_post_init() __attribute__((weak));

#ifdef __cplusplus
}
#endif

#endif /* __CROS_EC_USB_PD_TCPM_H */
