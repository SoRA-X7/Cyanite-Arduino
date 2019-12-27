make clean
sleep 0.5
make
sleep 0.5
sudo dfu-programmer atmega16u2 erase
sleep 0.5
sudo dfu-programmer atmega16u2 flash Joystick.hex
sleep 0.5
sudo dfu-programmer atmega16u2 reset
