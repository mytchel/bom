/*
 * Largely taken from minix's beagle port at
 * /minix/drivers/storage/mmc/omap_mmc.h
 * with some changes.
 */

#ifndef _OMAP_MMC_H_
#define _OMAP_MMC_H_

struct mmchs_regs {
  uint32_t pad[68];
  uint32_t sysconfig;
  uint32_t sysstatus;
  uint32_t pad1[3];
  uint32_t csre;
  uint32_t systest;
  uint32_t con;
  uint32_t pwcnt;
  uint32_t pad2[51];
  uint32_t sdmasa;
  uint32_t blk;
  uint32_t arg;
  uint32_t cmd;
  uint32_t rsp10;
  uint32_t rsp32;
  uint32_t rsp54;
  uint32_t rsp76;
  uint32_t data;
  uint32_t pstate;
  uint32_t hctl;
  uint32_t sysctl;
  uint32_t stat;
  uint32_t ie;
  uint32_t ise;
  uint32_t ac12;
  uint32_t capa;
  uint32_t pad3[1];
  uint32_t curcapa;
  uint32_t pad4[1];
  uint32_t fe;
  uint32_t admaes;
  uint32_t admasal;
  uint32_t admasah;
  uint32_t pad5[39];
  uint32_t rev;
};

#define MMCHS_SD_SYSCONFIG_AUTOIDLE                    (0x1 << 0)  /* Internal clock gating strategy */
#define MMCHS_SD_SYSCONFIG_SOFTRESET                   (0x1 << 1)  /* Software reset bit writing  */
#define MMCHS_SD_SYSCONFIG_ENAWAKEUP                   (0x1 << 2)  /* Wake-up feature control */
#define MMCHS_SD_SYSCONFIG_SIDLEMODE                   (0x3 << 3)  /* Power management */
#define MMCHS_SD_SYSCONFIG_SIDLEMODE_UNCONDITIONAL     (0x0 << 3)  /* Go into idle mode unconditionally upon request */
#define MMCHS_SD_SYSCONFIG_SIDLEMODE_IGNORE            (0x1 << 3)  /* Ignore ILDE requests */
#define MMCHS_SD_SYSCONFIG_SIDLEMODE_IDLE              (0x2 << 3)  /* Acknowledge IDLE request switch to wake-up mode */
#define MMCHS_SD_SYSCONFIG_SIDLEMODE_SMART_IDLE        (0x3 << 3)  /* Smart-idle */
#define MMCHS_SD_SYSCONFIG_CLOCKACTIVITY               (0x3 << 8)  /* Clock activity during wake-up */
#define MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_OFF           (0x0 << 8)  /* Interface and functional clock can be switched off */
#define MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_IF            (0x1 << 8)  /* Only Interface clock (functional can be switched off*/
#define MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_FUNC          (0x2 << 8)  /* Only Functional clock (interface clock can be switched off) */
#define MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_BOOTH         (0x3 << 8)  /* Booth the interface and functional clock are maintained */
#define MMCHS_SD_SYSCONFIG_STANDBYMODE                 (0x3 << 12) /* Configuration for standby */
#define MMCHS_SD_SYSCONFIG_STANDBYMODE_FORCE_STANDBY   (0x0 << 12) /* Force standby mode upon idle request*/
#define MMCHS_SD_SYSCONFIG_STANDBYMODE_NO_STANDBY      (0x1 << 12) /* Never go into standby mode */
#define MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_INTERNAL (0x2 << 12) /* Go into wake-up mode based on internal knowledge */
#define MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_SMART    (0x3 << 12) /* Go info wake-up mode when possible */

#define MMCHS_SD_SYSSTATUS_RESETDONE 0x01

#define MMCHS_SD_CON_DW8          (0x1 << 5) /* 8-bit mode MMC select , For SD clear this bit */
#define MMCHS_SD_CON_DW8_1BIT     (0x0 << 5) /* 1 or 4 bits data width configuration(also set SD_HCTL) */
#define MMCHS_SD_CON_DW8_8BITS    (0x1 << 5) /* 8 bits data width configuration */
#define MMCHS_SD_CON_INIT         (0x1 << 1) /* Send initialization stream (all cards) */
#define MMCHS_SD_CON_OD           (0x1 << 0) /* Card open drain mode (MMC cards only) */
#define MMCHS_SD_CON_MODE         (0x1 << 4) /* Mode select */

#define MMCHS_SD_BLK_NBLK             (0xffffu << 16) /* Block count for the current transfer */
#define MMCHS_SD_BLK_BLEN             (0xfff << 0)     /* Transfer block size */
#define MMCHS_SD_BLK_BLEN_NOTRANSFER  (0x0 << 0)       /* No transfer */

#define MMCHS_SD_CMD_INDEX                (0x3f << 24) /* Command index */
#define MMCHS_SD_CMD_INDEX_CMD(x)         (x << 24)    /* MMC command index binary encoded values from 0 to 63 */

#define MMCHS_SD_ARG_MASK                 (0xffffffffu)      /* Mask everything */
#define MMCHS_SD_ARG_CMD8_VHS             (0x1 << (16 - 8))  /* Voltage between 2.7 and 3.6 v*/
#define MMCHS_SD_ARG_CMD8_CHECK_PATTERN   (0xaa <<(8 - 8))   /* 10101010b pattern */

#define MMCHS_SD_CMD_TYPE                 (0x3 << 22) /* Command type. */
#define MMCHS_SD_CMD_TYPE_OTHER           (0x0 << 22) /* Other type of commands (like go idle) */
#define MMCHS_SD_CMD_TYPE_BUS_SUSPEND     (0x1 << 22) /* Upon CMD52 "Bus Suspend" operation */
#define MMCHS_SD_CMD_TYPE_FUNCTION_SELECT (0x2 << 22) /* Upon CMD52 "Function Select" operation */
#define MMCHS_SD_CMD_TYPE_IOABORT         (0x3 << 22) /* Upon CMD12 and CMD21 "I/O Abort */
#define MMCHS_SD_CMD_DP                   (0x1 << 21) /* Data present select */
#define MMCHS_SD_CMD_DP_DATA              (0x1 << 21) /* Additional data is present on the data lines */
#define MMCHS_SD_CMD_DP_NODATA            (0x0 << 21) /* No additional data is present on the data lines */
#define MMCHS_SD_CMD_CICE                 (0x1 << 20) /* Command index response check enable */
#define MMCHS_SD_CMD_CICE_ENABLE          (0x1 << 20) /* Enable index check response  */
#define MMCHS_SD_CMD_CICE_DISABLE         (0x0 << 20) /* Disable index check response */
#define MMCHS_SD_CMD_CCCE                 (0x1 << 19) /* Command CRC7 Check enable on responses*/
#define MMCHS_SD_CMD_CCCE_ENABLE          (0x1 << 19) /* Enable CRC7 Check on response */
#define MMCHS_SD_CMD_CCCE_DISABLE         (0x0 << 19) /* Disable CRC7 Check on response */
#define MMCHS_SD_CMD_RSP_TYPE             (0x3 << 16) /* Response type */
#define MMCHS_SD_CMD_RSP_TYPE_NO_RESP     (0x0 << 16) /* No response */
#define MMCHS_SD_CMD_RSP_TYPE_136B        (0x1 << 16) /* Response length 136 bits */
#define MMCHS_SD_CMD_RSP_TYPE_48B         (0x2 << 16) /* Response length 48 bits */
#define MMCHS_SD_CMD_RSP_TYPE_48B_BUSY    (0x3 << 16) /* Response length 48 bits with busy after response */
#define MMCHS_SD_CMD_MSBS                 (0x1 << 5)  /* Multi/Single block select */
#define MMCHS_SD_CMD_MSBS_SINGLE          (0x0 << 5)  /* Single block mode */
#define MMCHS_SD_CMD_MSBS_MULTI           (0x0 << 5)  /* Multi block mode */
#define MMCHS_SD_CMD_DDIR                 (0x1 << 4)  /* Data transfer direction */
#define MMCHS_SD_CMD_DDIR_READ            (0x1 << 4)  /* Data read (card to host) */
#define MMCHS_SD_CMD_DDIR_WRITE           (0x0 << 4)  /* Data write (host to card)  */
#define MMCHS_SD_CMD_ACEN                 (0x1 << 2)  /* Auto CMD12 Enable */
#define MMCHS_SD_CMD_ACEN_DIS             (0x0 << 2)  /* Auto CMD12 Disable */
#define MMCHS_SD_CMD_ACEN_EN              (0x1 << 2)  /* Auto CMD12 Enable */
#define MMCHS_SD_CMD_BCE                  (0x1 << 1)  /* Block Count Enable(for multi block transfer) */
#define MMCHS_SD_CMD_BCE_DIS              (0x0 << 1)  /* Disabled block count for infinite transfer*/
#define MMCHS_SD_CMD_BCE_EN               (0x1 << 1)  /* Enabled for multi block transfer with know amount of blocks */
#define MMCHS_SD_CMD_DE                   (0x1 << 0)  /* DMA enable */
#define MMCHS_SD_CMD_DE_DIS               (0x0 << 0)  /* Disable DMA */
#define MMCHS_SD_CMD_DE_EN                (0x1 << 0)  /* Enable DMA  */
#define MMCHS_SD_CMD_MASK  				   ~(0x1 << 30  | 0x1 << 31 | 0x1 << 18 | 0x1 <<3) /* bits 30 , 31 and 18 are reserved */

#define MMCHS_SD_PSTATE_CDP          (0x1 << 18) /* Card detect pin */
#define MMCHS_SD_PSTATE_CSS          (0x1 << 17) /* Card state stable */
#define MMCHS_SD_PSTATE_CI           (0x1 << 16) /* Card Inserted */
#define MMCHS_SD_PSTATE_CI_INSERTED  (0x1 << 16) /* Card Inserted  is inserted*/
#define MMCHS_SD_PSTATE_BRE          (0x1 << 11) /* Buffer read enable */
#define MMCHS_SD_PSTATE_BRE_DIS      (0x0 << 11) /* Read BLEN bytes disabled*/
#define MMCHS_SD_PSTATE_BRE_EN       (0x1 << 11) /* Read BLEN bytes enabled*/
#define MMCHS_SD_PSTATE_BWE          (0x1 << 10) /* Buffer Write enable */
#define MMCHS_SD_PSTATE_BWE_DIS      (0x0 << 10) /* There is no room left in the buffer to write BLEN bytes of data */
#define MMCHS_SD_PSTATE_BWE_EN       (0x1 << 10) /* There is enough space in the buffer to write BLEN bytes of data*/
#define MMCHS_SD_PSTATE_DATI         (0x1 <<  1) /* Command inhibit (mmc_dat) */
#define MMCHS_SD_PSTATE_CMDI         (0x1 <<  0) /* Command inhibit (mmc_cmd) */

#define MMCHS_SD_HCTL_DTW            (0x1 << 1) /*Data transfer width.(must be set after a successful ACMD6) */
#define MMCHS_SD_HCTL_DTW_1BIT       (0x0 << 1) /*1 bit transfer with */
#define MMCHS_SD_HCTL_DTW_4BIT       (0x1 << 1) /*4 bit transfer with */
#define MMCHS_SD_HCTL_SDBP           (0x1 << 8) /*SD bus power */
#define MMCHS_SD_HCTL_SDVS           (0x7 << 9) /*SD bus voltage select */
#define MMCHS_SD_HCTL_SDVS_VS18      (0x5 << 9) /*1.8 V */
#define MMCHS_SD_HCTL_SDVS_VS30      (0x6 << 9) /*3.0 V */
#define MMCHS_SD_HCTL_SDVS_VS33      (0x7 << 9) /*3.3 V */
#define MMCHS_SD_HCTL_IWE            (0x1 << 24)/* wake-up event on SD interrupt */
#define MMCHS_SD_HCTL_REM            (0x1 << 26)/* wake-up event on SD removal */
#define MMCHS_SD_HCTL_INS            (0x1 << 25)/* wake-up event on SD insersion */

#define MMCHS_SD_SYSCTL_CLKD (0x3ff << 6)  /* 10 bits clock frequency select */
#define MMCHS_SD_SYSCTL_SRD  (0x1   << 26)  /* Soft reset for mmc_dat line */
#define MMCHS_SD_SYSCTL_SRC  (0x1   << 25)  /* Soft reset for mmc_cmd line */
#define MMCHS_SD_SYSCTL_SRA  (0x1   << 24)  /* Soft reset all (host controller) */

#define MMCHS_SD_SYSCTL_ICE     (0x1 << 0) /* Internal clock enable register  */
#define MMCHS_SD_SYSCTL_ICS          (0x1 << 1) /* Internal clock stable register  */
#define MMCHS_SD_SYSCTL_ICS_UNSTABLE (0x0 << 1) /* Internal clock is unstable */
#define MMCHS_SD_SYSCTL_ICS_STABLE   (0x1 << 1) /* Internal clock is stable   */
#define MMCHS_SD_SYSCTL_CEN          (0x1 << 2) /* Card lock enable provide clock to the card */
#define MMCHS_SD_SYSCTL_CEN_DIS      (0x0 << 2) /* Internal clock is unstable */
#define MMCHS_SD_SYSCTL_CEN_EN       (0x1 << 2) /* Internal clock is stable   */

#define MMCHS_SD_SYSCTL_DTO          (0xf << 16) /* Data timeout counter  */
#define MMCHS_SD_SYSCTL_DTO_2POW13   (0x0 << 16) /* TCF x 2^13  */
#define MMCHS_SD_SYSCTL_DTO_2POW14   (0x1 << 16) /* TCF x 2^14  */
#define MMCHS_SD_SYSCTL_DTO_2POW20   (0x7 << 16) /* TCF x 2^20  */
#define MMCHS_SD_SYSCTL_DTO_2POW27   (0xe << 16) /* TCF x 2^27  */

#define MMCHS_SD_STAT_CERR            (0x1 << 28) /* card error */
#define MMCHS_SD_STAT_DEB             (0x1 << 22) /* data end bit error */
#define MMCHS_SD_STAT_DCRC            (0x1 << 21) /* data CRC error */
#define MMCHS_SD_STAT_DTO             (0x1 << 20) /* data timeout error */
#define MMCHS_SD_STAT_CIE             (0x1 << 19) /* command index error */
#define MMCHS_SD_STAT_CEB             (0x1 << 18) /* command end bit error */
#define MMCHS_SD_STAT_CCRC            (0x1 << 17) /* command CRC error */
#define MMCHS_SD_STAT_CTO             (0x1 << 16) /* command timeout error */
#define MMCHS_SD_STAT_ERRI            (0x01 << 15) /* Error interrupt */
#define MMCHS_SD_STAT_ERROR_MASK      (0xff << 15 | 0x3 << 24 | 0x03 << 28)
#define MMCHS_SD_STAT_CINS            (0x1 << 6) /* Card inserted */
#define MMCHS_SD_STAT_BRR             (0x1 << 5) /* Buffer Read ready */
#define MMCHS_SD_STAT_BWR             (0x1 << 4) /* Buffer Write ready */
#define MMCHS_SD_STAT_CC              (0x1 << 0) /* Command complete status */
#define MMCHS_SD_STAT_CC_UNRAISED     (0x0 << 0) /* Command not completed */
#define MMCHS_SD_STAT_CC_RAISED       (0x1 << 0) /* Command completed */
#define MMCHS_SD_STAT_TC              (0x1 << 1) /* Transfer complete status */
#define MMCHS_SD_STAT_TC_UNRAISED     (0x0 << 1) /* Transfer not completed */
#define MMCHS_SD_STAT_TC_RAISED       (0x1 << 1) /* Transfer completed */

#define MMCHS_SD_IE_ERROR_MASK        (0xff << 15 | 0x3 << 24 | 0x03 << 28)

#define MMCHS_SD_IE_CC                (0x1 << 0) /* Command complete interrupt enable */
#define MMCHS_SD_IE_TC                (0x1 << 1) /* Transfer complete interrupt enable */
#define MMCHS_SD_IE_BRR               (0x1 << 5) /* Buffer read ready interrupt  */
#define MMCHS_SD_IE_BWR               (0x1 << 4) /* Buffer write ready interrupt  */
#define MMCHS_SD_IE_CINS              (0x1 << 6) /* Buffer write ready interrupt  */
#define MMCHS_SD_IE_CREM              (0x1 << 7) /* Buffer write ready interrupt  */

#define MMCHS_SD_CAPA_VS_MASK (0x7 << 24 )  /* voltage mask */
#define MMCHS_SD_CAPA_VS18 (0x01 << 26 )  /* 1.8 volt */
#define MMCHS_SD_CAPA_VS30 (0x01 << 25 )  /* 3.0 volt */
#define MMCHS_SD_CAPA_VS33 (0x01 << 24 )  /* 3.3 volt */

#endif
