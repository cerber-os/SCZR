## Project for Real-time Operating Systems subject at Warsaw University of Technology

*Project uses compression algorithm from `https://github.com/ariya/FastLZ`*

### Building
1. Download buildroot repository and put it in CWD
2. Create symlinks between files/directories present in `buildroot_cfg` and `buildroot`
3. Run make
4. Enter `buildroot/` and run make. First build may take upto a few hours.


### QEMU deployment
Dmesg queues:
```
mkfifo /tmp/transmitter.in /tmp/transmitter.out
mkfifo /tmp/receiver.in /tmp/receiver.out
```

Transmitter:
```
sudo qemu-system-x86_64                     \
    -kernel output/images/bzImage           \
    -m 256M                                 \
    -usb -device usb-tablet                 \
    -device usb-host,hostbus=7,hostport=3.4 \
    -serial pipe:/tmp/transmitter           \
    -append 'console=ttyS0 mode=TRANSMITTER'
```

Receiver:
```
sudo qemu-system-x86_64                     \
    -kernel output/images/bzImage           \
    -m 256M                                 \
    -usb -device usb-tablet                 \
    -device usb-host,hostbus=7,hostport=3.3 \
    -serial pipe:/tmp/receiver              \
    -append 'console=ttyS0 mode=RECEIVER'
```

Stats collection:
```
cat /tmp/receiver.out | grep 'CSV' | tee /tmp/stats.csv
```

Enabling two-core mode add:
```
-enable-kvm                                     \
-M pc -cpu host -smp cores=2,threads=1,sockets=1 \
```
