#ifndef __I2CSLAVE_RK_H
#define __I2CSLAVE_RK_H

/** Available sizes for I2C slave registers (1, 2, or 4 bytes) */
typedef enum {
	REG_SIZE_8 = sizeof (uint8_t),
	REG_SIZE_16 = sizeof (uint16_t),
	REG_SIZE_32 = sizeof (uint32_t)
} I2C_Reg_Size_t;

/** Available address sizes for I2C slave registers (1 or 2 bytes) */
typedef enum {
	REG_ADDR_SIZE_8 = sizeof (uint8_t),
	REG_ADDR_SIZE_16 = sizeof (uint16_t)
} I2C_Reg_Addr_Size_t;

/**
 * Class for making a Photon/Electron be an I2C slave device
 *
 * This is opposite of the usual case; normally the Photon is the master to slave devices
 * like sensors and displays.
 *
 * Note: You can only instantiate one of these because of the way the event handlers
 * are registered.
 */
class I2CSlave {
public:
	/**
	 * Initialize the object. Normally you have one of these as a global variable.
	 *
	 * wire is the Wire interface to use, typically Wire (D0/D1). On the Electron it can be Wire1 (C4/C5)
	 *
	 * addr is the 7-bit I2C address of this slave device.
	 *
	 * numRegisters is the number of 32-, 16-, or 8-bit registers to allocate space for (based on regSize). 
	 * If regAddrSize is 8, then this should be <= 256 because anything above that will be inaccessible.
	 * 
	 * regSize is the size of each register (1, 2, or 4 bytes, default 4)
	 * 
	 * regAddrSize is the size of the register addresses (1 or 2 bytes, default 2)
	 */
	I2CSlave(TwoWire &wire, uint8_t addr, size_t numRegisters, I2C_Reg_Size_t regSize=REG_SIZE_32, I2C_Reg_Addr_Size_t regAddrSize=REG_ADDR_SIZE_16);
	virtual ~I2CSlave();

	/**
	 * The begin method must be called from setup()
	 */
	void begin();

	/**
	 * Gets the value of a register
	 *
	 * regAddr must be 0 <= regAddr < numRegisters. If regAddrSize is 8 bits, upper byte is ignored.
	 *
	 * The value is typically set by the I2C master, though you can also read values you've set
	 * locally on this machine. See also getRegisterSet.
	 * 
	 * If regSize is less than 32 bits, then upper bits will be zeroes.
	 */
	uint32_t getRegister(uint16_t regAddr) const;

	/**
	 * Sets the value of a register
	 *
	 * regAddr must be 0 <= regAddr < numRegisters. If regAddrSize is 8 bits, upper byte is ignored.
	 *
	 * value is a 32-bit value, not interpreted in any way. If regSize is less than 32 bits,
	 * the upper unused bits are ignored.
	 *
	 * You normally set a register so it can be retrieved from the I2C master. The model of this class
	 * is that you store the value in a register whenever it is changed, as this is a low-cost operation.
	 *
	 * This class doesn't support a model where the value is instantaenously generated when the I2C
	 * master requests it as you don't have much time to to this and it requires annoying ISR programming.
	 */
	void setRegister(uint16_t regAddr, uint32_t value);

	/**
	 * Whenever the I2C master sets a register, a flag is set (for the first 32 registers).
	 *
	 * If you are interested in knowing when this happens, call getRegisterSet() from your loop. If it
	 * returns true, then regAddr will be filled in with the register that was set. You can then use
	 * getRegister() to get the value, if you care about that.
	 *
	 * You can also use this merely as a signal. For example, if the master writes to register 4, you
	 * might interpret this as a trigger to do something.
	 */
	bool getRegisterSet(uint16_t &regAddr);

	/**
	 * Return a bitmask that can be used to truncate values or addresses based on register/address size
	 */
	static uint32_t regSizeMask(I2C_Reg_Size_t regSize);
	static uint16_t regAddrSizeMask(I2C_Reg_Addr_Size_t regAddrSize);

protected:
	void receiveEvent(int numBytes);
	static void receiveEventStatic(int numBytes);

	void requestEvent();
	static void requestEventStatic();

protected:
	TwoWire &wire;
	uint8_t addr; // 7-bit address for this device
	const I2C_Reg_Size_t regSize;
	const I2C_Reg_Addr_Size_t regAddrSize;
	size_t numRegisters;
	uint8_t *registers;
	uint16_t nextReadAddr;
	volatile uint32_t registerSetFlags;
};


#endif /* __I2CSLAVE_RK_H */
