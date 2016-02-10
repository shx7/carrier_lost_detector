## carrier_lost_detector
Simulation carrier lost event in linux kernel
Main aim of this project is figure out a way how to manipulate of carrier state in linux kernel simultaneously.

####Project contains:
- Kprobes implementation. Useless due to *probes does not allow to modify kernel structures
- Trampoline implementation as kernel module. Actually works, but requires hardcoded address of netif_* kernel functions.
  Also sometimes causes kernel panic...
- Netlink test program
