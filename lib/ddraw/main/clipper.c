/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/clipper.c
 * PURPOSE:              DirectDraw Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT WINAPI Main_DirectDrawClipper_SetHwnd(
    LPDIRECTDRAWCLIPPER iface, DWORD dwFlags, HWND hWnd) 
{
   	DX_STUB;
}

ULONG WINAPI Main_DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface) 
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawClipper_GetClipList(
    LPDIRECTDRAWCLIPPER iface, LPRECT lpRect, LPRGNDATA lpClipList,
    LPDWORD lpdwSize)
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawClipper_SetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD dwFlag) 
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawClipper_QueryInterface(
    LPDIRECTDRAWCLIPPER iface, REFIID riid, LPVOID* ppvObj) 
{
   	DX_STUB;
}

ULONG WINAPI Main_DirectDrawClipper_AddRef (LPDIRECTDRAWCLIPPER iface)
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawClipper_GetHWnd(
    LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr) 
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawClipper_Initialize(
     LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags) 
{
   	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawClipper_IsClipListChanged(
    LPDIRECTDRAWCLIPPER iface, BOOL* lpbChanged) 
{
   	DX_STUB;
}

static IDirectDrawClipperVtbl DirectDrawClipper_Vtable =
{
    Main_DirectDrawClipper_QueryInterface,
    Main_DirectDrawClipper_AddRef,
    Main_DirectDrawClipper_Release,
    Main_DirectDrawClipper_GetClipList,
    Main_DirectDrawClipper_GetHWnd,
    Main_DirectDrawClipper_Initialize,
    Main_DirectDrawClipper_IsClipListChanged,
    Main_DirectDrawClipper_SetClipList,
    Main_DirectDrawClipper_SetHwnd
};
