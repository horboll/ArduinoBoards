# PeoBoards Arduino based boards

Custom Arduino boards

Add `https://raw.githubusercontent.com/horboll/ArduinoBoards/master/package_peoboards_index.json` to your Arduino boards manager sources


## Build board zip

Clone https://github.com/possan/ArduinoCore-samd
Compress it: `zip -r PeoBoards-LoraBoardM0Mk2-x.x.xx.zip ArduinoCore-samd`
Update version in index
Upload


## Build bootloader

Clone https://github.com/possan/uf2-samd21
Run `make`
Copy firmware binaries in `build/peoboards_lora_board_m0_mk2/` to board file repo `bootloaders/peoboards_lora_board_m0_mk`
