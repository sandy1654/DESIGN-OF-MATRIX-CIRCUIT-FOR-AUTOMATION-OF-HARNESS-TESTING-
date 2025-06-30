#include <p18f4580.h>

// Define constants
#define FREQ    8000000      // Internal Oscillator Frequency (8 MHz)
#define BAUD    4800         // Baud rate
#define MY_UBRR ((FREQ/16/BAUD) - 1)  // Calculate UBRR value for high-speed mode

// MCP23S17 Register Addresses
#define IODIRA  0x00   // I/O direction register for port A
#define IODIRB  0x01   // I/O direction register for port B
#define GPIOA   0x12   // GPIO register for port A
#define GPIOB   0x13   // GPIO register for port B

// Function prototypes
void UART_Init(void);
void UART_Transmit(unsigned char data);
unsigned char UART_Receive(void);
void handle_UART_Errors(void);
void SPI_Init(void);
void SPI_ByteWrite(unsigned char chipSelect, unsigned char opcode, unsigned char addr, unsigned char data);
void MCP_Init(void);
void Set_GPIO(unsigned char chipSelect, unsigned char port, unsigned char value);

// UART Initialization
void UART_Init(void) {
    unsigned int ubrr = MY_UBRR;

    // Set internal oscillator to 8 MHz
    OSCCONbits.IRCF = 0b111;  // Set internal oscillator to 8 MHz

    // Set baud rate
    SPBRG = ubrr;        // Set SPBRG for baud rate (4800)

    // Configure UART
    TXSTAbits.BRGH = 1;  // High-speed baud rate generation
    TXSTAbits.SYNC = 0;  // Asynchronous mode
    RCSTAbits.SPEN = 1;  // Enable serial port
    TXSTAbits.TXEN = 1;  // Enable transmitter
    RCSTAbits.CREN = 1;  // Enable receiver

    // Configure TX/RX pins
    TRISCbits.TRISC6 = 0;  // TX pin as output
    TRISCbits.TRISC7 = 1;  // RX pin as input
}

// SPI Initialization (Master mode)
void SPI_Init(void) {
    TRISCbits.TRISC3 = 0;  // SCK pin as output
    TRISCbits.TRISC5 = 0;  // SDO pin as output
    TRISCbits.TRISC4 = 1;  // SDI pin as input

    // Configure SPI (Master mode, Fosc/16, SCK idle low, data sampled at rising edge)
    SSPSTATbits.CKE = 1;    // Data transmitted on rising edge
    SSPCON1bits.CKP = 0;    // Clock idle state is low
    SSPCON1bits.SSPM = 0b0001;  // SPI Master mode, clock = Fosc/16
    SSPCON1bits.SSPEN = 1;  // Enable SPI

    // Configure RD0 to RD7 as SS pins
    TRISDbits.TRISD0 = 0;   // Set RD0 as output (SS pin for MCP1)
    TRISDbits.TRISD1 = 0;   // Set RD1 as output (SS pin for MCP2)
    TRISDbits.TRISD2 = 0;   // Set RD2 as output (SS pin for MCP3)
    TRISDbits.TRISD3 = 0;   // Set RD3 as output (SS pin for MCP4)
    TRISDbits.TRISD4 = 0;   // Set RD4 as output (SS pin for MCP5)
    TRISDbits.TRISD5 = 0;   // Set RD5 as output (SS pin for MCP6)
    TRISDbits.TRISD6 = 0;   // Set RD6 as output (SS pin for MCP7)
    TRISDbits.TRISD7 = 0;   // Set RD7 as output (SS pin for MCP8)
    LATDbits.LATD0 = 1;     // Set SS for MCP1 high (inactive)
    LATDbits.LATD1 = 1;     // Set SS for MCP2 high (inactive)
    LATDbits.LATD2 = 1;     // Set SS for MCP3 high (inactive)
    LATDbits.LATD3 = 1;     // Set SS for MCP4 high (inactive)
    LATDbits.LATD4 = 1;     // Set SS for MCP5 high (inactive)
    LATDbits.LATD5 = 1;     // Set SS for MCP6 high (inactive)
    LATDbits.LATD6 = 1;     // Set SS for MCP7 high (inactive)
    LATDbits.LATD7 = 1;     // Set SS for MCP8 high (inactive)
}

// SPI Byte Write with new sequence format
void SPI_ByteWrite(unsigned char chipSelect, unsigned char opcode, unsigned char addr, unsigned char data) {
    // Deactivate all MCP23S17 chips
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;
    LATDbits.LATD4 = 1;
    LATDbits.LATD5 = 1;
    LATDbits.LATD6 = 1;
    LATDbits.LATD7 = 1;

    // Activate selected MCP23S17
    switch (chipSelect) {
        case 0: LATDbits.LATD0 = 0; break;
        case 1: LATDbits.LATD1 = 0; break;
        case 2: LATDbits.LATD2 = 0; break;
        case 3: LATDbits.LATD3 = 0; break;
        case 4: LATDbits.LATD4 = 0; break;
        case 5: LATDbits.LATD5 = 0; break;
        case 6: LATDbits.LATD6 = 0; break;
        case 7: LATDbits.LATD7 = 0; break;
    }

    // Send opcode
    SSPBUF = opcode;        // Load opcode into SSPBUF to start transmission
    while (!SSPSTATbits.BF); // Wait until transmission is complete

    // Send address
    SSPBUF = addr;         // Load address into SSPBUF
    while (!SSPSTATbits.BF); // Wait until transmission is complete

    // Send data
    SSPBUF = data;         // Load data into SSPBUF
    while (!SSPSTATbits.BF); // Wait until transmission is complete

    // Deactivate all MCP23S17 chips
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;
    LATDbits.LATD4 = 1;
    LATDbits.LATD5 = 1;
    LATDbits.LATD6 = 1;
    LATDbits.LATD7 = 1;
}

// MCP23S17 Initialization
void MCP_Init(void) {
    unsigned char opcode = 0x40;  // Opcode for MCP23S17 write operation (address = 0x20)
    
    // Initialize MCP1 to MCP8
    SPI_ByteWrite(0, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP1
    SPI_ByteWrite(0, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP1
    
    SPI_ByteWrite(1, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP2
    SPI_ByteWrite(1, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP2
    
    SPI_ByteWrite(2, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP3
    SPI_ByteWrite(2, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP3
    
    SPI_ByteWrite(3, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP4
    SPI_ByteWrite(3, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP4
    
    SPI_ByteWrite(4, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP5
    SPI_ByteWrite(4, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP5
    
    SPI_ByteWrite(5, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP6
    SPI_ByteWrite(5, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP6
    
    SPI_ByteWrite(6, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP7
    SPI_ByteWrite(6, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP7
    
    SPI_ByteWrite(7, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs) for MCP8
    SPI_ByteWrite(7, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs) for MCP8
}

// Transmit data via UART
void UART_Transmit(unsigned char data) {
    while (!PIR1bits.TXIF);  // Wait until the transmit buffer is empty
    TXREG = data;            // Transmit the data
}

// Receive data via UART
unsigned char UART_Receive(void) {
    while (!PIR1bits.RCIF);  // Wait until data is received
    handle_UART_Errors();    // Check for overrun or framing errors
    return RCREG;            // Return the received data
}

// Check and handle UART errors
void handle_UART_Errors(void) {
    if (RCSTAbits.OERR) {  // Overrun error
        RCSTAbits.CREN = 0;  // Clear the overrun error flag
        RCSTAbits.CREN = 1;  // Re-enable receiver
    }
    
    if (RCSTAbits.FERR) {  // Framing error
        unsigned char dummy = RCREG;  // Clear the framing error by reading RCREG
    }
}

// Main function
void main(void) {
    unsigned char data[18];  // Array to hold 18 bytes of data from UART
    unsigned char i;

    UART_Init();    // Initialize UART
    SPI_Init();     // Initialize SPI
    MCP_Init();     // Initialize MCP23S17 and set GPIOA and GPIOB pins low by default

    while (1) {
        // Receive start signal (0x7E)
        while (UART_Receive() != 0x7E);

        // Receive 16 bytes of data from UART
        for (i = 0; i < 16; i++) {
            data[i] = UART_Receive();
        }

        // Receive end signal (0x7F)
        while (UART_Receive() != 0x7F);

        // Process data for MCP1 to MCP8
        for (i = 0; i < 8; i++) {
            SPI_ByteWrite(i, 0x40, GPIOA, data[i * 2]);  // Update GPIOA for MCP(i+1)
            SPI_ByteWrite(i, 0x40, GPIOB, data[i * 2 + 1]);  // Update GPIOB for MCP(i+1)
        }
    }
}

