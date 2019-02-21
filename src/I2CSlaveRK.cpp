
#include "Particle.h"
#include "I2CSlaveRK.h"

static I2CSlave *globalObj = 0;

I2CSlave::I2CSlave(TwoWire &wire, uint8_t addr, size_t numRegisters, I2C_Reg_Size_t regSize, I2C_Reg_Addr_Size_t regAddrSize) :
		wire(wire), addr(addr), regSize(regSize), regAddrSize(regAddrSize), 
		numRegisters(numRegisters), nextReadAddr(0), registerSetFlags(0) {
	registers = (uint8_t *)malloc(numRegisters * regSize);
	globalObj = this;
}

I2CSlave::~I2CSlave() {
	free(registers);
	globalObj = 0;
}

void I2CSlave::begin() {

	wire.begin(addr);
	wire.onReceive(receiveEventStatic);
	wire.onRequest(requestEventStatic);
}

uint32_t I2CSlave::getRegister(uint16_t regAddr) const {
	if (regAddr >= numRegisters) {
		return 0;
	}
	uint32_t value = registers[regAddr * regAddrSize];
	if (regSize > REG_SIZE_8) {
		value |= (registers[regAddr * regAddrSize + 1] << 8);
	}
	if (regSize == REG_SIZE_32) {
		value |= (registers[regAddr * regAddrSize + 2] << 16);
		value |= (registers[regAddr * regAddrSize + 3] << 24);
	}
	return value;
}

void I2CSlave::setRegister(uint16_t regAddr, uint32_t value) {
	if (regAddr >= numRegisters) {
		return;
	}
	registers[regAddr * regAddrSize] = value & 0xff;
	if (regSize > REG_SIZE_8) {
		registers[regAddr * regAddrSize + 1] = (value >> 8) & 0xff;
	}
	if (regSize == REG_SIZE_32) {
		registers[regAddr * regAddrSize + 2] = (value >> 16) & 0xff;
		registers[regAddr * regAddrSize + 3] = (value >> 24) & 0xff;
	}
}

bool I2CSlave::getRegisterSet(uint16_t &regAddr) {
	if (registerSetFlags) {
		for(uint16_t ii = 0; ii < 32; ii++) {
			if (registerSetFlags & (1 << ii)) {
				// Bit is set, return the address and clear the flag
				regAddr = ii;
				registerSetFlags &= ~(1 << ii);
				return true;
			}
		}
		return false;
	}
	else {
		return false;
	}
}

// [static]
uint32_t regSizeMask(I2C_Reg_Size_t regSize) {
	switch (regSize) {
		case REG_SIZE_8:  return 0xff;
		case REG_SIZE_16: return 0xffff;
		default:          return 0xffffffff;
	}
}

// [static]
uint16_t regAddrSizeMask(I2C_Reg_Addr_Size_t regAddrSize) {
	switch (regAddrSize) {
		case REG_ADDR_SIZE_8: return 0xff;
		default:              return 0xffff;
	}
}


void I2CSlave::receiveEvent(int numBytes) {
	if (numBytes == regAddrSize) {
		// This is just an  address to read the register. Save for requestEvent.
		nextReadAddr = (wire.read() & 0xff);
		if (regAddrSize == REG_ADDR_SIZE_16) {
			nextReadAddr |= (wire.read() & 0xff) << 8;
		}
	}
	else
	if (numBytes == (regAddrSize + regSize)) {
		// Set a register
		uint16_t writeAddr = (wire.read() & 0xff);
		if (regAddrSize == REG_ADDR_SIZE_16) {
			writeAddr |= (wire.read() & 0xff) << 8;
		}

		// If address is out of range, read and discard
		if (writeAddr >= numRegisters) {
			for(int i=0; i<regSize; i++){
				wire.read();
			}
			return;
		}

		// Read variable number of bytes
		registers[writeAddr * regAddrSize] = (wire.read() & 0xff);
		if (regSize > REG_SIZE_8) {
			registers[writeAddr * regAddrSize + 1] = (wire.read() & 0xff);
		}
		if (regSize == REG_SIZE_32) {
			registers[writeAddr * regAddrSize + 2] = (wire.read() & 0xff);
			registers[writeAddr * regAddrSize + 3] = (wire.read() & 0xff);
		}

		// Set register flag
		if (writeAddr < 32) {
			registerSetFlags |= 1 << writeAddr;
		}
	}
}

void I2CSlave::requestEvent() {
	// Request to read
	if (nextReadAddr >= numRegisters) {
		// Address out of range, return zeroes.
		const uint8_t zeroes[regSize] = {};
		wire.write(zeroes, regSize);
	}
	else {
		wire.write(registers + (nextReadAddr * regSize), regSize);
	}
}

// [static]
void I2CSlave::receiveEventStatic(int numBytes) {
	if (globalObj) {
		globalObj->receiveEvent(numBytes);
	}
}

// [static]
void I2CSlave::requestEventStatic() {
	if (globalObj) {
		globalObj->requestEvent();
	}
}
