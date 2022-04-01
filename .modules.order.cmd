cmd_/home/guest/Documents/driver/modules.order := {   echo /home/guest/Documents/driver/simple.ko; :; } | awk '!x[$$0]++' - > /home/guest/Documents/driver/modules.order
