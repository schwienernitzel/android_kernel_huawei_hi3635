

#ifndef ATFDRIVER_H_
#define ATFDRIVER_H_

#include <linux/init.h>

#define ACCESS_REGISTER_FN_MAIN_ID              (0xc500aa01)
#define ACCESS_REGISTER_FN_SUB_ID_DDR_INTLV     (0x55bbcce0)
#define ACCESS_REGISTER_FN_SUB_ID_DDR_FLUX_W    (0x55bbcce1)
#define ACCESS_REGISTER_FN_SUB_ID_DDR_FLUX_R    (0x55bbcce2)
#define ACCESS_REGISTER_FN_SUB_ID_DDR_DRAM_R    (0x55bbcce3)
#define ACCESS_REGISTER_FN_SUB_ID_DDR_STDID_W   (0x55bbcce4)
#define ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG    (0x55bbcce5)
noinline int atfd_hisi_fn_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
noinline int atfd_hisi_service_tsp_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
noinline int atfd_hisi_service_access_register_smc(u64 main_fun_id, u64 buff_addr_phy, u64 data_len, u64 sub_fun_id);
void configure_master_security(unsigned int is_security, int master_id);
void configure_dss_register_security(uint32_t addr, uint32_t val, uint8_t bw, uint8_t bs);

typedef enum _master_id_type_ {
    MASTER_VDEC_ID = 0,
    MASTER_VENC_ID,
    MASTER_DSS_ID,
    MASTER_ID_MAX
} MASTER_ID_TYPE;

#endif /* ATFDRIVER_H_ */
