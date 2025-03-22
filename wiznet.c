#define FOSC 11059200UL
#include <string.h>
#include <8051.h>

#define MOSI P1_5
#define MISO P1_6
#define SCLK P1_7
#define CS P1_4
#define CMD_BUFFER_SIZE 21
#define RX_LED P1_1 
#define TX_LED P1_0
#define IMR_ADDR 0x0016 
#define SIMR_ADDR 0x0018
#define Sn_IR_ADDR 0x0002   

// extern void SPI_init(void);
// extern void SPI_enable(void);
// extern void SPI_disable(void);
// extern void SPI_send_bit(unsigned char bit);
// extern void SPI_transfer(unsigned char data);
// extern unsigned char SPI_receive(void);

char __xdata cmd_buffer[CMD_BUFFER_SIZE];  // buffer to hold the command
unsigned char __xdata cmd_index = 0; 
unsigned char __xdata size_m; // size of the message in UDP
unsigned char __xdata ip_addr[4] = {192, 168, 16, 222};  // Initial IP of wiznet
unsigned char __xdata mac_addr[6] = {0x00, 0x08, 0xDC, 0x01, 0x02, 0x05}; // Initial MAC of wiznet
unsigned char __xdata subnet_mask[4] = {255, 255, 255, 0}; // Initial Subnet of wiznet
unsigned char __xdata gateway_addr[4] = {192, 168, 16, 1}; // Initial Gateway of wiznet
unsigned char __xdata RTU_address = '3'; // Initial RTU for wiznet
unsigned char __xdata received_char;
unsigned char __xdata status;
__bit uart_data_received = 0;
__bit w5500_interrupt_triggered = 0;
__bit wake_up = 0;


void LED_init() {
    TX_LED = 0;  
    RX_LED = 0;  
}

void UART_init(unsigned long baudrate){
    unsigned long __xdata BRT = 256 - FOSC / (12 * 15 * baudrate);
    SCON = 0x50; 
    SM0 = 0;
    SM1 = 1; 
	TMOD = 0x20; 
	TL1 = BRT; 
	TH1 = BRT;
	TR1 = 1; // start timer 1
    ES = 1; // enable UART serial interrupt
    EA = 1; // enable global interrupts
}

// UART print a character
void UART_transmit(unsigned char dat) {
    SBUF = dat;
    while (!TI){} 
    TI = 0; 
}

// UART print a string
void UART_print(unsigned char* string)
{
    while(*string)
    {
        UART_transmit(*string); 
        string++; 
    }
}

// clear the UART screen
void clear_screen() {
    UART_print("\033[2J\033[H");
}

// UART input interrupt
void UART_ISR() __interrupt 4 {
    
    if (RI) {
        RI = 0;
        received_char = SBUF;
        uart_data_received = 1;
        if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = received_char; 
        }
    }
}

// Display the UART menu
void display_config() {
    unsigned int __xdata i;
    UART_print("CURRENT CONFIG:                        CHANGE CMD:\r\n");

    UART_print("RTU Address(0-9): ");
    UART_transmit(RTU_address);
    UART_print("                    RTU=#\r\n");

    UART_print("IP Address: ");
    for (i = 0; i < 4; i++) {
        if (ip_addr[i] >= 100) {  
            UART_transmit(ip_addr[i] / 100 + '0');          
            UART_transmit((ip_addr[i] / 10) % 10 + '0');    
        } else if (ip_addr[i] >= 10) {  
            UART_transmit((ip_addr[i] / 10) % 10 + '0');    
        }
        UART_transmit(ip_addr[i] % 10 + '0');              

        if (i < 3) UART_transmit('.');  
    }
    UART_print("             IP=#.#.#.#\r\n");
    
    UART_print("Subnet Mask: ");
    for (i = 0; i < 4; i++) {
        if (subnet_mask[i] >= 100) {
            UART_transmit(subnet_mask[i] / 100 + '0');
            UART_transmit((subnet_mask[i] / 10) % 10 + '0');
        } else if (subnet_mask[i] >= 10) {
            UART_transmit((subnet_mask[i] / 10) % 10 + '0');
        }
        UART_transmit(subnet_mask[i] % 10 + '0');

        if (i < 3) UART_transmit('.');
    }
    UART_print("             SUB=#.#.#.#\r\n");

    UART_print("Gateway: ");
    for (i = 0; i < 4; i++) {
        if (gateway_addr[i] >= 100) {
            UART_transmit(gateway_addr[i] / 100 + '0');
            UART_transmit((gateway_addr[i] / 10) % 10 + '0');
        } else if (gateway_addr[i] >= 10) {
            UART_transmit((gateway_addr[i] / 10) % 10 + '0');
        }
        UART_transmit(gateway_addr[i] % 10 + '0');

        if (i < 3) UART_transmit('.');
    }
    UART_print("                  GATE=#.#.#.#\r\n");

    UART_print("MAC Address: ");
    for (i = 0; i < 6; i++) {
        UART_transmit((mac_addr[i] >> 4) + ((mac_addr[i] >> 4) > 9 ? 'A' - 10 : '0'));
        UART_transmit((mac_addr[i] & 0x0F) + ((mac_addr[i] & 0x0F) > 9 ? 'A' - 10 : '0'));
        if (i < 5) UART_transmit(':');
    }
    UART_print("         MAC=FF:FF:FF:FF:FF:FF\r\n");
}

// function used to change the IP, Subnet, and gateway address based on the UART input
void parse_ip_address(char* str, unsigned char* ip_array) {
    unsigned char __xdata i = 0;      
    unsigned int __xdata num = 0;     

    while (*str) {
        if (*str == '.') {
            if (i < 4) {                       
                ip_array[i++] = (unsigned char)num;  
                num = 0;                             
            }
        } else if (*str >= '0' && *str <= '9') {
            num = num * 10 + (*str - '0');     
        }
        str++;
    }

    if (i < 4) {
        ip_array[i] = (unsigned char)num;       
    }
}

void SPI_init()
{
    MOSI = 1;
    SCLK = 1;
    CS = 1;
}

void SPI_enable()
{
    CS = 0;
    SCLK = 0;
}

void SPI_disable()
{
    CS = 1;
    SCLK = 1;
}

// updates MOSI bit by bit to be sent
void SPI_send_bit(unsigned char bit) {
    unsigned int __xdata i = 0;
    MOSI = bit;  
    SCLK = 1;    
    //for(i = 0; i < 1000; i++);
    SCLK = 0;    
}

// checks for the data to be sent bit by bit 
void SPI_transfer(unsigned char data) {
    unsigned char __xdata i;
    for (i = 0; i < 8; i++) {
        SPI_send_bit((data & 0x80) ? 1 : 0);  
        data <<= 1;  
    }
}


unsigned char SPI_receive(void) {
    unsigned char __xdata i = 0;
    unsigned int __xdata j = 0;
    unsigned char __xdata received_data = 0;
    
    for (i = 0; i < 8; i++) {
        received_data <<= 1;
         
        if (MISO) {
            received_data |= 0x01;  // received data 1 if MISO = 1
        }
        SCLK = 1;    
        //for(j = 0; j < 1000; j++);
        SCLK = 0;  
    }
    return received_data;
}

// function used to write to 1 register
void writeReg(unsigned short address, unsigned char data, unsigned char control) 
{    
    SPI_enable();
    SPI_transfer(address >> 8);  
    SPI_transfer(address & 0xFF);        
    SPI_transfer(control);
    SPI_transfer(data);
    SPI_disable();
}

// function used to write to multiple registers
void writeMultiple(unsigned short address, unsigned char data[], unsigned char control, unsigned int size)
{
    unsigned int __xdata i = 0;
    SPI_enable();
    SPI_transfer(address >> 8);
    SPI_transfer(address & 0xFF);        
    SPI_transfer(control);
    for(i = 0; i < size; i++)
    {
    SPI_transfer(data[i]);
    }
    SPI_disable();

}

// function to read from 1 register
unsigned char readReg(unsigned short address, unsigned char control) 
{
    unsigned char __xdata received_data;
    SPI_enable();
    SPI_transfer(address >> 8);   
    SPI_transfer(address & 0xFF); 
    SPI_transfer(control);
    received_data = SPI_receive();
    SPI_disable();
    return received_data;  
}

// function to read from multiple registers
unsigned char* readMultiple(unsigned short address, unsigned char control, unsigned int size)
{
    unsigned int __xdata i = 0;
    unsigned char* __xdata buffer;
    SPI_enable();
    SPI_transfer(address >> 8);
    SPI_transfer(address & 0xFF);        
    SPI_transfer(control);
    for(i = 0; i < size; i++)
    {
      buffer[i] = SPI_receive();
    }
    SPI_disable();

    return buffer;
}

// function to calculate control byte
unsigned char get_control_byte(unsigned char BSB, unsigned char r_w, unsigned char OM)
{
    unsigned char __xdata control = (BSB<<3)|(r_w<<2)|(OM);
    return control;
}

void enable_W5500_interrupts() {
    writeReg(SIMR_ADDR, 0x03, get_control_byte(0x00, 1, 0));  // Enable interrupts for Socket 0 (UDP) and Socket 1 (TCP)
}

void ISR_INT0() __interrupt 0 {
    w5500_interrupt_triggered = 1;
}

void INT0_init() {
    IT0 = 1;     // Configure INT0 for edge-triggered interrupt
    EX0 = 1;     // Enable external interrupt 0
    EA = 1;      // Enable global interrupts
}

void setup_W5500_common_registers(unsigned char __xdata mode, unsigned char __xdata ip[4], unsigned char __xdata mac[6], unsigned char __xdata subnet[4], unsigned char __xdata gateway[4]) {

    writeReg(0x0000, mode, get_control_byte(0x0000, 1, 00)); // setting the mode 
    writeMultiple(0x0009, mac, get_control_byte(0x0000, 1, 00), 6); // setting the MAC of the WIZNET
    writeMultiple(0x000F, ip, get_control_byte(0x0000, 1, 00), 4); // setting the IP of the WIZNET
    writeMultiple(0x0005, subnet, get_control_byte(0x0000, 1, 00), 4); // setting the subnet of the WIZNET
    writeMultiple(0x0001, gateway, get_control_byte(0x0000, 1, 00), 4); // setting the gateway of the WIZNET

}

void set_socket_dest(unsigned char __xdata ip[4], unsigned char __xdata port[2])
{
    writeMultiple(0x000C, ip, get_control_byte(0x01, 1, 00), 4); // setting the destination IP
    writeMultiple(0x0010, port, get_control_byte(0x01, 1, 00), 2); // setting the destination port
}

void setup_UDP_socket() {
    
    writeReg(0x0000, 0x02, get_control_byte(0x01, 1, 00));  // set socket to UDP mode

    writeReg(0x0004, 0x00, get_control_byte(0x01, 1, 00));  // set the UDP port
    writeReg(0x0005, 0x50, get_control_byte(0x01, 1, 00));  
    
    writeReg(0x0001, 0x01, get_control_byte(0x01, 1, 00)); // open the UDP socket

}

void send_UDP_packet(unsigned char *data, unsigned int __xdata len) {
    unsigned short tx_write_pointer; 

    tx_write_pointer = (readReg(0x0024, get_control_byte(0x01, 0, 00)) << 8) + readReg(0x0025, get_control_byte(0x01, 0, 00));  // Sn_TX_WR high and low bytes

    writeMultiple(tx_write_pointer, data, get_control_byte(0x02, 1, 00), len);
    tx_write_pointer += len; // increase Sn_TX_WR by the data length to be sent

    writeReg(0x0024, (tx_write_pointer >> 8), get_control_byte(0x01, 1, 00)); // update the Sn_TX_WR registers
    writeReg(0x0025, tx_write_pointer, get_control_byte(0x01, 1, 00));
    writeReg(0x0001, 0x20, get_control_byte(0x01, 1, 00)); // command to SEND
}

unsigned char* read_UDP_packet() {
    unsigned short rx_read_pointer; 
    unsigned char* h;
    unsigned char* buffer;

    rx_read_pointer = (readReg(0x0028, get_control_byte(0x01, 0, 00)) << 8) + readReg(0x0029, get_control_byte(0x01, 0, 00));  // Sn_RX_WR high and low bytes

    h = readMultiple(rx_read_pointer, get_control_byte(0x03, 0, 00), 8); // read the UDP header

    rx_read_pointer += 8; // increase Sn_RX_WR by the header length

    size_m = h[6] + h[7]; // get the message length from the header

    buffer = readMultiple(rx_read_pointer, get_control_byte(0x03, 0, 00), size_m); // store message in buffer
    buffer[size_m] = '\0';  // end buffer at the length
    rx_read_pointer += size_m; // incread sn_RX_WR buffer by the length of data received

    writeReg(0x0028, (rx_read_pointer >> 8), get_control_byte(0x01, 1, 00));  // update the Sn_RX_WR buffer
    writeReg(0x0029, rx_read_pointer, get_control_byte(0x01, 1, 00));  

    writeReg(0x0001, 0x40, get_control_byte(0x01, 1, 00));  // command to RECV

    return buffer;
}

void setup_TCP_server_socket()
{
    writeReg(0x0000, 0x01, get_control_byte(0x05, 1, 00)); // set socket to TCP mode

    writeReg(0x0004, 0x13, get_control_byte(0x05, 1, 00));  // set TCP port
    writeReg(0x0005, 0x88, get_control_byte(0x05, 1, 00));
    
    writeReg(0x0001, 0x01, get_control_byte(0x05, 1, 00)); // open the TCP socket
    writeReg(0x0001, 0x02, get_control_byte(0x05, 1, 00)); // listen for TCP connections

}

void send_TCP_packet(unsigned char *data, unsigned int __xdata len) {
    unsigned short tx_write_pointer; 
    tx_write_pointer = (readReg(0x0024, get_control_byte(0x05, 0, 00)) << 8) + readReg(0x0025, get_control_byte(0x05, 0, 00)); 

    writeMultiple(tx_write_pointer, data, get_control_byte(0x06, 1, 00), len);
    tx_write_pointer += len;

    writeReg(0x0024, (tx_write_pointer >> 8), get_control_byte(0x05, 1, 00));
    writeReg(0x0025, tx_write_pointer, get_control_byte(0x05, 1, 00));
    writeReg(0x0001, 0x20, get_control_byte(0x05, 1, 00));
}

unsigned char* receive_TCP_packet(unsigned int __xdata len)
{
    unsigned short rx_read_pointer;
    unsigned char* buffer;

    rx_read_pointer = (readReg(0x0028, get_control_byte(0x05, 0, 0)) << 8) + readReg(0x0029, get_control_byte(0x05, 0, 0));

    buffer = readMultiple(rx_read_pointer, get_control_byte(0x07, 0, 0), len);
    buffer[len] = '\0';
    rx_read_pointer += len;
    
    writeReg(0x0028, (rx_read_pointer >> 8), get_control_byte(0x05, 1, 00));  
    writeReg(0x0029, rx_read_pointer, get_control_byte(0x05, 1, 00));  

    writeReg(0x0001, 0x40, get_control_byte(0x05, 1, 00));  

    return buffer;
}

// function for recognizing UART commands on input
void process_command(unsigned char* command) {
    unsigned char *value;
    UART_print(cmd_buffer);
    UART_print("\r\n");
    if (strcmp(command, "clear\r") == 0) 
    {
        clear_screen();
        UART_print("Screen cleared. Press '?' to display configuration.\r\n");
    }
    else if (strncmp(command, "RTU=", 4) == 0)  
    {
        value = command + 4; 
        RTU_address = value[0];  
        UART_print("RTU Address updated.\r\n");
    }

    else if (strncmp(command, "IP=", 3) == 0) {
        parse_ip_address(&command[3], ip_addr);
        UART_print("IP address updated.\r\n");
        setup_W5500_common_registers(0x00, ip_addr, mac_addr, subnet_mask, gateway_addr);
        setup_TCP_server_socket();
    }

     else if (strncmp(command, "SUB=", 4) == 0) {
        parse_ip_address(&command[4], subnet_mask);
        UART_print("Subnet mask updated.\r\n");
        setup_W5500_common_registers(0x00, ip_addr, mac_addr, subnet_mask, gateway_addr);
    }

    else if (strncmp(command, "GATE=", 5) == 0) {
        parse_ip_address(&command[5], gateway_addr);
        UART_print("Gateway address updated.\r\n");
        setup_W5500_common_registers(0x00, ip_addr, mac_addr, subnet_mask, gateway_addr);
    }
}

// function for handling UART inputs
void handle_UART_input() {
    if (uart_data_received) {
        uart_data_received = 0;
            
        if (received_char == '?') {
                cmd_buffer[cmd_index] = '\0';
                cmd_index = 0;
                clear_screen();
                display_config();
        } 

        else if (received_char == '\r') {
                cmd_buffer[cmd_index] = '\0'; 
                process_command(cmd_buffer);
                cmd_index = 0; 
        }
    }
}

// function to check the format of the message and the RTU 
__bit process_message(unsigned char* package) {
    unsigned char i = 3;
    if (package[0] != ':' || package[2] != '<') {
        UART_print("Invalid format\r\n");
        return 0;  
    }

    if (package[1] != RTU_address) {
        UART_print("Invalid RTU address\r\n");
        return 0;  
    }
  
    while (package[i] != '>') {
        if (package[i] >= 'a' && package[i] <= 'z') {
            package[i] = package[i] - 32;  
        }
        i++;
    }

    package[2] = '[';
    package[i] = ']';
    package[i + 1] = '\0'; 
    return 1;  
}

// function to listen for a TCP connection again once a connection is closed
void check_and_reset_TCP_socket() {
    status = readReg(0x0003, get_control_byte(0x05, 0, 0));

    if (status == 0x00 || status == 0x1C) {
        setup_TCP_server_socket();  
    }
}

void enter_PD_mode() {
    PCON |= 0x02; 
}

void main()
{
    unsigned char __xdata mode = 0;
    unsigned char __xdata dest_ip[4] = {192,168,16,120};
    unsigned char __xdata dest_port[2] = {0xD4, 0x31};
    unsigned short rsr = 0;
    unsigned int received_size;
    unsigned char* buffer;
    unsigned char s0_ir;
    unsigned char s1_ir;
    SPI_init();
    UART_init(9600);
    setup_W5500_common_registers(mode, ip_addr, mac_addr, subnet_mask, gateway_addr);
    setup_UDP_socket();
    setup_TCP_server_socket();
    set_socket_dest(dest_ip, dest_port);
    LED_init();
    INT0_init();
    enable_W5500_interrupts();
    while(1)
    {
        
        handle_UART_input();
        
        if (w5500_interrupt_triggered)
        {
            readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0));
            
            w5500_interrupt_triggered = 0;  
            s0_ir = readReg(Sn_IR_ADDR, get_control_byte(0x01, 0, 0)); // interrupt for Socket 0
            s1_ir = readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0)); // interrupt for Socket 1

            if(s0_ir & 0x04) // if interrupt receives a packet
            {
                TX_LED = 1;
                buffer = read_UDP_packet();
                TX_LED = 0;
                if (process_message(buffer)) 
                {
                    RX_LED = 1;
                    send_UDP_packet(buffer, size_m);
                    RX_LED = 0;
                }
                
                if (readReg(Sn_IR_ADDR, get_control_byte(0x01, 0, 0)) & 0x10) // if interrupt detects closing of socket
                {
                    writeReg(Sn_IR_ADDR, 0x10, get_control_byte(0x01, 1, 0)); // refresh the interrupt
                }

                writeReg(Sn_IR_ADDR, 0x04, get_control_byte(0x01, 1, 0));
            }
            
            readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0));
            
            if (s1_ir & 0x01) // if interrupt detects opening of TCP socket
            {
                writeReg(Sn_IR_ADDR, 0x01, get_control_byte(0x05, 1, 0)); // refresh the interrupt
            }

            if (s1_ir & 0x04) // if interrupt receives a packet
            {
                received_size = (readReg(0x0026, get_control_byte(0x05, 0, 0)) << 8) + readReg(0x0027, get_control_byte(0x05, 0, 0)); // get the size of the received packet
                
                if (received_size > 0) 
                {
                    buffer = receive_TCP_packet(received_size);
                    if (process_message(buffer)) 
                    {
                        send_TCP_packet(buffer, received_size);
                    }
                }
                writeReg(Sn_IR_ADDR, 0x04, get_control_byte(0x05, 1, 0)); // refresh the interrupt
                readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0));
            }

            readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0));

            if (readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0)) & 0x10) // if connection breaks 
            {
                writeReg(Sn_IR_ADDR, 0x10, get_control_byte(0x05, 1, 0)); // refresh the interrupt (0x10 for connection break and 0x02 for listen)
                readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0));
            }

            if (readReg(Sn_IR_ADDR, get_control_byte(0x05, 0, 0)) & 0x02) // if interrupt detects opening of TCP socket
            {
                writeReg(Sn_IR_ADDR, 0x02, get_control_byte(0x05, 1, 0)); // refresh the interrupt
                check_and_reset_TCP_socket();
            }

        }
               
    }
} 
