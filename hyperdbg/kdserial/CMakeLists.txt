set(SourceFiles
    "common.c"
    "hardware.c"
    "ioaccess.c"
    "uartio.c"
    "apm88xxxx.c"
    "bcm2835.c"
    "msm8974.c"
    "msm8x60.c"
    "mx6uart.c"
    "nvidia.c"
    "omap.c"
    "pl011.c"
    "sam5250.c"
    "sdm845.c"
    "spimax311.c"
    "uart16550.c"
    "usif.c"
    "kdcom.h"
    "uartp.h"
    "common.h"
    "win11sdk.h"
)
wdk_add_library(kdserial SHARED
    KMDF 1.15
    ${SourceFiles}
)
