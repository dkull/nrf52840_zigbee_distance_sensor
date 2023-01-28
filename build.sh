# python3 -m pip install west pyelftools
set +x
#rm -rf build/
source ~/ncs/zephyr/zephyr-env.sh
python3 -m west build -b nrf52840_mdk_usb_dongle ./
uf2conv.py build/zephyr/zephyr.hex -c -f0xADA52840 && mv flash.uf2 CURRENT.UF2
