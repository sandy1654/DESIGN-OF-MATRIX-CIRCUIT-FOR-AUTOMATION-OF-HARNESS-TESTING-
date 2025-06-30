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

    // Configure RD0-RD7 as SS pins for MCP1-MCP8
    TRISDbits.TRISD0 = 0;   // Set RD0 as output (SS pin for MCP1)
    TRISDbits.TRISD1 = 0;   // Set RD1 as output (SS pin for MCP2)
    TRISDbits.TRISD2 = 0;   // Set RD2 as output (SS pin for MCP3)
    TRISDbits.TRISD3 = 0;   // Set RD3 as output (SS pin for MCP4)
    TRISDbits.TRISD4 = 0;   // Set RD4 as output (SS pin for MCP5)
    TRISDbits.TRISD5 = 0;   // Set RD5 as output (SS pin for MCP6)
    TRISDbits.TRISD6 = 0;   // Set RD6 as output (SS pin for MCP7)
    TRISDbits.TRISD7 = 0;   // Set RD7 as output (SS pin for MCP8)

    // Deactivate all MCPs by setting SS high
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;
    LATDbits.LATD4 = 1;
    LATDbits.LATD5 = 1;
    LATDbits.LATD6 = 1;
    LATDbits.LATD7 = 1;
}

// SPI Byte Write with support for 8 MCP chips
void SPI_ByteWrite(unsigned char chipSelect, unsigned char opcode, unsigned char addr, unsigned char data) {
    // Deactivate all MCPs by setting SS high
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;
    LATDbits.LATD4 = 1;
    LATDbits.LATD5 = 1;
    LATDbits.LATD6 = 1;
    LATDbits.LATD7 = 1;

    // Select the appropriate MCP chip
    switch (chipSelect) {
        case 0: LATDbits.LATD0 = 0; break;  // Activate MCP1
        case 1: LATDbits.LATD1 = 0; break;  // Activate MCP2
        case 2: LATDbits.LATD2 = 0; break;  // Activate MCP3
        case 3: LATDbits.LATD3 = 0; break;  // Activate MCP4
        case 4: LATDbits.LATD4 = 0; break;  // Activate MCP5
        case 5: LATDbits.LATD5 = 0; break;  // Activate MCP6
        case 6: LATDbits.LATD6 = 0; break;  // Activate MCP7
        case 7: LATDbits.LATD7 = 0; break;  // Activate MCP8
    }

    // Send opcode
    SSPBUF = opcode;        
    while (!SSPSTATbits.BF); 

    // Send address
    SSPBUF = addr;         
    while (!SSPSTATbits.BF);

    // Send data
    SSPBUF = data;         
    while (!SSPSTATbits.BF);

    // Deactivate all MCPs by setting SS high
    LATDbits.LATD0 = 1;
    LATDbits.LATD1 = 1;
    LATDbits.LATD2 = 1;
    LATDbits.LATD3 = 1;
    LATDbits.LATD4 = 1;
    LATDbits.LATD5 = 1;
    LATDbits.LATD6 = 1;
    LATDbits.LATD7 = 1;
}

// MCP23S17 Initialization for all 8 MCPs
void MCP_Init(void) {
    unsigned char opcode = 0x40;  // Opcode for MCP23S17 write operation (address = 0x20)

    // MCP1 Initialization
    SPI_ByteWrite(0, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP1)
    SPI_ByteWrite(0, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP1)

    // MCP2 Initialization
    SPI_ByteWrite(1, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP2)
    SPI_ByteWrite(1, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP2)

    // MCP3 Initialization
    SPI_ByteWrite(2, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP3)
    SPI_ByteWrite(2, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP3)

    // MCP4 Initialization
    SPI_ByteWrite(3, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP4)
    SPI_ByteWrite(3, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP4)

    // MCP5 Initialization
    SPI_ByteWrite(4, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP5)
    SPI_ByteWrite(4, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP5)

    // MCP6 Initialization
    SPI_ByteWrite(5, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP6)
    SPI_ByteWrite(5, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP6)

    // MCP7 Initialization
    SPI_ByteWrite(6, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP7)
    SPI_ByteWrite(6, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP7)

    // MCP8 Initialization
    SPI_ByteWrite(7, opcode, IODIRA, 0x00);  // Set IODIRA to 0x00 (all pins as outputs for MCP8)
    SPI_ByteWrite(7, opcode, IODIRB, 0x00);  // Set IODIRB to 0x00 (all pins as outputs for MCP8)
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
    unsigned char data[16];    // Array to hold 16 bytes of data from UART (2 per MCP)
    unsigned char i;

    UART_Init();    // Initialize UART
    SPI_Init();     // Initialize SPI
    MCP_Init();     // Initialize MCP23S17 and set GPIOA and GPIOB pins low by default

    while (1) {
        // Receive 16 bytes of data from UART (2 bytes for each of the 8 MCPs)
        for (i = 0; i < 16; i++) {
            data[i] = UART_Receive();
        }

        // Process data for each MCP (update GPIOA and GPIOB for all 8 MCPs)
        for (i = 0; i < 8; i++) {
            SPI_ByteWrite(i, 0x40, GPIOA, data[2 * i]);     // Update GPIOA for MCP i
            SPI_ByteWrite(i, 0x40, GPIOB, data[2 * i + 1]); // Update GPIOB for MCP i
        }
    }
}

