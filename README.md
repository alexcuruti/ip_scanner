# **Multithreading IP scanner**
A relatively simple project for the operating systems course featuring a multithreading ip scanner implemented in C. The program scans a given range of IP addresses and ports, checks for active hosts and open ports using pthreads for effective parallel execution. It also retrieves the MAC address of active hosts and writes everything successfully found into an output txt file.

# **Features**
* Scans a specified range of IP addresses to check for active hosts
* Uses `ping` to determine if an IP is active
* Checks for open ports on active hosts using socket connections
* Retrieves MAC addresses of active hosts using `arp`
* Uses multithreading to speed up the scanning process
* Supports custom IP ranges, port ranges, and thread count
* Outputs results to an output txt file named output.txt

# **Usage**
To compile the program use the following command in a terminal inside the folder that contains the C file:
#####
    gcc -o prog multithreading_Ip_Scanner.c -lpthread

To run the scanner use the following command in that same folder:
#####
    ./prog [start_ip] [end_ip] [start_port] [end_port] [thread_count]

* start_ip (optional): Beginning of IP range (default: 192.168.1.1)
* end_ip (optional): End of IP range (default: 192.168.255.255)
* start_port (optional): Start of port range (default: 1)
* end_port (optional): End of port range (default: 1000)
* thread_count (optional): Number of threads to use (default: 10)

# **Example**
#####
    ./prog 192.168.1.1 192.168.1.80 1 100 10

# **Output**
Any active IPs, open ports and MAC addresses will be written in `output.txt`.


# **Dependencies**

* GCC compiler
* `ping` and `arp` available on the system
