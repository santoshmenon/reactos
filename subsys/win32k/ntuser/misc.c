/* $Id: misc.c,v 1.15 2003/08/29 09:29:11 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Misc User funcs
 * FILE:             subsys/win32k/ntuser/misc.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY:
 *       2003/05/22  Created
 */
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>
#include <win32k/win32k.h>
#include <win32k/dc.h>
#include <include/error.h>
#include <include/window.h>
#include <include/painting.h>
#include <include/dce.h>
#include <include/mouse.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>


/*
 * @implemented
 */
DWORD
STDCALL
NtUserCallNoParam(
  DWORD Routine)
{
  NTSTATUS Status;
  DWORD Result = 0;
  PWINSTATION_OBJECT WinStaObject;

  switch(Routine)
  {
    case NOPARAM_ROUTINE_GETDOUBLECLICKTIME:
      Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                           KernelMode,
                                           0,
                                           &WinStaObject);
      if (!NT_SUCCESS(Status))
        return (DWORD)FALSE;

      Result = WinStaObject->SystemCursor.DblClickSpeed;
      
      ObDereferenceObject(WinStaObject);
      return Result;
  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallNoParam()\n", Routine);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
NtUserCallOneParam(
  DWORD Param,
  DWORD Routine)
{
  NTSTATUS Status;
  DWORD Result = 0;
  PWINSTATION_OBJECT WinStaObject;
  PWINDOW_OBJECT WindowObject;
  
  switch(Routine)
  {
    case ONEPARAM_ROUTINE_GETMENU:
      WindowObject = IntGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      
      Result = (DWORD)WindowObject->Menu;
      
      IntReleaseWindowObject(WindowObject);
      return Result;
      
    case ONEPARAM_ROUTINE_ISWINDOWUNICODE:
      WindowObject = IntGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      Result = WindowObject->Unicode;
      IntReleaseWindowObject(WindowObject);
      return Result;
      
    case ONEPARAM_ROUTINE_WINDOWFROMDC:
      return (DWORD)IntWindowFromDC((HDC)Param);
      
    case ONEPARAM_ROUTINE_GETWNDCONTEXTHLPID:
      WindowObject = IntGetWindowObject((HWND)Param);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
      }
      
      Result = WindowObject->ContextHelpId;
      
      IntReleaseWindowObject(WindowObject);
      return Result;
    case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
      Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                           KernelMode,
                                           0,
                                           &WinStaObject);
      if (!NT_SUCCESS(Status))
        return (DWORD)FALSE;

      Result = (DWORD)IntSwapMouseButton(WinStaObject, (BOOL)Param);

      ObDereferenceObject(WinStaObject);
      return Result;

  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam()\n Param=0x%x\n", 
          Routine, Param);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
NtUserCallTwoParam(
  DWORD Param1,
  DWORD Param2,
  DWORD Routine)
{
  NTSTATUS Status;
  PWINDOW_OBJECT WindowObject;
  PSYSTEM_CURSORINFO CurInfo;
  PWINSTATION_OBJECT WinStaObject;
  PPOINT Pos;
  
  switch(Routine)
  {
    case TWOPARAM_ROUTINE_ENABLEWINDOW:
	  UNIMPLEMENTED
      return 0;

    case TWOPARAM_ROUTINE_UNKNOWN:
	  UNIMPLEMENTED
	  return 0;

    case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
	  UNIMPLEMENTED
	  return 0;

    case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
	  UNIMPLEMENTED
	  return 0;

    case TWOPARAM_ROUTINE_VALIDATERGN:
      return (DWORD)NtUserValidateRgn((HWND) Param1, (HRGN) Param2);
      
    case TWOPARAM_ROUTINE_SETWNDCONTEXTHLPID:
      WindowObject = IntGetWindowObject((HWND)Param1);
      if(!WindowObject)
      {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return (DWORD)FALSE;
      }
      
      WindowObject->ContextHelpId = Param2;
      
      IntReleaseWindowObject(WindowObject);
      return (DWORD)TRUE;
      
    case TWOPARAM_ROUTINE_CURSORPOSITION:
      if(!Param1)
        return (DWORD)FALSE;
      Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                           KernelMode,
                                           0,
                                           &WinStaObject);
      if (!NT_SUCCESS(Status))
        return (DWORD)FALSE;
        
      Pos = (PPOINT)Param1;
      
      if(Param2)
      {
        /* set cursor position */
        
        CurInfo = &WinStaObject->SystemCursor;
        /* FIXME - check if process has WINSTA_WRITEATTRIBUTES */
        
        //CheckClipCursor(&Pos->x, &Pos->y, CurInfo);  
        if((Pos->x != CurInfo->x) || (Pos->y != CurInfo->y))
        {
          MouseMoveCursor(Pos->x, Pos->y);
        }

      }
      else
      {
        /* get cursor position */
        /* FIXME - check if process has WINSTA_READATTRIBUTES */
        Pos->x = WinStaObject->SystemCursor.x;
        Pos->y = WinStaObject->SystemCursor.y;
      }
      
      ObDereferenceObject(WinStaObject);
      
      return (DWORD)TRUE;

  }
  DPRINT1("Calling invalid routine number 0x%x in NtUserCallOneParam()\n Param1=0x%x Parm2=0x%x\n",
          Routine, Param1, Param2);
  SetLastWin32Error(ERROR_INVALID_PARAMETER);
  return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
NtUserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni)
{
  /* FIXME: This should be obtained from the registry */
  static LOGFONTW CaptionFont =
  { 14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
    0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, L"" };
/*  { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
    0, 0, DEFAULT_QUALITY, FF_MODERN, L"Bitstream Vera Sans Bold" };*/
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject;
  
  switch(uiAction)
  {
    case SPI_SETDOUBLECLKWIDTH:
    case SPI_SETDOUBLECLKHEIGHT:
    case SPI_SETDOUBLECLICKTIME:
      Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                           KernelMode,
                                           0,
                                           &WinStaObject);
      if (!NT_SUCCESS(Status))
        return (DWORD)FALSE;
      
      switch(uiAction)
      {
        case SPI_SETDOUBLECLKWIDTH:
          /* FIXME limit the maximum value? */
          WinStaObject->SystemCursor.DblClickWidth = uiParam;
          break;
        case SPI_SETDOUBLECLKHEIGHT:
          /* FIXME limit the maximum value? */
          WinStaObject->SystemCursor.DblClickHeight = uiParam;
          break;
        case SPI_SETDOUBLECLICKTIME:
          /* FIXME limit the maximum time to 1000 ms? */
          WinStaObject->SystemCursor.DblClickSpeed = uiParam;
          break;
      }
      
      /* FIXME save the value to the registry */
      
      ObDereferenceObject(WinStaObject);
      return TRUE;

    case SPI_GETWORKAREA:
      {
        ((PRECT)pvParam)->left = 0;
        ((PRECT)pvParam)->top = 0;
        ((PRECT)pvParam)->right = 640;
        ((PRECT)pvParam)->bottom = 480;
        return TRUE;
      }
    case SPI_GETICONTITLELOGFONT:
      {
        memcpy(pvParam, &CaptionFont, sizeof(CaptionFont));
        return TRUE;
      }
    case SPI_GETNONCLIENTMETRICS:
      {
        LPNONCLIENTMETRICSW pMetrics = (LPNONCLIENTMETRICSW)pvParam;
    
        if (pMetrics->cbSize != sizeof(NONCLIENTMETRICSW) || 
            uiParam != sizeof(NONCLIENTMETRICSW))
        {
          return FALSE;
        }

        memset((char *)pvParam + sizeof(pMetrics->cbSize), 0,
          pMetrics->cbSize - sizeof(pMetrics->cbSize));

        pMetrics->iBorderWidth = 1;
        pMetrics->iScrollWidth = NtUserGetSystemMetrics(SM_CXVSCROLL);
        pMetrics->iScrollHeight = NtUserGetSystemMetrics(SM_CYHSCROLL);
        pMetrics->iCaptionWidth = NtUserGetSystemMetrics(SM_CXSIZE);
        pMetrics->iCaptionHeight = NtUserGetSystemMetrics(SM_CYSIZE);
        memcpy((LPVOID)&(pMetrics->lfCaptionFont), &CaptionFont, sizeof(CaptionFont));
        pMetrics->lfCaptionFont.lfWeight = FW_BOLD;
        pMetrics->iSmCaptionWidth = NtUserGetSystemMetrics(SM_CXSMSIZE);
        pMetrics->iSmCaptionHeight = NtUserGetSystemMetrics(SM_CYSMSIZE);
        memcpy((LPVOID)&(pMetrics->lfSmCaptionFont), &CaptionFont, sizeof(CaptionFont));
        pMetrics->iMenuWidth = NtUserGetSystemMetrics(SM_CXMENUSIZE);
        pMetrics->iMenuHeight = NtUserGetSystemMetrics(SM_CYMENUSIZE);
        memcpy((LPVOID)&(pMetrics->lfMenuFont), &CaptionFont, sizeof(CaptionFont));
        memcpy((LPVOID)&(pMetrics->lfStatusFont), &CaptionFont, sizeof(CaptionFont));
        memcpy((LPVOID)&(pMetrics->lfMessageFont), &CaptionFont, sizeof(CaptionFont));
        return TRUE;
      }
    
  }
  return FALSE;
}
