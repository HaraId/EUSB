UDE основные файлы для подключения:

1)#include <ude/1.0/UdeCx.h>


2)udecxstub.lib

<AdditionalDependencies>$(DDK_LIB_PATH)\ude\1.0\udecxstub.lib;usbdex.lib;WdmSec.lib;%(AdditionalDependencies)</AdditionalDependencies>



Для SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
 подключать $(DDK_LIB_PATH)\wdmsec.lib

Если он не может проверить подпись - перевести время (очень странно)