import socket
import time
import curses
import random
import string

WIZNET_IP = "192.168.16.222"  
WIZNET_UDP_PORT = 80    
WIZNET_TCP_PORT = 5000       

LOCAL_IP = "192.168.16.120"
LOCAL_PORT = 54321

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind((LOCAL_IP, LOCAL_PORT))

packets_sent = 0
packets_received = 0
error_rate = 0.0
last_message_sent = ""
last_message_received = ""
response_time = 0.0


mode = input("Enter the connection mode (UDP/TCP): ").strip().upper()
if mode not in ["UDP", "TCP"]:
    print("Invalid mode selected. Please enter 'UDP' or 'TCP'.")
    exit(1)  

def format_message(rtu_address, message):
    return f":{rtu_address}<{message}>"

def send_udp_message(rtu_address, message):
    global packets_sent, last_message_sent, response_time
    formatted_message = format_message(rtu_address, message)
    sock.sendto(formatted_message.encode(), (WIZNET_IP, WIZNET_UDP_PORT))
    packets_sent += 1
    last_message_sent = formatted_message
    start_time = time.time()
    return start_time

def receive_udp_message(start_time):
    global packets_received, last_message_received, response_time, error_rate
    sock.settimeout(6)
    try:
        data, addr = sock.recvfrom(1024)
        response_time = (time.time() - start_time) * 1000
        packets_received += 1
        last_message_received = data.decode()
    except socket.timeout:
        error_rate = ((packets_sent - packets_received) / packets_sent) * 100 if packets_sent > 0 else 0

def connect_to_wiznet_tcp():
    global tcp_sock
    try:
        tcp_sock.connect((WIZNET_IP, WIZNET_TCP_PORT))
    except socket.error as e:
        print(f"Failed to connect to WIZnet TCP server: {e}")

def send_tcp_message(rtu_address, message):
    global packets_sent, last_message_sent, response_time
    formatted_message = format_message(rtu_address, message)
    try:
        tcp_sock.sendall(formatted_message.encode())
        start_time = time.time()
        packets_sent += 1
        last_message_sent = formatted_message
    except socket.error as e:
        print(f"Failed to send TCP message: {e}")
    return start_time

def receive_tcp_message(start_time):
    global packets_received, last_message_received, response_time, error_rate
    tcp_sock.settimeout(6)
    try:
        data = tcp_sock.recv(1024)
        response_time = (time.time() - start_time) * 1000
        packets_received += 1 
        last_message_received = data.decode()
    except socket.timeout:
        error_rate = ((packets_sent - packets_received) / packets_sent) * 100 if packets_sent > 0 else 0

def close_tcp_connection():
    tcp_sock.close()

def generate_random_string():
    length = random.randint(8, 16)
    letters = string.ascii_lowercase
    return ''.join(random.choice(letters) for _ in range(length))

def update_screen(stdscr):
    global packets_sent, packets_received, error_rate, last_message_sent, last_message_received, response_time, mode

    stdscr.clear()
    stdscr.border(0)
    curses.start_color()
    curses.init_pair(1, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)
    
    stdscr.attron(curses.color_pair(1))
    stdscr.addstr(1, 2, "Polling Statistics")
    stdscr.attroff(curses.color_pair(1))
    
    stdscr.addstr(3, 2, f"Packets Sent: {packets_sent}")
    stdscr.addstr(4, 2, f"Packets Received: {packets_received}")
    stdscr.attron(curses.color_pair(2))
    stdscr.addstr(5, 2, f"Error Rate: {error_rate:.2f}%")
    stdscr.attroff(curses.color_pair(2))
    stdscr.addstr(6, 2, f"Message Sent: {last_message_sent}")
    stdscr.addstr(7, 2, f"Message Rec: {last_message_received}")
    stdscr.addstr(8, 2, f"Response Time: {response_time:.2f} ms")
    stdscr.addstr(9, 2, f"Mode: {mode}")
    
    stdscr.attron(curses.color_pair(3))
    stdscr.addstr(11, 2, "Press Enter to stop pinging")
    stdscr.attroff(curses.color_pair(3))
    
    stdscr.refresh()

def main(stdscr):
    global mode

    stdscr.nodelay(True)
    stdscr.timeout(100)

    if mode == "TCP":
        connect_to_wiznet_tcp()
    
    while True:
        message = generate_random_string()
        if mode == "UDP":
            start_time = send_udp_message(3, message)
            receive_udp_message(start_time)
        elif mode == "TCP":
            start_time = send_tcp_message(3, message)
            receive_tcp_message(start_time)
        
        update_screen(stdscr)

        
        key = stdscr.getch()
        
       
        if key == ord('\n'): 
            break
        
        time.sleep(1)  

    
    if mode == "TCP":
        close_tcp_connection()

curses.wrapper(main)
