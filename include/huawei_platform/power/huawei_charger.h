/*
 * drivers/power/huawei_charger.h
 *
 *huawei charger driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/device.h>    /*for struct charge_device_info*/
#include <linux/notifier.h>    /*for struct charge_device_info*/
#include <linux/workqueue.h>    /*for struct charge_device_info*/
#include <linux/power_supply.h>    /*for struct charge_device_info*/
#include <huawei_platform/dsm/dsm_pub.h>
#ifndef _HUAWEI_CHARGER
#define _HUAWEI_CHARGER

/*************************marco define area***************************/
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#define ERROR_FCP_VOL_OVER_HIGH             (10421)
#define ERROR_FCP_DETECT             (10422)
#define ERROR_FCP_OUTPUT             (10423)
#define ERROR_SWITCH_ATTACH             (10424)
#define ERROR_ADAPTER_OVLT             (10425)
#define ERROR_ADAPTER_OCCURRENT             (10426)
#define ERROR_ADAPTER_OTEMP             (10427)
#define get_index(x) (x-ERROR_FCP_VOL_OVER_HIGH)
#define ERR_NO_STRING_SIZE 128
#define CHARGELOG_SIZE      (1024)
/*options of charge current(include input current & charge into battery current)*/
#define CHARGE_CURRENT_0000_MA            (0)
#define CHARGE_CURRENT_0500_MA            (500)
#define CHARGE_CURRENT_0800_MA            (800)
#define CHARGE_CURRENT_1000_MA            (1000)
#define CHARGE_CURRENT_1200_MA            (1200)
#define CHARGE_CURRENT_1900_MA            (1900)
#define CHARGE_CURRENT_2000_MA            (2000)
#define CHARGE_CURRENT_4000_MA            (4000)
#define CHARGE_CURRENT_MAX_MA            (32767)

/*options of battery voltage*/
#define BATTERY_VOLTAGE_0000_MV             (0)
#define BATTERY_VOLTAGE_0200_MV             (200)

#define BATTERY_VOLTAGE_MIN_MV              (-32767)
#define BATTERY_VOLTAGE_MAX_MV              (32767)
#define BATTERY_VOLTAGE_3200_MV             (3200)
#define BATTERY_VOLTAGE_3400_MV             (3400)
#define BATTERY_VOLTAGE_4100_MV             (4200)
#define BATTERY_VOLTAGE_4200_MV             (4200)
#define BATTERY_VOLTAGE_4350_MV             (4350)
#define BATTERY_VOLTAGE_4500_MV             (4500)

/*options of NTC battery temperature*/
#define BATTERY_TEMPERATURE_MIN             (-32767)
#define BATTERY_TEMPERATURE_MAX             (32767)
#define BATTERY_TEMPERATURE_0_C             (0)
#define BATTERY_TEMPERATURE_5_C             (5)

#define CHARGING_WORK_TIMEOUT                (30000)
#define MIN_CHARGING_CURRENT_OFFSET          (-10)
#define BATTERY_FULL_CHECK_TIMIES            (2)

#define WATCHDOG_TIMER_DISABLE     (0)
#define WATCHDOG_TIMER_40_S     (40)
#define WATCHDOG_TIMER_80_S     (80)

/*options of charge voltage (for dpm voltage setting,also ovp & uvp protect)*/
#define CHARGE_VOLTAGE_4360_MV    (4360)
#define CHARGE_VOLTAGE_4520_MV    (4520)
#define CHARGE_VOLTAGE_4600_MV    (4600)
#define CHARGE_VOLTAGE_5000_MV    (5000)
#define CHARGE_VOLTAGE_6300_MV    (6300)
#define CHARGE_VOLTAGE_6500_MV    (6500)

/*options of charge states from chip*/
#define CHAGRE_STATE_NORMAL              (0x00)
#define CHAGRE_STATE_VBUS_OVP           (0x01)
#define CHAGRE_STATE_NOT_PG               (0x02)
#define CHAGRE_STATE_WDT_FAULT         (0x04)
#define CHAGRE_STATE_BATT_OVP           (0x08)
#define CHAGRE_STATE_CHRG_DONE         (0x10)
#define CHAGRE_STATE_INPUT_DPM          (0x20)

/*options of vbus voltage*/
#define VBUS_VOLTAGE_FCP_MIN_MV              (7000)
#define VBUS_VOLTAGE_NON_FCP_MAX_MV          (6500)
#define VBUS_VOLTAGE_ABNORMAL_MAX_COUNT (2)


/*************************struct define area***************************/
enum usb_charger_type{
    CHARGER_TYPE_USB = 0,      //SDP
    CHARGER_TYPE_BC_USB,       //CDP
    CHARGER_TYPE_NON_STANDARD, //UNKNOW
    CHARGER_TYPE_STANDARD,     //DCP
    CHARGER_TYPE_FCP,          //FCP
    CHARGER_REMOVED,           //not connected
    USB_EVENT_OTG_ID,
};

enum charge_fault_type{
    CHARGE_FAULT_NON = 0,
    CHARGE_FAULT_BOOST_OCP,
    CHARGE_FAULT_TOTAL,
};

enum charge_sysfs_type{
    CHARGE_SYSFS_IIN_THERMAL = 0,
    CHARGE_SYSFS_ICHG_THERMAL,
    CHARGE_SYSFS_IIN_RUNNINGTEST,
    CHARGE_SYSFS_ICHG_RUNNINGTEST,
    CHARGE_SYSFS_ENABLE_CHARGER,
    CHARGE_SYSFS_LIMIT_CHARGING,
    CHARGE_SYSFS_REGULATION_VOLTAGE,
    CHARGE_SYSFS_BATFET_DISABLE,
    CHARGE_SYSFS_WATCHDOG_DISABLE,
    CHARGE_SYSFS_CHARGELOG,
    CHARGE_SYSFS_CHARGELOG_HEAD,
    CHARGE_SYSFS_IBUS,
    CHARGE_SYSFS_HIZ,
    CHARGE_SYSFS_CHARGE_TYPE,
#ifdef CONFIG_GRACE_SELECT_2A_1A_AC
    CHARGE_SYSFS_SELECTED_AC,
#endif
};

enum fcp_check_stage_type{
    FCP_STAGE_DEFAUTL,
    FCP_STAGE_SUPPORT_DETECT,
    FCP_STAGE_ADAPTER_DETECT,
    FCP_STAGE_ADAPTER_ENABLE,
    FCP_STAGE_SUCESS,
    FCP_STAGE_CHARGE_DONE,
};
enum fcp_retry_operate_type{
    FCP_RETRY_OPERATE_DEFAUTL,
    FCP_RETRY_OPERATE_RESET_ADAPTER,
    FCP_RETRY_OPERATE_RESET_FSA9688,
    FCP_RETRY_OPERATE_UNVALID,
};
static const char* const fcp_check_stage[] = {
    [0] = "FCP_STAGE_DEFAUTL",
    [1] = "FCP_STAGE_SUPPORT_DETECT",
    [2] = "FCP_STAGE_ADAPTER_DETECT",
    [3] = "FCP_STAGE_ADAPTER_ENABLE",
    [4] = "FCP_STAGE_SUCESS",
};

struct charge_sysfs_data{
    unsigned int iin_thl;
    unsigned int ichg_thl;
    unsigned int iin_rt;
    unsigned int ichg_rt;
    unsigned int vterm_rt;
    unsigned int charge_limit;
    unsigned int wdt_disable;
    unsigned int charge_enable;
    unsigned int batfet_disable;
    unsigned int hiz_enable;
    int ibus;
    struct mutex dump_reg_lock;
    struct mutex dump_reg_head_lock;
    char reg_value[CHARGELOG_SIZE];
    char reg_head[CHARGELOG_SIZE];
#ifdef CONFIG_GRACE_SELECT_2A_1A_AC
    unsigned int selected_AC;
#endif
};

struct charge_core_data{
    unsigned int iin;
    unsigned int ichg;
    unsigned int vterm;
    unsigned int iin_ac;
    unsigned int ichg_ac;
    unsigned int iin_usb;
    unsigned int ichg_usb;
    unsigned int iin_nonstd;
    unsigned int ichg_nonstd;
    unsigned int iin_bc_usb;
    unsigned int ichg_bc_usb;
    unsigned int iin_fcp;
    unsigned int ichg_fcp;
    unsigned int iterm;
    unsigned int vdpm;
    unsigned int iin_max;
    unsigned int ichg_max;
    unsigned int otg_curr;
};

struct charge_device_ops{
    int (*chip_init)(void);
    int (*set_input_current)(int value);
    int (*set_charge_current)(int value);
    int (*set_terminal_voltage)(int value);
    int (*set_dpm_voltage)(int value);
    int (*set_terminal_current)(int value);
    int (*set_charge_enable)(int enable);
    int (*set_otg_enable)(int enable);
    int (*set_term_enable)(int enable);
    int (*get_charge_state)(unsigned int *state);
    int (*reset_watchdog_timer)(void);
    int (*dump_register)(char *reg_value);
    int (*get_register_head)(char *reg_head);
    int (*set_watchdog_timer)(int value);
    int (*set_batfet_disable)(int disable);
    int (*get_ibus)(void);
    int (*get_vbus)(unsigned int *value);
    int (*check_charger_plugged)(void);
    int (*check_input_dpm_state)(void);
    int (*fcp_chip_init)(void);
    int  (*set_covn_start)(int enable);
    int (*set_charger_hiz)(int enable);
    int (*set_otg_current)(int value);
    int (*stop_charge_config)(void);
};

struct fcp_adapter_device_ops{
    int (*get_adapter_output_current)(void);
    int (*set_adapter_output_vol)(void);
    int (*detect_adapter)(void);
    int (*is_support_fcp)(void);
    int (*switch_chip_reset)(void);
    int (*fcp_adapter_reset)(void);
};
struct charge_device_info{
    struct device   *dev;
    struct notifier_block   usb_nb;
    struct notifier_block   fault_nb;
    struct delayed_work   charge_work;
    struct delayed_work   otg_work;
    struct work_struct      usb_work;
    struct work_struct      fault_work;
    struct charge_device_ops *ops;
    struct fcp_adapter_device_ops *fcp_ops;
    struct charge_core_data *core_data;
    struct charge_sysfs_data sysfs_data;

    enum usb_charger_type charger_type;
    enum power_supply_type charger_source;
    enum charge_fault_type charge_fault;
    unsigned int charge_enable;
    unsigned int input_current;
    unsigned int charge_current;
    unsigned int check_full_count;
#ifdef CONFIG_GRACE_SELECT_2A_1A_AC
    unsigned int pre_start_select_AC;
    unsigned int start_select_AC;
#endif
};

/****************variable and function declarationn area******************/
extern struct atomic_notifier_head fault_notifier_list;
extern struct device *charge_dev;
int charge_ops_register(struct charge_device_ops *ops);
int charge_check_charger_plugged(void);
int charge_check_input_dpm_state(void);
int fcp_adapter_ops_register(struct fcp_adapter_device_ops *ops);

#endif
