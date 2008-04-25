#ifndef PRECOMP_H__
#define PRECOMP_H__

#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0800
#include <stdio.h>
#include <windows.h>
#include <limits.h>
#include <setupapi.h>
#include <devguid.h>
#include <commctrl.h>

#include <dinput.h>
#include <dsound.h>
#include "resource.h"

typedef struct
{
	HWND hDialogs[7];

}DXDIAG_CONTEXT, *PDXDIAG_CONTEXT;



/* globals */
extern HINSTANCE hInst;

/* dialog wnd proc */
INT_PTR CALLBACK SystemPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DisplayPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SoundPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MusicPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InputPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NetworkPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpPageWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL GetRegValue(HKEY hBaseKey, LPWSTR SubKey, LPWSTR ValueName, DWORD Type, LPWSTR Result, DWORD Size);


#endif
