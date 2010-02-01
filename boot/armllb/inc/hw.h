/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/hw.h
 * PURPOSE:         LLB Hardware Functions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

VOID
NTAPI
LlbHwInitialize(
    VOID
);

ULONG
NTAPI
LlbHwGetScreenWidth(
    VOID
);
 
ULONG
NTAPI
LlbHwGetScreenHeight(
    VOID
);

ULONG
NTAPI
LlbHwVideoCreateColor(
    IN ULONG Red,
    IN ULONG Green,
    IN ULONG Blue
);

PVOID
NTAPI
LlbHwGetFrameBuffer(
    VOID
);

ULONG
NTAPI
LlbHwGetBoardType(
    VOID
);

ULONG
NTAPI
LlbHwGetPClk(
    VOID
);

ULONG
NTAPI
LlbHwGetTmr0Base(
    VOID
);
 
ULONG
NTAPI
LlbHwGetUartBase(
    IN ULONG Port
);

ULONG
NTAPI
LlbHwGetSerialUart(
    VOID
);
 
VOID
NTAPI
LlbHwUartSendChar(
    IN CHAR Char
);

BOOLEAN
NTAPI
LlbHwUartTxReady(
    VOID
);

#ifdef _VERSATILE_
#include "versa.h"
#elif _OMAP3_
#include "omap3.h"
#endif

/* EOF */
