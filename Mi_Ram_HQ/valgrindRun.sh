#i /bin/bash

echo "#============ Started Valgrind MiRam ============#"

LD_LIBRARY_PATH="/home/utnso/tp-2021-1c-ITec-SO/Utils/Debug" valgrind --leak-check=full ./Debug/Mi_Ram_HQ

echo "#============ Valgrind Report Finished  ============#"