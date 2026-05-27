/* SPDX-License-Identifier: MIT */
/*
 * w25q128jv.c – Portable driver for Winbond W25Q128JV SPI NOR flash.
 *
 * Implements every Standard-SPI instruction from datasheet Section 8
 * (Rev F, 03/27/2018).  All blocking waits respect the worst-case
 * timing values from Section 9.6.
 */

#include "w25q128jv.h"

/* ================================================================== */
/*  Internal helpers                                                   */
/* ================================================================== */

static int cs_lo(const w25q_dev_t *dev)
{
	return dev->hal->cs_assert(dev->hal->ctx);
}

static int cs_hi(const w25q_dev_t *dev)
{
	return dev->hal->cs_deassert(dev->hal->ctx);
}

static int spi_tx(const w25q_dev_t *dev, const uint8_t *buf, size_t len)
{
	return dev->hal->spi_xfer(dev->hal->ctx, buf, NULL, len);
}

static int spi_rx(const w25q_dev_t *dev, uint8_t *buf, size_t len)
{
	return dev->hal->spi_xfer(dev->hal->ctx, NULL, buf, len);
}

static void delay_us(const w25q_dev_t *dev, uint32_t us)
{
	dev->hal->delay_us(dev->hal->ctx, us);
}

static uint32_t tick_ms(const w25q_dev_t *dev)
{
	return dev->hal->tick_ms(dev->hal->ctx);
}

static uint32_t elapsed(uint32_t start, uint32_t now)
{
	return now - start;          /* works correctly on wrap */
}

/* Send a single-byte command inside its own /CS frame. */
static w25q_err_t cmd_only(const w25q_dev_t *dev, uint8_t opcode)
{
	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, &opcode, 1);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

/* Send opcode + 24-bit address inside its own /CS frame. */
static w25q_err_t cmd_addr(const w25q_dev_t *dev,
			   uint8_t opcode, uint32_t addr)
{
	uint8_t buf[4] = {
		opcode,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, buf, sizeof(buf));
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

/* ================================================================== */
/*  Initialisation                                                     */
/* ================================================================== */

w25q_err_t w25q_init(w25q_dev_t *dev, const w25q_hal_t *hal)
{
	if (!dev || !hal)
		return W25Q_ERR_PARAM;
	if (!hal->cs_assert || !hal->cs_deassert ||
	    !hal->spi_xfer  || !hal->delay_us    || !hal->tick_ms)
		return W25Q_ERR_PARAM;

	dev->hal = hal;
	return W25Q_OK;
}

/* ================================================================== */
/*  Write-enable / disable  (8.2.1 – 8.2.3)                           */
/* ================================================================== */

w25q_err_t w25q_write_enable(w25q_dev_t *dev)
{
	return cmd_only(dev, W25Q_CMD_WRITE_ENABLE);
}

w25q_err_t w25q_volatile_sr_write_enable(w25q_dev_t *dev)
{
	return cmd_only(dev, W25Q_CMD_VOLATILE_SR_WRITE_ENABLE);
}

w25q_err_t w25q_write_disable(w25q_dev_t *dev)
{
	return cmd_only(dev, W25Q_CMD_WRITE_DISABLE);
}

/* ================================================================== */
/*  Status registers  (8.2.4 – 8.2.5)                                 */
/* ================================================================== */

static w25q_err_t read_sr(w25q_dev_t *dev, uint8_t opcode, uint8_t *sr)
{
	if (!sr)
		return W25Q_ERR_PARAM;

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, &opcode, 1);
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, sr, 1);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

w25q_err_t w25q_read_sr1(w25q_dev_t *dev, uint8_t *sr)
{
	return read_sr(dev, W25Q_CMD_READ_SR1, sr);
}

w25q_err_t w25q_read_sr2(w25q_dev_t *dev, uint8_t *sr)
{
	return read_sr(dev, W25Q_CMD_READ_SR2, sr);
}

w25q_err_t w25q_read_sr3(w25q_dev_t *dev, uint8_t *sr)
{
	return read_sr(dev, W25Q_CMD_READ_SR3, sr);
}

static w25q_err_t write_sr(w25q_dev_t *dev, uint8_t opcode, uint8_t val)
{
	uint8_t buf[2] = { opcode, val };

	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, buf, sizeof(buf));
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;

	return w25q_wait_busy(dev, W25Q_TIMEOUT_WRITE_SR_MS);
}

w25q_err_t w25q_write_sr1(w25q_dev_t *dev, uint8_t val)
{
	return write_sr(dev, W25Q_CMD_WRITE_SR1, val);
}

w25q_err_t w25q_write_sr2(w25q_dev_t *dev, uint8_t val)
{
	return write_sr(dev, W25Q_CMD_WRITE_SR2, val);
}

w25q_err_t w25q_write_sr3(w25q_dev_t *dev, uint8_t val)
{
	return write_sr(dev, W25Q_CMD_WRITE_SR3, val);
}

/* ================================================================== */
/*  Read operations  (8.2.6 – 8.2.7)                                  */
/* ================================================================== */

w25q_err_t w25q_read(w25q_dev_t *dev, uint32_t addr,
		     uint8_t *buf, size_t len)
{
	if (!buf || (addr + len) > W25Q128JV_CHIP_SIZE)
		return W25Q_ERR_PARAM;

	uint8_t hdr[4] = {
		W25Q_CMD_READ_DATA,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, buf, len);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

w25q_err_t w25q_fast_read(w25q_dev_t *dev, uint32_t addr,
			  uint8_t *buf, size_t len)
{
	if (!buf || (addr + len) > W25Q128JV_CHIP_SIZE)
		return W25Q_ERR_PARAM;

	uint8_t hdr[5] = {
		W25Q_CMD_FAST_READ,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
		0xFF,                   /* dummy byte */
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, buf, len);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

/* ================================================================== */
/*  Page program  (8.2.13)                                             */
/* ================================================================== */

w25q_err_t w25q_page_program(w25q_dev_t *dev, uint32_t addr,
			     const uint8_t *data, size_t len)
{
	if (!data || len == 0 || len > W25Q128JV_PAGE_SIZE)
		return W25Q_ERR_PARAM;
	if (addr >= W25Q128JV_CHIP_SIZE)
		return W25Q_ERR_PARAM;

	/* Ensure the write does not cross a page boundary. */
	size_t page_remain = W25Q128JV_PAGE_SIZE - (addr & 0xFF);
	if (len > page_remain)
		return W25Q_ERR_PARAM;

	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;

	uint8_t hdr[4] = {
		W25Q_CMD_PAGE_PROGRAM,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_tx(dev, data, len);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;

	return w25q_wait_busy(dev, W25Q_TIMEOUT_PAGE_PROG_MS);
}

w25q_err_t w25q_write(w25q_dev_t *dev, uint32_t addr,
		      const uint8_t *data, size_t len)
{
	if (!data || (addr + len) > W25Q128JV_CHIP_SIZE)
		return W25Q_ERR_PARAM;

	while (len) {
		size_t page_remain = W25Q128JV_PAGE_SIZE -
				     (addr & (W25Q128JV_PAGE_SIZE - 1));
		size_t chunk = (len < page_remain) ? len : page_remain;

		w25q_err_t err = w25q_page_program(dev, addr, data, chunk);
		if (err)
			return err;

		addr += chunk;
		data += chunk;
		len  -= chunk;
	}
	return W25Q_OK;
}

/* ================================================================== */
/*  Erase operations  (8.2.15 – 8.2.18)                               */
/* ================================================================== */

static w25q_err_t erase_with_addr(w25q_dev_t *dev, uint8_t opcode,
				  uint32_t addr, uint32_t timeout_ms)
{
	if (addr >= W25Q128JV_CHIP_SIZE)
		return W25Q_ERR_PARAM;

	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;

	err = cmd_addr(dev, opcode, addr);
	if (err)
		return err;

	return w25q_wait_busy(dev, timeout_ms);
}

w25q_err_t w25q_sector_erase(w25q_dev_t *dev, uint32_t addr)
{
	return erase_with_addr(dev, W25Q_CMD_SECTOR_ERASE_4KB,
			       addr, W25Q_TIMEOUT_SECTOR_ERASE_MS);
}

w25q_err_t w25q_block_erase_32k(w25q_dev_t *dev, uint32_t addr)
{
	return erase_with_addr(dev, W25Q_CMD_BLOCK_ERASE_32KB,
			       addr, W25Q_TIMEOUT_BLOCK32_ERASE_MS);
}

w25q_err_t w25q_block_erase_64k(w25q_dev_t *dev, uint32_t addr)
{
	return erase_with_addr(dev, W25Q_CMD_BLOCK_ERASE_64KB,
			       addr, W25Q_TIMEOUT_BLOCK64_ERASE_MS);
}

w25q_err_t w25q_chip_erase(w25q_dev_t *dev)
{
	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;

	err = cmd_only(dev, W25Q_CMD_CHIP_ERASE);
	if (err)
		return err;

	return w25q_wait_busy(dev, W25Q_TIMEOUT_CHIP_ERASE_MS);
}

/* ================================================================== */
/*  Suspend / Resume  (8.2.19 – 8.2.20)                               */
/* ================================================================== */

w25q_err_t w25q_suspend(w25q_dev_t *dev)
{
	w25q_err_t err = cmd_only(dev, W25Q_CMD_ERASE_PROGRAM_SUSPEND);
	if (err)
		return err;
	delay_us(dev, W25Q_DELAY_SUSPEND_US);
	return W25Q_OK;
}

w25q_err_t w25q_resume(w25q_dev_t *dev)
{
	return cmd_only(dev, W25Q_CMD_ERASE_PROGRAM_RESUME);
}

/* ================================================================== */
/*  Power management  (8.2.21 – 8.2.22)                               */
/* ================================================================== */

w25q_err_t w25q_power_down(w25q_dev_t *dev)
{
	w25q_err_t err = cmd_only(dev, W25Q_CMD_POWER_DOWN);
	if (err)
		return err;
	delay_us(dev, W25Q_DELAY_POWER_DOWN_US);
	return W25Q_OK;
}

w25q_err_t w25q_release_power_down(w25q_dev_t *dev)
{
	w25q_err_t err = cmd_only(dev, W25Q_CMD_RELEASE_POWERDOWN_ID);
	if (err)
		return err;
	delay_us(dev, W25Q_DELAY_RELEASE_PD_US);
	return W25Q_OK;
}

w25q_err_t w25q_release_power_down_id(w25q_dev_t *dev, uint8_t *device_id)
{
	if (!device_id)
		return W25Q_ERR_PARAM;

	uint8_t hdr[4] = { W25Q_CMD_RELEASE_POWERDOWN_ID, 0xFF, 0xFF, 0xFF };

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, device_id, 1);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;

	delay_us(dev, W25Q_DELAY_RELEASE_PD_ID_US);
	return W25Q_OK;
}

/* ================================================================== */
/*  Identification  (8.2.23, 8.2.26 – 8.2.27)                         */
/* ================================================================== */

w25q_err_t w25q_read_manufacturer_device_id(w25q_dev_t *dev,
					    uint8_t *mfr_id,
					    uint8_t *device_id)
{
	if (!mfr_id || !device_id)
		return W25Q_ERR_PARAM;

	uint8_t hdr[4] = { W25Q_CMD_MANUFACTURER_DEVICE_ID, 0x00, 0x00, 0x00 };
	uint8_t id[2];

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, id, sizeof(id));
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;

	*mfr_id    = id[0];
	*device_id = id[1];
	return W25Q_OK;
}

w25q_err_t w25q_read_unique_id(w25q_dev_t *dev, uint8_t uid[8])
{
	if (!uid)
		return W25Q_ERR_PARAM;

	/* Opcode (4Bh) + 4 dummy bytes */
	uint8_t hdr[5] = { W25Q_CMD_READ_UNIQUE_ID, 0xFF, 0xFF, 0xFF, 0xFF };

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, uid, 8);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

w25q_err_t w25q_read_jedec_id(w25q_dev_t *dev,
			      uint8_t *mfr_id,
			      uint8_t *mem_type,
			      uint8_t *capacity)
{
	if (!mfr_id || !mem_type || !capacity)
		return W25Q_ERR_PARAM;

	uint8_t cmd = W25Q_CMD_JEDEC_ID;
	uint8_t id[3];

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, &cmd, 1);
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, id, sizeof(id));
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;

	*mfr_id   = id[0];
	*mem_type = id[1];
	*capacity = id[2];
	return W25Q_OK;
}

/* ================================================================== */
/*  SFDP  (8.2.28)                                                     */
/* ================================================================== */

w25q_err_t w25q_read_sfdp(w25q_dev_t *dev, uint32_t addr,
			  uint8_t *buf, size_t len)
{
	if (!buf)
		return W25Q_ERR_PARAM;

	/* Opcode + 24-bit addr + 1 dummy byte */
	uint8_t hdr[5] = {
		W25Q_CMD_READ_SFDP,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
		0xFF,
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, buf, len);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

/* ================================================================== */
/*  Security registers  (8.2.29 – 8.2.31)                             */
/* ================================================================== */

w25q_err_t w25q_erase_security_register(w25q_dev_t *dev, uint32_t reg_addr)
{
	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;

	err = cmd_addr(dev, W25Q_CMD_ERASE_SECURITY_REG, reg_addr);
	if (err)
		return err;

	return w25q_wait_busy(dev, W25Q_TIMEOUT_SECTOR_ERASE_MS);
}

w25q_err_t w25q_program_security_register(w25q_dev_t *dev, uint32_t reg_addr,
					  const uint8_t *data, size_t len)
{
	if (!data || len == 0 || len > W25Q128JV_PAGE_SIZE)
		return W25Q_ERR_PARAM;

	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;

	uint8_t hdr[4] = {
		W25Q_CMD_PROGRAM_SECURITY_REG,
		(uint8_t)(reg_addr >> 16),
		(uint8_t)(reg_addr >> 8),
		(uint8_t)(reg_addr),
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_tx(dev, data, len);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;

	return w25q_wait_busy(dev, W25Q_TIMEOUT_PAGE_PROG_MS);
}

w25q_err_t w25q_read_security_register(w25q_dev_t *dev, uint32_t reg_addr,
				       uint8_t *buf, size_t len)
{
	if (!buf)
		return W25Q_ERR_PARAM;

	/* Opcode + 24-bit addr + 1 dummy byte */
	uint8_t hdr[5] = {
		W25Q_CMD_READ_SECURITY_REG,
		(uint8_t)(reg_addr >> 16),
		(uint8_t)(reg_addr >> 8),
		(uint8_t)(reg_addr),
		0xFF,
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, buf, len);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

/* ================================================================== */
/*  Block / Sector locks  (8.2.32 – 8.2.36)                           */
/* ================================================================== */

w25q_err_t w25q_individual_block_lock(w25q_dev_t *dev, uint32_t addr)
{
	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;
	return cmd_addr(dev, W25Q_CMD_INDIVIDUAL_BLOCK_LOCK, addr);
}

w25q_err_t w25q_individual_block_unlock(w25q_dev_t *dev, uint32_t addr)
{
	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;
	return cmd_addr(dev, W25Q_CMD_INDIVIDUAL_BLOCK_UNLOCK, addr);
}

w25q_err_t w25q_read_block_lock(w25q_dev_t *dev, uint32_t addr,
				uint8_t *locked)
{
	if (!locked)
		return W25Q_ERR_PARAM;

	uint8_t hdr[4] = {
		W25Q_CMD_READ_BLOCK_LOCK,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
	};

	if (cs_lo(dev))
		return W25Q_ERR_SPI;
	int rc = spi_tx(dev, hdr, sizeof(hdr));
	if (rc) { cs_hi(dev); return W25Q_ERR_SPI; }
	rc = spi_rx(dev, locked, 1);
	if (cs_hi(dev) || rc)
		return W25Q_ERR_SPI;
	return W25Q_OK;
}

w25q_err_t w25q_global_block_lock(w25q_dev_t *dev)
{
	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;
	return cmd_only(dev, W25Q_CMD_GLOBAL_BLOCK_LOCK);
}

w25q_err_t w25q_global_block_unlock(w25q_dev_t *dev)
{
	w25q_err_t err = w25q_write_enable(dev);
	if (err)
		return err;
	return cmd_only(dev, W25Q_CMD_GLOBAL_BLOCK_UNLOCK);
}

/* ================================================================== */
/*  Reset  (8.2.37)                                                    */
/* ================================================================== */

w25q_err_t w25q_reset(w25q_dev_t *dev)
{
	w25q_err_t err = cmd_only(dev, W25Q_CMD_ENABLE_RESET);
	if (err)
		return err;

	err = cmd_only(dev, W25Q_CMD_RESET_DEVICE);
	if (err)
		return err;

	delay_us(dev, W25Q_DELAY_RESET_US);
	return W25Q_OK;
}

/* ================================================================== */
/*  Utility                                                            */
/* ================================================================== */

w25q_err_t w25q_is_busy(w25q_dev_t *dev, uint8_t *busy)
{
	if (!busy)
		return W25Q_ERR_PARAM;

	uint8_t sr;
	w25q_err_t err = w25q_read_sr1(dev, &sr);
	if (err)
		return err;

	*busy = (sr & W25Q_SR1_BUSY) ? 1 : 0;
	return W25Q_OK;
}

w25q_err_t w25q_wait_busy(w25q_dev_t *dev, uint32_t timeout_ms)
{
	uint32_t start = tick_ms(dev);

	for (;;) {
		uint8_t sr;
		w25q_err_t err = w25q_read_sr1(dev, &sr);
		if (err)
			return err;

		if (!(sr & W25Q_SR1_BUSY))
			return W25Q_OK;

		if (elapsed(start, tick_ms(dev)) >= timeout_ms)
			return W25Q_ERR_TIMEOUT;
	}
}
