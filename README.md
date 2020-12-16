Transmitter:
```
sudo qemu-system-x86_64 -kernel output/images/bzImage -m 256M -usb -device usb-tablet -device usb-host,hostbus=7,hostport=3.4 -append 'console=ttyS0 mode=TRANSMITTER'
```

Receiver:
```
sudo qemu-system-x86_64 -kernel output/images/bzImage -m 256M -usb -device usb-tablet -device usb-host,hostbus=7,hostport=3.3 -append 'console=ttyS0 mode=RECEIVER'
```