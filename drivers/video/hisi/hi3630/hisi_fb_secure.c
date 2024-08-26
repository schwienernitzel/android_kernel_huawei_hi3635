/* Copyright (c) 2014-2015, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_fb.h"
extern void configure_dss_register_security(uint32_t addr, uint32_t val, uint8_t bw, uint8_t bs);
static void hisifd_secure_set_reg(uint32_t addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	configure_dss_register_security(addr, val, bw, bs);
}

/* for DRM config */
static void hisifd_secure_layer_config(struct hisi_fb_data_type *hisifd, dss_layer_t *layer)
{
	struct hisifb_secure *secure_ctrl = NULL;

	BUG_ON(hisifd == NULL || layer == NULL);
	secure_ctrl = &(hisifd->secure_ctrl);
	BUG_ON(secure_ctrl == NULL);

	//dma rch secure
	secure_ctrl->set_reg(HISI_DSS_BASE + DSS_DP_CTRL_OFFSET + PDP_DMA_SECU_EN, 0x1, 1, layer->chn_idx);
	//mmu rch secure
	secure_ctrl->set_reg(HISI_DSS_BASE + DSS_DP_CTRL_OFFSET + PDP_MMU_SECU_EN, 0x1, 1, layer->chn_idx);

	//rot_tlb_secu
	#if 0
	if(layer->need_cap & CAP_ROT) {
	    secure_ctrl->set_reg(HISI_DSS_BASE + DSS_TOP_CTRL_OFFSET + ROT_TLB_SECU_EN, 0x1, 1, 0);
    }
    #endif
}
static void hisifd_secure_layer_deconfig(struct hisi_fb_data_type *hisifd, dss_layer_t *layer)
{
	struct hisifb_secure *secure_ctrl = NULL;

	BUG_ON(hisifd == NULL || layer == NULL);
	secure_ctrl = &(hisifd->secure_ctrl);
	BUG_ON(secure_ctrl == NULL);

	//dma rch secure
	secure_ctrl->set_reg(HISI_DSS_BASE + DSS_DP_CTRL_OFFSET + PDP_DMA_SECU_DIS, 0x1, 1, layer->chn_idx);
	//mmu rch secure
	secure_ctrl->set_reg(HISI_DSS_BASE + DSS_DP_CTRL_OFFSET + PDP_MMU_SECU_DIS, 0x1, 1, layer->chn_idx);

	//rot_tlb_secu
	//secure_ctrl->set_reg(HISI_DSS_BASE + DSS_TOP_CTRL_OFFSET + ROT_TLB_SECU_DIS, 0x1, 1, 0);
}

void hisifb_secure_register(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_secure *secure_ctrl = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);
	secure_ctrl = &(hisifd->secure_ctrl);
	BUG_ON(secure_ctrl == NULL);

	if (secure_ctrl->secure_created) {
		return;
	}

	secure_ctrl->set_reg = hisifd_secure_set_reg;
	secure_ctrl->secure_layer_config = hisifd_secure_layer_config;
	secure_ctrl->secure_layer_deconfig = hisifd_secure_layer_deconfig;
	secure_ctrl->hisifd = hisifd;

	secure_ctrl->secure_created = 1;

}

void hisifb_secure_unregister(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisifb_secure *secure_ctrl = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);
	secure_ctrl = &(hisifd->secure_ctrl);
	BUG_ON(secure_ctrl == NULL);

	if (!secure_ctrl->secure_created)
		return;

	secure_ctrl->secure_created = 0;
}
