cd ~/develop/kernel/chardev_test/C_LAN_Chatroom/

make

sudo insmod chat_device.ko

make -f MakeFileForChatApp

sudo ./chat_client

dmesg | tail


