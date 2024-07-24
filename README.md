# ezlopi-bkfirmare
 
### Files not pushed
1.  software/TuyaOS/vendor/bk7231n/toolchain/gcc-arm-none-eabi-4_9-2015q1.tar
2. software/TuyaOS/vendor/bk7231n_1.3.5.zip
3. software/TuyaOS/docs

### Export SDK Path before running Build_flash.sh script
```
export TUYA_SDK_PATH=/home/maaz/GitHub/ezlopi-bkfirmware/tuya/Projects/BK7231N_TuyaOS-3.9.0
```
*Yours may be different

### Build Project
```
cd apps
```
```
cd wolf_blinky
```
```
sh ezlopi_flash.sh 1.0.0 build 
```

### Flash Binaries
```
sh ezlopi_flash.sh 1.0.0 flash /dev/ttyUSB0
```
or Build and Flash
```
sh ezlopi_flash.sh 1.0.0 build_flash /dev/ttyUSB0
```

### Giving Access to USB0
```
sudo chmod 666 /dev/ttyUSB0
```