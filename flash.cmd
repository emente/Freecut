avrdude -P com2 -p m128 -c stk500v2 -B1 -U flash:w:freecut.hex -F
sleep 2