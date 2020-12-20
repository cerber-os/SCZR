## Project for Real-time Operating Systems subject at Warsaw University of Technology

//TODO: README

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
