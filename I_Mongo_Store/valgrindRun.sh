#i /bin/bash

echo "#============ Started Valgrind I_Mongo_Store ============#"

LD_LIBRARY_PATH="/home/utnso/tp-2021-1c-ITec-SO/Utils/Debug" ./Debug/I_Mongo_Store CFLAGS=-g &
PID=$!
echo Terminando el proceso $PID
kill -SIGINT $PID
LD_LIBRARY_PATH="/home/utnso/tp-2021-1c-ITec-SO/Utils/Debug" valgrind --leak-check=full ./Debug/I_Mongo_Store "N"

echo "#============ Valgrind Report Finished  ============#"