# Keysight Challenge - Time-Aware Traffic Shaper Application

A userspace tool that buffers packets received on a specified interface
and echoes them back during specific time slices, according to their
802.1Q tag priority in a round-robin fashion.

## Compilation

Makefile arguments
 - `NODEBUG=1` disables debug messages. Make sure you do so in release
    builds. Greatly affects performance.
 - `DUMMY_MODE=1` introduces dummy VLAN tags with different priorities
    to test with regular traffic.

## Usage

```bash
# create a raw socket and listen for incoming traffic
user1@target:~$ sudo ./bin/tsn ${IFACE}

# generate traffic from another host
user2@source:~$ sudo ping -c 50 -i 0.01 ${TARGET_IP}
```

Best to test it between two hosts since the pcap hook (and Wireshark)
doesn't capture packets sent via a raw socket. Pretty much the reason
why you can't see your DHCP queries with Wireshark.

## TODOs
 - Reduce sender thread's mutex abuse; notify it from the receiver
   thread when content is available.
 - Clean up code? :/
