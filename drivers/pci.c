#include "pci.h"

/*
    Bit 31: Enable bit
    Bit 30-24: Reserved
    Bit 23-16: Bus number
    Bit 15-11: Device number (slot)
    Bit 10-8: Function number
    Bit 7-0: Register offset

    Where bits 1-0 of the register offset are always 0
*/

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;
    uint16_t tmp = 0;

    // Create configuration address as per above
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    uint16_t port = CONFIG_ADDRESS;
    port_dword_out(port, address);

    // Read in the data
    // ((offset & 2) * 8) = 0 will choose the first word of the 32 bits register
    port = CONFIG_DATA;
    tmp = (uint16_t)((port_dword_in(port) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

void pciConfigWriteWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data) {
    uint32_t address;
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;

    // Create configuration address as per above
    address = (uint32_t)((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    // Write out the address
    uint16_t port = CONFIG_ADDRESS;
    port_dword_out(port, address);

    // Write out the data
    port = CONFIG_DATA;
    port_dword_out(port, data);

}

/*
    When a configuration access attempt to select a device that does not exists on the bus,
    the device will return all 1's on the data(read) lines. This is called a "no-device present" response.
*/

uint16_t pciCheckVendor(uint8_t bus, uint8_t slot) {
    uint16_t vendor, device;

    // Try and read the first configuration register. Since there are no vendors that == 0xFFFF,
    // it must be a non-existent device.
    if ((vendor = pciConfigReadWord(bus, slot, 0, 0)) != 0xFFFF) {
        device = pciConfigReadWord(bus, slot, 0, 2);
        return device;
    }
    return (uint16_t) 0;
}

/*
    Reg 0x0: Offset 0x0: Device ID bits 31-16, Vendor ID bits 15-0
    Reg 0x1: Offset 0x4: Status bits 31-16, Command bits 15-0
    Reg 0x2: Offset 0x8: Class code bits 31-24, Subclass code bits 23-16,
        Prog IF bits 15-8, Revision ID bits 7-0
    Reg 0x3: Offset 0xC: BIST bits 31-24, Header type bits 23-16,
        Latency timer bits 15-8, Cache line size bits 7-0
*/

struct pci_device get_pci_device(uint8_t bus, uint8_t slot, uint8_t func) {
    struct pci_device device;
    device.deviceID = pciConfigReadWord(bus, slot, func, 0x0);
    device.vendorID = pciConfigReadWord(bus, slot, func, 0x2);
    device.status = pciConfigReadWord(bus, slot, func, 0x4);
    device.command = pciConfigReadWord(bus, slot, func, 0x6);
    device.class = pciConfigReadWord(bus, slot, func, 0x8);
    device.subclass = pciConfigReadWord(bus, slot, func, 0xA);
    device.progIF = pciConfigReadWord(bus, slot, func, 0xC);
    device.revisionID = pciConfigReadWord(bus, slot, func, 0xE);
    device.bist = pciConfigReadWord(bus, slot, func, 0x10);
    device.headerType = pciConfigReadWord(bus, slot, func, 0x12);
    device.latencyTimer = pciConfigReadWord(bus, slot, func, 0x14);
    device.cacheLineSize = pciConfigReadWord(bus, slot, func, 0x16);
    return device;
}

void get_all_pci_devices() {
    uint8_t bus, slot, func;
    for (bus = 0; bus < MAX_SLOT; bus++) {
        for (slot = 0; slot < MAX_SLOT; slot++) {
            for (func = 0; func < MAX_FUNC; func++) {
                uint16_t vendorID = pciConfigReadWord(bus, slot, func, 0);
                if (vendorID != 0xFFFF) {
                    struct pci_device device = get_pci_device(bus, slot, func);
                    kprint("Device: ");
                    kprint_hex(device.deviceID);
                    kprint("Vendor: ");
                    kprint_hex(device.vendorID);
                    kprint("Class: ");
                    kprint_hex(device.class);
                }
            }
        }
    }
}

void set_pci_command_register(uint8_t bus, uint8_t slot, uint8_t func, uint16_t command) {
    uint16_t data = get_pci_command_register(bus, slot, func);
    data = (data & 0xFF00) | command;
    pciConfigWriteWord(bus, slot, func, 0x4, data);
}

uint16_t get_pci_command_register(uint8_t bus, uint8_t slot, uint8_t func) {
    return (uint16_t) pciConfigReadWord(bus, slot, func, 0x4);
}

uint16_t command_register_to_int(struct command_register command) {
    uint16_t data = 0;
    data |= command.ioSpaceEnable << 0;
    data |= command.memSpaceEnable << 1;
    data |= command.busMasterEnable << 2;
    data |= command.specialCycles << 3;
    data |= command.memWriteAndInvalidateEnable << 4;
    data |= command.vgaPaletteSnoop << 5;
    data |= command.parityErrorResponse << 6;
    data |= command.idselSteppingControl << 7;
    data |= command.serrEnable << 8;
    data |= command.fastBackToBackEnable << 9;
    data |= command.interruptDisable << 10;
    return data;
}

/*
    Bit 15: Detecte Parity Error
    Bit 14: Signaled System Error
    Bit 13: Received Master Abort
    Bit 12: Received Target Abort
    Bit 11: Signaled Target Abort
    Bit 10-9: DEVSEL Timing
    Bit 8: Master Data Parity Error
    Bit 7: Fast Back-to-Back Capable
    Bit 6: Reserved
    Bit 5: 66 MHz Capable
    Bit 4: Capabilities List
    Bit 3: Interrupt Status
    Bit 2-0: Reserved
*/

uint16_t get_pci_status_register(uint8_t bus, uint8_t slot, uint8_t func) {
    return pciConfigReadWord(bus, slot, func, 0x6);
}