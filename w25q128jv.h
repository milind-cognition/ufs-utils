/* SPDX-License-Identifier: MIT */
/*
 * w25q128jv.h – Portable driver for Winbond W25Q128JV SPI NOR flash.
 *
 * Covers every Standard-SPI instruction from datasheet Section 8
 * (Rev F, 03/27/2018) with worst-case timing from Section 9.
 * Platform independence is achieved through a HAL callback struct;
 * no hardware-specific code exists in this driver.
 */

#ifndef W25Q128JV_H
#define W25Q128JV_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/*  Device geometry                                                    */
/* ------------------------------------------------------------------ */
#define W25Q128JV_PAGE_SIZE          256U
#define W25Q128JV_SECTOR_SIZE        4096U          /* 4 KB  */
#define W25Q128JV_BLOCK_SIZE_32K     (32U * 1024U)  /* 32 KB */
#define W25Q128JV_BLOCK_SIZE_64K     (64U * 1024U)  /* 64 KB */
#define W25Q128JV_CHIP_SIZE          (16UL * 1024 * 1024) /* 16 MB */
#define W25Q128JV_NUM_PAGES          (W25Q128JV_CHIP_SIZE / W25Q128JV_PAGE_SIZE)
#define W25Q128JV_NUM_SECTORS        (W25Q128JV_CHIP_SIZE / W25Q128JV_SECTOR_SIZE)

/* ------------------------------------------------------------------ */
/*  Manufacturer / Device identification                               */
/* ------------------------------------------------------------------ */
#define W25Q128JV_MANUFACTURER_ID    0xEFU
#define W25Q128JV_DEVICE_ID          0x17U
#define W25Q128JV_JEDEC_MEM_TYPE_IQ  0x40U  /* IQ/JQ variant */
#define W25Q128JV_JEDEC_MEM_TYPE_IM  0x70U  /* IM*/JM* variant */
#define W25Q128JV_JEDEC_CAPACITY     0x18U

/* ------------------------------------------------------------------ */
/*  Instruction opcodes  (Section 8.1.2 – Standard SPI)                */
/* ------------------------------------------------------------------ */
#define W25Q_CMD_WRITE_ENABLE              0x06U
#define W25Q_CMD_VOLATILE_SR_WRITE_ENABLE  0x50U
#define W25Q_CMD_WRITE_DISABLE             0x04U

#define W25Q_CMD_READ_SR1                  0x05U
#define W25Q_CMD_READ_SR2                  0x35U
#define W25Q_CMD_READ_SR3                  0x15U
#define W25Q_CMD_WRITE_SR1                 0x01U
#define W25Q_CMD_WRITE_SR2                 0x31U
#define W25Q_CMD_WRITE_SR3                 0x11U

#define W25Q_CMD_READ_DATA                 0x03U
#define W25Q_CMD_FAST_READ                 0x0BU

#define W25Q_CMD_PAGE_PROGRAM              0x02U

#define W25Q_CMD_SECTOR_ERASE_4KB          0x20U
#define W25Q_CMD_BLOCK_ERASE_32KB          0x52U
#define W25Q_CMD_BLOCK_ERASE_64KB          0xD8U
#define W25Q_CMD_CHIP_ERASE                0xC7U
#define W25Q_CMD_CHIP_ERASE_ALT            0x60U

#define W25Q_CMD_ERASE_PROGRAM_SUSPEND     0x75U
#define W25Q_CMD_ERASE_PROGRAM_RESUME      0x7AU

#define W25Q_CMD_POWER_DOWN                0xB9U
#define W25Q_CMD_RELEASE_POWERDOWN_ID      0xABU
#define W25Q_CMD_MANUFACTURER_DEVICE_ID    0x90U
#define W25Q_CMD_JEDEC_ID                  0x9FU
#define W25Q_CMD_READ_UNIQUE_ID            0x4BU

#define W25Q_CMD_READ_SFDP                 0x5AU

#define W25Q_CMD_ERASE_SECURITY_REG        0x44U
#define W25Q_CMD_PROGRAM_SECURITY_REG      0x42U
#define W25Q_CMD_READ_SECURITY_REG         0x48U

#define W25Q_CMD_INDIVIDUAL_BLOCK_LOCK     0x36U
#define W25Q_CMD_INDIVIDUAL_BLOCK_UNLOCK   0x39U
#define W25Q_CMD_READ_BLOCK_LOCK           0x3DU
#define W25Q_CMD_GLOBAL_BLOCK_LOCK         0x7EU
#define W25Q_CMD_GLOBAL_BLOCK_UNLOCK       0x98U

#define W25Q_CMD_ENABLE_RESET              0x66U
#define W25Q_CMD_RESET_DEVICE              0x99U

/* ------------------------------------------------------------------ */
/*  Status-register bit masks                                          */
/* ------------------------------------------------------------------ */
/* SR-1 */
#define W25Q_SR1_BUSY  (1U << 0)
#define W25Q_SR1_WEL   (1U << 1)
#define W25Q_SR1_BP0   (1U << 2)
#define W25Q_SR1_BP1   (1U << 3)
#define W25Q_SR1_BP2   (1U << 4)
#define W25Q_SR1_TB    (1U << 5)
#define W25Q_SR1_SEC   (1U << 6)
#define W25Q_SR1_SRP   (1U << 7)
/* SR-2 */
#define W25Q_SR2_SRL   (1U << 0)
#define W25Q_SR2_QE    (1U << 1)
#define W25Q_SR2_LB1   (1U << 3)
#define W25Q_SR2_LB2   (1U << 4)
#define W25Q_SR2_LB3   (1U << 5)
#define W25Q_SR2_CMP   (1U << 6)
#define W25Q_SR2_SUS   (1U << 7)
/* SR-3 */
#define W25Q_SR3_WPS   (1U << 2)
#define W25Q_SR3_DRV0  (1U << 5)
#define W25Q_SR3_DRV1  (1U << 6)

/* ------------------------------------------------------------------ */
/*  Timing – worst-case (max) values from Section 9.6                  */
/* ------------------------------------------------------------------ */
#define W25Q_TIMEOUT_WRITE_SR_MS      15U       /* tW   max  */
#define W25Q_TIMEOUT_PAGE_PROG_MS      3U       /* tPP  max  */
#define W25Q_TIMEOUT_SECTOR_ERASE_MS 400U       /* tSE  max  */
#define W25Q_TIMEOUT_BLOCK32_ERASE_MS 1600U     /* tBE1 max  */
#define W25Q_TIMEOUT_BLOCK64_ERASE_MS 2000U     /* tBE2 max  */
#define W25Q_TIMEOUT_CHIP_ERASE_MS   200000UL   /* tCE  max  (200 s) */
#define W25Q_DELAY_POWER_DOWN_US       3U       /* tDP  max  */
#define W25Q_DELAY_RELEASE_PD_US       3U       /* tRES1 max */
#define W25Q_DELAY_RELEASE_PD_ID_US    2U       /* tRES2 max (1.8 µs) */
#define W25Q_DELAY_SUSPEND_US         20U       /* tSUS max  */
#define W25Q_DELAY_RESET_US           30U       /* tRST max  */

/* Typical timing (informational) */
#define W25Q_TYP_PAGE_PROG_US        400U       /* tPP  typ (0.4 ms) */
#define W25Q_TYP_SECTOR_ERASE_MS      45U       /* tSE  typ  */
#define W25Q_TYP_BLOCK32_ERASE_MS    120U       /* tBE1 typ  */
#define W25Q_TYP_BLOCK64_ERASE_MS    150U       /* tBE2 typ  */
#define W25Q_TYP_CHIP_ERASE_MS     40000UL      /* tCE  typ (40 s) */
#define W25Q_TYP_WRITE_SR_MS          10U       /* tW   typ  */

/* ------------------------------------------------------------------ */
/*  Security-register base addresses                                   */
/* ------------------------------------------------------------------ */
#define W25Q_SECURITY_REG1  0x001000UL
#define W25Q_SECURITY_REG2  0x002000UL
#define W25Q_SECURITY_REG3  0x003000UL

/* ------------------------------------------------------------------ */
/*  Error codes                                                        */
/* ------------------------------------------------------------------ */
typedef enum {
	W25Q_OK = 0,
	W25Q_ERR_SPI,
	W25Q_ERR_TIMEOUT,
	W25Q_ERR_PARAM,
	W25Q_ERR_BUSY,
} w25q_err_t;

/* ------------------------------------------------------------------ */
/*  HAL callback interface                                             */
/* ------------------------------------------------------------------ */
/**
 * struct w25q_hal – Platform abstraction callbacks.
 *
 * Every function pointer except @ctx must be non-NULL.
 * All callbacks receive @ctx as an opaque user pointer that
 * can carry platform-specific handles (SPI peripheral, GPIO, etc.).
 */
typedef struct {
	/**
	 * cs_assert – Drive /CS low (select the device).
	 * Return 0 on success, non-zero on failure.
	 */
	int (*cs_assert)(void *ctx);

	/**
	 * cs_deassert – Drive /CS high (deselect the device).
	 * Return 0 on success, non-zero on failure.
	 */
	int (*cs_deassert)(void *ctx);

	/**
	 * spi_xfer – Full-duplex SPI transfer of @len bytes.
	 *
	 * Simultaneously clock out @tx and clock in @rx.
	 *   - @tx == NULL  →  transmit 0xFF (dummy bytes).
	 *   - @rx == NULL  →  discard received data.
	 * Return 0 on success, non-zero on failure.
	 */
	int (*spi_xfer)(void *ctx, const uint8_t *tx,
			uint8_t *rx, size_t len);

	/**
	 * delay_us – Busy-wait or sleep for @us microseconds.
	 */
	void (*delay_us)(void *ctx, uint32_t us);

	/**
	 * tick_ms – Return a monotonic millisecond counter.
	 *
	 * Used for timeout detection during busy-wait polling.
	 * Wrap-around is handled internally.
	 */
	uint32_t (*tick_ms)(void *ctx);

	/** Opaque user context forwarded to every callback. */
	void *ctx;
} w25q_hal_t;

/* ------------------------------------------------------------------ */
/*  Device handle                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
	const w25q_hal_t *hal;
} w25q_dev_t;

/* ------------------------------------------------------------------ */
/*  Initialisation                                                     */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_init(w25q_dev_t *dev, const w25q_hal_t *hal);

/* ------------------------------------------------------------------ */
/*  Write-enable / disable  (8.2.1 – 8.2.3)                           */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_write_enable(w25q_dev_t *dev);
w25q_err_t w25q_volatile_sr_write_enable(w25q_dev_t *dev);
w25q_err_t w25q_write_disable(w25q_dev_t *dev);

/* ------------------------------------------------------------------ */
/*  Status registers  (8.2.4 – 8.2.5)                                 */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_read_sr1(w25q_dev_t *dev, uint8_t *sr);
w25q_err_t w25q_read_sr2(w25q_dev_t *dev, uint8_t *sr);
w25q_err_t w25q_read_sr3(w25q_dev_t *dev, uint8_t *sr);

w25q_err_t w25q_write_sr1(w25q_dev_t *dev, uint8_t val);
w25q_err_t w25q_write_sr2(w25q_dev_t *dev, uint8_t val);
w25q_err_t w25q_write_sr3(w25q_dev_t *dev, uint8_t val);

/* ------------------------------------------------------------------ */
/*  Read operations  (8.2.6 – 8.2.7)                                  */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_read(w25q_dev_t *dev, uint32_t addr,
		     uint8_t *buf, size_t len);
w25q_err_t w25q_fast_read(w25q_dev_t *dev, uint32_t addr,
			  uint8_t *buf, size_t len);

/* ------------------------------------------------------------------ */
/*  Page program  (8.2.13)                                             */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_page_program(w25q_dev_t *dev, uint32_t addr,
			     const uint8_t *data, size_t len);

/** Multi-page write – automatically splits across page boundaries. */
w25q_err_t w25q_write(w25q_dev_t *dev, uint32_t addr,
		      const uint8_t *data, size_t len);

/* ------------------------------------------------------------------ */
/*  Erase operations  (8.2.15 – 8.2.18)                               */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_sector_erase(w25q_dev_t *dev, uint32_t addr);
w25q_err_t w25q_block_erase_32k(w25q_dev_t *dev, uint32_t addr);
w25q_err_t w25q_block_erase_64k(w25q_dev_t *dev, uint32_t addr);
w25q_err_t w25q_chip_erase(w25q_dev_t *dev);

/* ------------------------------------------------------------------ */
/*  Suspend / Resume  (8.2.19 – 8.2.20)                               */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_suspend(w25q_dev_t *dev);
w25q_err_t w25q_resume(w25q_dev_t *dev);

/* ------------------------------------------------------------------ */
/*  Power management  (8.2.21 – 8.2.22)                               */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_power_down(w25q_dev_t *dev);
w25q_err_t w25q_release_power_down(w25q_dev_t *dev);
w25q_err_t w25q_release_power_down_id(w25q_dev_t *dev, uint8_t *device_id);

/* ------------------------------------------------------------------ */
/*  Identification  (8.2.23, 8.2.26 – 8.2.27)                         */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_read_manufacturer_device_id(w25q_dev_t *dev,
					    uint8_t *mfr_id,
					    uint8_t *device_id);
w25q_err_t w25q_read_unique_id(w25q_dev_t *dev, uint8_t uid[8]);
w25q_err_t w25q_read_jedec_id(w25q_dev_t *dev,
			      uint8_t *mfr_id,
			      uint8_t *mem_type,
			      uint8_t *capacity);

/* ------------------------------------------------------------------ */
/*  SFDP  (8.2.28)                                                     */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_read_sfdp(w25q_dev_t *dev, uint32_t addr,
			  uint8_t *buf, size_t len);

/* ------------------------------------------------------------------ */
/*  Security registers  (8.2.29 – 8.2.31)                             */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_erase_security_register(w25q_dev_t *dev, uint32_t reg_addr);
w25q_err_t w25q_program_security_register(w25q_dev_t *dev, uint32_t reg_addr,
					  const uint8_t *data, size_t len);
w25q_err_t w25q_read_security_register(w25q_dev_t *dev, uint32_t reg_addr,
				       uint8_t *buf, size_t len);

/* ------------------------------------------------------------------ */
/*  Block / Sector locks  (8.2.32 – 8.2.36)                           */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_individual_block_lock(w25q_dev_t *dev, uint32_t addr);
w25q_err_t w25q_individual_block_unlock(w25q_dev_t *dev, uint32_t addr);
w25q_err_t w25q_read_block_lock(w25q_dev_t *dev, uint32_t addr,
				uint8_t *locked);
w25q_err_t w25q_global_block_lock(w25q_dev_t *dev);
w25q_err_t w25q_global_block_unlock(w25q_dev_t *dev);

/* ------------------------------------------------------------------ */
/*  Reset  (8.2.37)                                                    */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_reset(w25q_dev_t *dev);

/* ------------------------------------------------------------------ */
/*  Utility                                                            */
/* ------------------------------------------------------------------ */
w25q_err_t w25q_wait_busy(w25q_dev_t *dev, uint32_t timeout_ms);
w25q_err_t w25q_is_busy(w25q_dev_t *dev, uint8_t *busy);

#ifdef __cplusplus
}
#endif

#endif /* W25Q128JV_H */
