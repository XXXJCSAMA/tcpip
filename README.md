# Project Description
This warehouse has placed the "Write TCP/IP protocol stack from 0" course source code. The goal of this course is to show how to write a small, portable TCP/IP stack starting with 0 lines of code.

** Project contains about 10,000 + lines of code, more than 10 protocols, more than 20 socket interfaces, 10 network applications **

# Protocol stack features
Platform support
* Support development and learning on Windows, Linux and Mac platforms
* Using the C99 standard C language, does not rely on a specific compiler
* Code portability is strong and can be easily ported to x86 and embedded platforms such as ARM
* Only basic semaphore, mutex, and time acquisition interfaces are required
Hardware support
* Does not limit the specific network interface type, can support other interface devices outside the Ethernet
* Support to add multiple network interfaces (nics), IP packets will automatically be sent to the correct interface according to the routing table
* Support loopback interface, to achieve the local packet self-collection
Functional characteristics
* Standard Socket interface:
* Interface: socket, bind, connnect, close
* Interface: sendto, recvfrom, send, recv, read, write, setsockopt
* Specific type
* SOCK_RAW: Allows applications to send and receive IP packets
* SOCK_DGRAM based: allows applications to send and receive
* Multithreading
* Supports multi-threaded operation of the same socket interface, that is, allows different threads to read and write at the same time
* Support the creation of any number of applications at the same time, whether client or server
* Data packet
* The packet is organized into multiple blocks of links to improve memory utilization efficiency
Protocol support
* Ethernet protocol: Support Ethernet packet sending and receiving and packet processing
* Address resolution Protocol ARP
* Complete support for ARP query and response processes
* Support no return (free) ARP packet sending
* Use a configurable ARP cache to improve query efficiency
* Automatically clean up invalid cache entries by updating ARP cache periodically for a certain period of time
* The input ARP and IP packets can be used to update the cache, reducing network traffic
* IPv4 Protocol
* Support basic IP packet sending and receiving, checksum calculation
* Support IP packet fragmentation and reassembly, reassembly timeout processing
* Built-in routing table for Internet access via router
* ICMPv4 protocol
* Supports response to incoming echo requests (i.e. can be pinged by others)
* Provides information that the destination port is unreachable to notify other machines of an access error
* UDP Protocol
* Basic UDP input/output processing to correctly pass input packets to the application
* TCP Protocol
* Support TCP state machine switching
* Timer based data sending timeout retransmission
* Calculate RTO adaptively according to the RTT situation to reduce data retransmission operations
* Fast retransmission based on repeated ACK
* Zero window with TCP persistent timer
* Application Layer protocol:
* Network Time Service NTP
■ The client can query the NTP server to obtain the current time
* Simple File Transfer Protocol TFTP
■ Implement TFTP client: you can upload and download files from the server
■ Implement TFTP server: allows clients to upload and download files to the server
■ Supports TFTP request handling with options
■ Automatic retransmission when data packet is lost
* Hypertext Transfer Protocol HTTP:
■ Implementation of multi-threaded version of the HTTP server, can provide the client with web access functions
* Domain Name Resolution Protocol DNS
■ Support domain name resolution and IP address conversion
■ Support for adding multiple DNS servers
■ Cache DNS query results, automatically deleted after timeout
■ Allows multiple tasks to be queried simultaneously
* DHCP Protocol (under development)

