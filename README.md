FreeBSD: Generic Segmentation Offload (GSO)
===========

The use of large frames makes network communication much less demanding for the CPU. Yet, backward compatibility and slow links requires the use of 1500 byte or smaller frames.
Modern NICs with hardware TCP segmentation offloading (TSO) address this problem. However, a generic software version (GSO) provided by the OS has reason to exist, for use on paths with no suitable hardware, such as between virtual machines or with older or buggy NICs.

A lot of the savings is provided by crossing the network stack once rather than many times for each large packet, but this saving can be obtained without hardware support. In addition, this mechanism can also be extended to protocols that require IP fragmentation, such as UDP.
The idea to reduce the CPU overhead is to postpone the TCP segmentation or IP fragmentation as late as possible. A solution would be to perform the segmentation within the device driver, but this requires changing every device driver. Therefore, the best solution is to segment just before the packet is passed to the driver (in <code>ether_output()</code>).

This preliminary implementation supports TCP, UDP on IPv4/IPv6.
In the TCP case, when a packet is created, if the GSO is active, it may be larger than the MTU, in this case the super-packet is marked with a flag (contained in <code>m_pkthdr.csum_flags</code>) to make sure that this is divided into smaller packets just before calling the device driver. In this way the functions that create the headers for layers TCP, IP and Ethernet are crossed once. Finally, the super-packet headers are copied and adjusted in all packets after performing the segmentation of the payload. 
A TCP packet can be divided into smaller packets without performing IP fragmentation, because TCP is a stream-oriented protocol. Instead, in the UDP case, IP fragmentation is required. However, also in this case, the fragmentation can be delayed and performed just before calling the device driver. In this way, the MAC layer is traversed only once, and the MAC header, the same for all, is simply copied into all fragments.

The experiments performed with TCP provide significant results if the receiver is able to perform aggregation hardware or software (RSS or LRO). We also noticed that by lowering the clock frequency of the transmitter, the speedup increases. This occurs because with maximum frequency, the system is able to saturate the link. Also with UDP traffic, we can notice a speedup given by the GSO.

Our preliminary implementation, depending on CPU speed, shows up to 95% speedup compared to segmentation done in the TCP/IPv4 stack, saturating a 10 Gbit link at 2 GHz with checksum offloading [Tab. 1].

##Patches & utilities

In this repo you can find: 
 * the kernel patches for
	* FreeBSD-current
	* FreeBSD 10-stable
	* FreeBSD 9-stable
 * a simple application that prints the GSO statistics:
	* gso-stat.c

===========

##How to use GSO

* Apply the right kernel patch.
	* FreeBSD-current
	* FreeBSD 10-stable
	* FreeBSD 9-stable

* To compile the GSO support add '**options GSO**' to your kernel config file and rebuild a kernel.

* To manage the GSO parameters there are some sysctls:
     * **net.inet.tcp.gso** - GSO enable on TCP communications (!=0)
     * **net.inet.udp.gso** - GSO enable on UDP communications (!=0)
 
     * for each interface:
          * **net.gso.dev."ifname”.max_burst** - GSO burst length limit [default: IP_MAXPACKET=65535]
          * **net.gso.dev."ifname”.enable_gso** - GSO enable on “ifname” interface (!=0)

* To show statistics:
     make sure that the GSO_STATS macro is defined in sys/net/gso.h
     use the simple gso-stats.c application to access the sysctl net.gso.stats that contains the address of the gsostats structure (defined in gso.h) which records the statistics. (compile with -I/path/to/kernel/src/patched/)

===========

##Experiments

* Test Date: Sep 9, 2014
* Transmitter: FreeBSD 11-CURRENT - CPU i7-870 at 2.93 GHz + Turboboost, Intel 10 Gbit NIC.
* Receiver: Linux 3.12.8 - CPU i7-3770K at 3.50GHz + Turboboost, Intel 10 Gbit NIC.
* Benchmark tool: netperf 2.6.0


| Freq. |   TSO   |   GSO   |   none   |  Speedup  |
| :----  | :-----: | :----:  | :----:   | -------: |
| *[GHz]* | *[Gbps]* | *[Gbps]*| *[Gbps]* | **GSO-none** |
| *2.93* | 9347 | 9298 | 8308 | **11.92 %** |
| *2.53* | 9266 | 9401 | 6771 | **38.84 %** |
| *2.00* | 9408 | 9294 | 5499 | **69.01 %** |
| *1.46* | 9408 | 8087 | 4075 | **98.45 %** |
| *1.05* | 9408 | 5673 | 2884 | **96.71 %** |
| *0.45* | 6760 | 2206 | 1244 | **77.33 %** |
Tab.1 TCP/IPv4 packets (checksum offloading enabled) 

-----------

| Freq. |   TSO   |   GSO   |   none   |  Speedup  |
| :----  | :-----: | :----:  | :----:   | -------: |
| *[GHz]* | *[Gbps]* | *[Gbps]*| *[Gbps]* | **GSO-none** |
| *2.93* | 7530 | 6939 | 4966 | **39.73 %** |
| *2.53* | 5133 | 7145 | 4008 | **78.27 %** |
| *2.00* | 5965 | 6331 | 3152 | **100.86 %** |
| *1.46* | 5565 | 5180 | 2348 | **120.61 %** |
| *1.05* | 8501 | 3607 | 1732 | **108.26 %** |
| *0.45* | 3665 | 1505 |  651 | **131.18 %** |
Tab.2 TCP/IPv6 packets (checksum offloading enabled)  

-----------

| Freq. |   GSO   |   none   |  Speedup  |
| :----  | :----:  | :----:   | -------: |
| *[GHz]* | *[Gbps]*| *[Gbps]* | **GSO-none** |
| *2.93* | 9440 | 8084 | **16.77 %** |
| *2.53* | 7772 | 6649 | **16.89 %** |
| *2.00* | 6336 | 5338 | **18.70 %** |
| *1.46* | 4748 | 4014 | **18.29 %** |
| *1.05* | 3359 | 2831 | **18.65 %** |
| *0.45* | 1312 | 1120 | **17.14 %** |
Tab.3 UDP/IPv4 packets

-----------

| Freq. |   GSO   |   none   |  Speedup  |
| :----  | :----:  | :----:   | -------: |
| *[GHz]* | *[Gbps]*| *[Gbps]* | **GSO-none** |
| *2.93* | 7281 | 6197 | **17.49 %** |
| *2.53* | 5953 | 5020 | **18.59 %** |
| *2.00* | 4804 | 4048 | **18.68 %** |
| *1.46* | 3582 | 3004 | **19.24 %** |
| *1.05* | 2512 | 2092 | **20.08 %** |
| *0.45* |  998 |  826 | **20.82 %** |
Tab.4 UDP/IPv6 packets

-----------
