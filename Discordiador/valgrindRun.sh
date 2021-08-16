#i /bin/bash

echo "#============ Started Valgrind Discordiador ============#"

LD_LIBRARY_PATH="/home/utnso/tp-2021-1c-ITec-SO/Utils/Debug" ./Debug/Discordiador CFLAGS=-g &
PID=$!
echo Terminando el proceso $PID
kill -SIGINT $PID
LD_LIBRARY_PATH="/home/utnso/tp-2021-1c-ITec-SO/Utils/Debug" valgrind --leak-check=full ./Debug/Discordiador

echo "#============ Valgrind Report Finished  ============#"