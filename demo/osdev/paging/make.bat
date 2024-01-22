aasm PageApp.a -I../../../inc/Kasha/n_ -o ../../../../_obj/PageApp.bin
aasm PageSub.a -I../../../inc/Kasha/n_ -o ../../../../_obj/PageSub.bin
ffset E:/vhd.vhd ../../../../_obj/PageApp.bin 1
ffset E:/vhd.vhd ../../../../_obj/PageSub.bin 50