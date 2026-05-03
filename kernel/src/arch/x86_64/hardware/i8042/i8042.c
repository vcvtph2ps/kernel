#include <arch/hardware/i8042/i8042.h>
#include <arch/io.h>
#include <assert.h>
#include <common/arch.h>
#include <stdint.h>
#include <string.h>

#define I8042_DATA_ACK 0xFA

#define I8042_PORT_TIMEOUT 100000

#define I8042_PORT_CMD_STATUS 0x64
#define I8042_PORT_DATA 0x60

#define I8042_COMMAND_TO_PORT2 0xD4

#define I8042_COMMAND_SELF_TEST 0xAA
#define I8042_DATA_SELF_TEST_PASSED 0x55

#define I8042_COMMAND_DISABLE_PORT1 0xAD
#define I8042_COMMAND_ENABLE_PORT1 0xAE
#define I8042_COMMAND_TEST_PORT1 0xAB

#define I8042_COMMAND_DISABLE_PORT2 0xA7
#define I8042_COMMAND_ENABLE_PORT2 0xA8
#define I8042_COMMAND_TEST_PORT2 0xA9

#define I8042_COMMAND_READ_CONFIG_BYTE 0x20
#define I8042_COMMAND_WRITE_CONFIG_BYTE 0x60

#define I8042_STATUS_RDYREAD (1 << 0)
#define I8042_STATUS_RDYWRITE (1 << 1)

#define I8042_CONFIG_BYTE_IRQ1 (1 << 0)
#define I8042_CONFIG_BYTE_IRQ2 (1 << 1)
#define I8042_CONFIG_BYTE_CLOCK1 (1 << 4)
#define I8042_CONFIG_BYTE_CLOCK2 (1 << 5)
#define I8042_CONFIG_BYTE_TRANSLATION (1 << 6)

#define I8042_KEYBOARD_DEVICE_ID 0xab83

typedef struct {
    bool available;
    bool exists;
    uint16_t device_id;
} i8042_port;

i8042_port g_i8042_ports[2] = { 0 };

static bool wait_write() {
    int timeout = I8042_PORT_TIMEOUT;
    while(--timeout)
        if(!(arch_io_port_read_u8(I8042_PORT_CMD_STATUS) & I8042_STATUS_RDYWRITE)) return true;
    return false;
}

static bool wait_read() {
    int timeout = I8042_PORT_TIMEOUT;
    while(--timeout)
        if(arch_io_port_read_u8(I8042_PORT_CMD_STATUS) & I8042_STATUS_RDYREAD) return true;
    return false;
}

static void i8042_write(uint16_t port, uint8_t value) {
    if(wait_write()) return arch_io_port_write_u8(port, value);
    arch_panic("i8042 write failed: controller failed to respond");
}

static uint8_t i8042_read(uint16_t port) {
    if(wait_read()) return arch_io_port_read_u8(port);
    arch_panic("i8042 read failed: controller failed to respond");
}

uint8_t arch_i8042_port_read(bool wait) {
    if(!wait || (wait && wait_read())) return arch_io_port_read_u8(I8042_PORT_DATA);
    arch_panic("i8042 read failed: port failed to respond");
}

bool arch_i8042_port_write(arch_i8042_port_t port, uint8_t value) {
    if(port == ARCH_I8042_PORT_TWO) {
        if(!wait_write()) return false;
        arch_io_port_write_u8(I8042_PORT_CMD_STATUS, I8042_COMMAND_TO_PORT2);
    }
    if(!wait_write()) return false;
    arch_io_port_write_u8(I8042_PORT_DATA, value);
    return true;
}

void arch_i8042_port_enable(arch_i8042_port_t port) {
    i8042_write(I8042_PORT_CMD_STATUS, port == ARCH_I8042_PORT_ONE ? I8042_COMMAND_ENABLE_PORT1 : I8042_COMMAND_ENABLE_PORT2);
}

static inline bool port_initialize(arch_i8042_port_t port) {
    uint8_t ack;
    // Reset device
    if(!arch_i8042_port_write(port, 0xFF)) return false;
    if(!wait_read()) return false;

    ack = arch_io_port_read_u8(I8042_PORT_DATA);
    if(ack != I8042_DATA_ACK) return false;

    if(!wait_read()) return false;
    uint8_t reset = arch_io_port_read_u8(I8042_PORT_DATA);
    
    // Flush any extra bytes sent during reset (e.g. mouse sends 0x00)
    while(arch_io_port_read_u8(I8042_PORT_CMD_STATUS) & I8042_STATUS_RDYREAD) {
        arch_io_port_read_u8(I8042_PORT_DATA);
    }

    if(reset != 0xAA) return false;

    // Disable Scanning command
    if(!arch_i8042_port_write(port, 0xF5)) return false;
    if(!wait_read()) return false;
    ack = arch_io_port_read_u8(I8042_PORT_DATA);
    if(ack != I8042_DATA_ACK) return false;

    // Identify
    if(!arch_i8042_port_write(port, 0xF2)) return false;
    if(!wait_read()) return false;
    ack = arch_io_port_read_u8(I8042_PORT_DATA);
    if(ack != I8042_DATA_ACK) return false;
    if(!wait_read()) return false;

    g_i8042_ports[port].device_id = ((uint16_t) arch_io_port_read_u8(I8042_PORT_DATA)) << 8;
    if(wait_read()) g_i8042_ports[port].device_id |= (uint16_t) arch_io_port_read_u8(I8042_PORT_DATA);

    // Enable Scanning command
    if(!arch_i8042_port_write(port, 0xF4)) return false;
    if(!wait_read()) return false;

    ack = arch_io_port_read_u8(I8042_PORT_DATA);
    if(ack != I8042_DATA_ACK) return false; // NOLINT
    return true;
}

void arch_ps2kbd_init(arch_i8042_port_t port);

static void port_initialize_drivers(arch_i8042_port_t port) {
    assert(g_i8042_ports[port].available && "port must be available to initialize drivers");
    if(g_i8042_ports[port].device_id == I8042_KEYBOARD_DEVICE_ID) {
        if(port == ARCH_I8042_PORT_ONE) {
            i8042_write(I8042_PORT_CMD_STATUS, 0x20);
            uint8_t configuration_byte = i8042_read(I8042_PORT_DATA);
            configuration_byte |= I8042_CONFIG_BYTE_TRANSLATION;
            i8042_write(I8042_PORT_CMD_STATUS, 0x60);
            i8042_write(I8042_PORT_DATA, configuration_byte);
        }
        arch_ps2kbd_init(port);
    }
}

bool arch_i8042_init() {
    LOG_INFO("Initializing controller\n");
    memset(g_i8042_ports, 0, sizeof(g_i8042_ports));

    // Reset the controller and devices
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_ENABLE_PORT1);
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_ENABLE_PORT2);

    arch_io_port_read_u8(I8042_PORT_DATA);

    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_READ_CONFIG_BYTE);
    uint8_t configuration_byte = i8042_read(I8042_PORT_DATA);
    configuration_byte &= ~(I8042_CONFIG_BYTE_IRQ1 | I8042_CONFIG_BYTE_IRQ2 | I8042_CONFIG_BYTE_TRANSLATION);
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_WRITE_CONFIG_BYTE);
    i8042_write(I8042_PORT_DATA, configuration_byte);

    // Self test to make sure the controller works
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_SELF_TEST);
    uint8_t self_test = i8042_read(I8042_PORT_DATA);
    if(self_test != I8042_DATA_SELF_TEST_PASSED) {
        LOG_FAIL("controller failed self test\n");
        return false;
    }

    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_WRITE_CONFIG_BYTE);
    i8042_write(I8042_PORT_DATA, configuration_byte);

    // Check for dual channel support
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_ENABLE_PORT2);
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_READ_CONFIG_BYTE);
    configuration_byte = i8042_read(I8042_PORT_DATA);
    bool dual_channel = (configuration_byte & I8042_CONFIG_BYTE_CLOCK2) == 0;
    if(dual_channel) {
        i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_DISABLE_PORT2);
        LOG_INFO("controller has dual channel support\n");
    } else {
        LOG_WARN("controller does not have dual channel support\n");
    }

    // Test ports
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_TEST_PORT1);
    g_i8042_ports[0].exists = true;
    g_i8042_ports[0].available = i8042_read(I8042_PORT_DATA) == 0;
    if(dual_channel) {
        i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_TEST_PORT2);
        g_i8042_ports[1].exists = true;
        g_i8042_ports[1].available = i8042_read(I8042_PORT_DATA) == 0;
    } else {
        g_i8042_ports[1].exists = false;
        g_i8042_ports[1].available = false;
    }

    // Enable IRQs
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_READ_CONFIG_BYTE);
    configuration_byte = i8042_read(I8042_PORT_DATA);
    if(g_i8042_ports[0].available) configuration_byte |= I8042_CONFIG_BYTE_IRQ1;
    if(g_i8042_ports[1].available) configuration_byte |= I8042_CONFIG_BYTE_IRQ2;
    i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_WRITE_CONFIG_BYTE);
    i8042_write(I8042_PORT_DATA, configuration_byte);

    // Initialize port one
    if(g_i8042_ports[0].available) {
        i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_ENABLE_PORT1);
        if(!port_initialize(ARCH_I8042_PORT_ONE)) g_i8042_ports[0].available = false;
        i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_DISABLE_PORT1);
    }

    // Initialize port two
    if(g_i8042_ports[1].available) {
        i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_ENABLE_PORT2);
        if(!port_initialize(ARCH_I8042_PORT_TWO)) {
            LOG_FAIL("failed to initialize port 2\n");
            g_i8042_ports[1].available = false;
        }
        i8042_write(I8042_PORT_CMD_STATUS, I8042_COMMAND_DISABLE_PORT2);
    }

    if(g_i8042_ports[0].available) {
        LOG_OKAY("successfully initialized port 1 with device id 0x%04x\n", g_i8042_ports[0].device_id);
    } else {
        LOG_FAIL("failed to initialize port 1\n");
    }

    if(g_i8042_ports[1].available) {
        LOG_OKAY("successfully initialized port 2 with device id 0x%04x\n", g_i8042_ports[1].device_id);
    } else {
        if(g_i8042_ports[1].exists) {
            LOG_FAIL("failed to initialize port 2\n");
        } else {
            LOG_WARN("port 2 does not exist\n");
        }
    }

    // Initialize drivers for the port(s)
    if(g_i8042_ports[0].available) port_initialize_drivers(ARCH_I8042_PORT_ONE);
    if(g_i8042_ports[1].available) port_initialize_drivers(ARCH_I8042_PORT_TWO);

    // Flush any leftover bytes in the buffer
    while(arch_io_port_read_u8(I8042_PORT_CMD_STATUS) & I8042_STATUS_RDYREAD) {
        arch_io_port_read_u8(I8042_PORT_DATA);
    }

    return true;
}
