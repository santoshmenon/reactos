/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Driver entry and initialization of win32k
 * FILE:             subsystems/win32/win32k/main/main.c
 * PROGRAMER:
 */

#include <win32k.h>
#include <include/napi.h>

#define NDEBUG
#include <debug.h>

HANDLE hModuleWin;

PGDI_HANDLE_TABLE INTERNAL_CALL GDIOBJ_iAllocHandleTable(OUT PSECTION_OBJECT *SectionObject);
BOOL INTERNAL_CALL GDI_CleanupForProcess (struct _EPROCESS *Process);

HANDLE GlobalUserHeap = NULL;
PSECTION_OBJECT GlobalUserHeapSection = NULL;

PSERVERINFO gpsi = NULL; // Global User Server Information.

SHORT gusLanguageID;

extern ULONG_PTR Win32kSSDT[];
extern UCHAR Win32kSSPT[];
extern ULONG Win32kNumberOfSysCalls;

NTSTATUS
APIENTRY
Win32kProcessCallback(struct _EPROCESS *Process,
                      BOOLEAN Create)
{
    PPROCESSINFO Win32Process;
    DECLARE_RETURN(NTSTATUS);

    DPRINT("Enter Win32kProcessCallback\n");
    UserEnterExclusive();

    /* Get the Win32 Process */
    Win32Process = PsGetProcessWin32Process(Process);

    /* Allocate one if needed */
    if (!Win32Process)
    {
        /* FIXME - lock the process */
        Win32Process = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(PROCESSINFO),
                                             'p23W');

        if (Win32Process == NULL) RETURN( STATUS_NO_MEMORY);

        RtlZeroMemory(Win32Process, sizeof(PROCESSINFO));

        PsSetProcessWin32Process(Process, Win32Process);
        /* FIXME - unlock the process */
    }

    if (Create)
    {
        SIZE_T ViewSize = 0;
        LARGE_INTEGER Offset;
        PVOID UserBase = NULL;
        NTSTATUS Status;
        extern PSECTION_OBJECT GlobalUserHeapSection;
        DPRINT("Creating W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());

        /* map the global heap into the process */
        Offset.QuadPart = 0;
        Status = MmMapViewOfSection(GlobalUserHeapSection,
                                    PsGetCurrentProcess(),
                                    &UserBase,
                                    0,
                                    0,
                                    &Offset,
                                    &ViewSize,
                                    ViewUnmap,
                                    SEC_NO_CHANGE,
                                    PAGE_EXECUTE_READ); /* would prefer PAGE_READONLY, but thanks to RTL heaps... */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to map the global heap! 0x%x\n", Status);
            RETURN(Status);
        }
        Win32Process->HeapMappings.Next = NULL;
        Win32Process->HeapMappings.KernelMapping = (PVOID)GlobalUserHeap;
        Win32Process->HeapMappings.UserMapping = UserBase;
        Win32Process->HeapMappings.Count = 1;

        InitializeListHead(&Win32Process->ClassList);

        InitializeListHead(&Win32Process->MenuListHead);

        InitializeListHead(&Win32Process->GDIBrushAttrFreeList);
        InitializeListHead(&Win32Process->GDIDcAttrFreeList);

        InitializeListHead(&Win32Process->PrivateFontListHead);
        ExInitializeFastMutex(&Win32Process->PrivateFontListLock);

        InitializeListHead(&Win32Process->DriverObjListHead);
        ExInitializeFastMutex(&Win32Process->DriverObjListLock);

        Win32Process->KeyboardLayout = W32kGetDefaultKeyLayout();

        if(Process->Peb != NULL)
        {
            /* map the gdi handle table to user land */
            Process->Peb->GdiSharedHandleTable = GDI_MapHandleTable(GdiTableSection, Process);
            Process->Peb->GdiDCAttributeList = GDI_BATCH_LIMIT;
        }

        Win32Process->peProcess = Process;
        /* setup process flags */
        Win32Process->W32PF_flags = 0;
    }
    else
    {
        DPRINT("Destroying W32 process PID:%d at IRQ level: %lu\n", Process->UniqueProcessId, KeGetCurrentIrql());
        IntCleanupMenus(Process, Win32Process);
        IntCleanupCurIcons(Process, Win32Process);
        CleanupMonitorImpl();

        /* no process windows should exist at this point, or the function will assert! */
        DestroyProcessClasses(Win32Process);

        GDI_CleanupForProcess(Process);

        co_IntGraphicsCheck(FALSE);

        /*
         * Deregister logon application automatically
         */
        if(LogonProcess == Win32Process)
        {
            LogonProcess = NULL;
        }
    }

    RETURN( STATUS_SUCCESS);

CLEANUP:
    UserLeave();
    DPRINT("Leave Win32kProcessCallback, ret=%i\n",_ret_);
    END_CLEANUP;
}


NTSTATUS
APIENTRY
Win32kThreadCallback(struct _ETHREAD *Thread,
                     PSW32THREADCALLOUTTYPE Type)
{
    struct _EPROCESS *Process;
    PTHREADINFO Win32Thread;
    int i;
    DECLARE_RETURN(NTSTATUS);

    DPRINT("Enter Win32kThreadCallback\n");
    UserEnterExclusive();

    Process = Thread->ThreadsProcess;

    /* Get the Win32 Thread */
    Win32Thread = PsGetThreadWin32Thread(Thread);

    /* Allocate one if needed */
    if (!Win32Thread)
    {
        /* FIXME - lock the process */
        Win32Thread = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(THREADINFO),
                                            't23W');

        if (Win32Thread == NULL) RETURN( STATUS_NO_MEMORY);

        RtlZeroMemory(Win32Thread, sizeof(THREADINFO));

        PsSetThreadWin32Thread(Thread, Win32Thread);
        /* FIXME - unlock the process */
    }
    if (Type == PsW32ThreadCalloutInitialize)
    {
        HWINSTA hWinSta = NULL;
        PTEB pTeb;
        HDESK hDesk = NULL;
        NTSTATUS Status;
        PUNICODE_STRING DesktopPath;
        PRTL_USER_PROCESS_PARAMETERS ProcessParams = (Process->Peb ? Process->Peb->ProcessParameters : NULL);

        DPRINT("Creating W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

        InitializeListHead(&Win32Thread->WindowListHead);
        InitializeListHead(&Win32Thread->W32CallbackListHead);
        InitializeListHead(&Win32Thread->PtiLink);
        for (i = 0; i < NB_HOOKS; i++)
        {
            InitializeListHead(&Win32Thread->aphkStart[i]);
        }

        /*
         * inherit the thread desktop and process window station (if not yet inherited) from the process startup
         * info structure. See documentation of CreateProcess()
         */
        DesktopPath = (ProcessParams ? ((ProcessParams->DesktopInfo.Length > 0) ? &ProcessParams->DesktopInfo : NULL) : NULL);
        Status = IntParseDesktopPath(Process,
                                     DesktopPath,
                                     &hWinSta,
                                     &hDesk);
        if(NT_SUCCESS(Status))
        {
            if(hWinSta != NULL)
            {
                if(Process != CsrProcess)
                {
                    HWINSTA hProcessWinSta = (HWINSTA)InterlockedCompareExchangePointer((PVOID)&Process->Win32WindowStation, (PVOID)hWinSta, NULL);
                    if(hProcessWinSta != NULL)
                    {
                        /* our process is already assigned to a different window station, we don't need the handle anymore */
                        NtClose(hWinSta);
                    }
                }
                else
                {
                    NtClose(hWinSta);
                }
            }

            if (hDesk != NULL)
            {
                PDESKTOP DesktopObject;
                Win32Thread->rpdesk = NULL;
                Status = ObReferenceObjectByHandle(hDesk,
                                                   0,
                                                   ExDesktopObjectType,
                                                   KernelMode,
                                                   (PVOID*)&DesktopObject,
                                                   NULL);
                NtClose(hDesk);
                if(NT_SUCCESS(Status))
                {
                    if (!IntSetThreadDesktop(DesktopObject,
                                             FALSE))
                    {
                        DPRINT1("Unable to set thread desktop\n");
                    }
                }
                else
                {
                    DPRINT1("Unable to reference thread desktop handle 0x%x\n", hDesk);
                }
            }
        }
        Win32Thread->TIF_flags &= ~TIF_INCLEANUP;
        co_IntDestroyCaret(Win32Thread);
        Win32Thread->ppi = PsGetCurrentProcessWin32Process();
        pTeb = NtCurrentTeb();
        if (pTeb)
        {
            Win32Thread->pClientInfo = (PCLIENTINFO)pTeb->Win32ClientInfo;
            Win32Thread->pClientInfo->pClientThreadInfo = NULL;
        }
        Win32Thread->MessageQueue = MsqCreateMessageQueue(Thread);
        Win32Thread->KeyboardLayout = W32kGetDefaultKeyLayout();
        Win32Thread->pEThread = Thread;
    }
    else
    {
        PSINGLE_LIST_ENTRY e;

        DPRINT("Destroying W32 thread TID:%d at IRQ level: %lu\n", Thread->Cid.UniqueThread, KeGetCurrentIrql());

        Win32Thread->TIF_flags |= TIF_INCLEANUP;
        DceFreeThreadDCE(Win32Thread);
        HOOK_DestroyThreadHooks(Thread);
        EVENT_DestroyThreadEvents(Thread);
        /* Cleanup timers */
        DestroyTimersForThread(Win32Thread);
        KeSetEvent(Win32Thread->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
        UnregisterThreadHotKeys(Thread);
        /* what if this co_ func crash in umode? what will clean us up then? */
        co_DestroyThreadWindows(Thread);
        IntBlockInput(Win32Thread, FALSE);
        MsqDestroyMessageQueue(Win32Thread->MessageQueue);
        IntCleanupThreadCallbacks(Win32Thread);

        /* cleanup user object references stack */
        e = PopEntryList(&Win32Thread->ReferencesList);
        while (e)
        {
            PUSER_REFERENCE_ENTRY ref = CONTAINING_RECORD(e, USER_REFERENCE_ENTRY, Entry);
            DPRINT("thread clean: remove reference obj 0x%x\n",ref->obj);
            UserDereferenceObject(ref->obj);

            e = PopEntryList(&Win32Thread->ReferencesList);
        }

        IntSetThreadDesktop(NULL,
                            TRUE);

        PsSetThreadWin32Thread(Thread, NULL);
    }

    RETURN( STATUS_SUCCESS);

CLEANUP:
    UserLeave();
    DPRINT("Leave Win32kThreadCallback, ret=%i\n",_ret_);
    END_CLEANUP;
}

NTSTATUS
Win32kInitWin32Thread(PETHREAD Thread)
{
    PEPROCESS Process;

    Process = Thread->ThreadsProcess;

    if (Process->Win32Process == NULL)
    {
        /* FIXME - lock the process */
        Process->Win32Process = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESSINFO), USERTAG_PROCESSINFO);

        if (Process->Win32Process == NULL)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(Process->Win32Process, sizeof(PROCESSINFO));
        /* FIXME - unlock the process */

        Win32kProcessCallback(Process, TRUE);
    }

    if (Thread->Tcb.Win32Thread == NULL)
    {
        Thread->Tcb.Win32Thread = ExAllocatePoolWithTag(NonPagedPool, sizeof(THREADINFO), USERTAG_THREADINFO);
        if (Thread->Tcb.Win32Thread == NULL)
            return STATUS_NO_MEMORY;

        RtlZeroMemory(Thread->Tcb.Win32Thread, sizeof(THREADINFO));

        Win32kThreadCallback(Thread, PsW32ThreadCalloutInitialize);
    }

    return(STATUS_SUCCESS);
}

C_ASSERT(sizeof(SERVERINFO) <= PAGE_SIZE);

// Return on failure
#define NT_ROF(x) \
    Status = (x); \
    if (!NT_SUCCESS(Status)) \
    { \
        DPRINT1("Failed '%s' (0x%lx)\n", #x, Status); \
        return Status; \
    }

/*
 * This definition doesn't work
 */
INIT_FUNCTION
NTSTATUS
APIENTRY
DriverEntry(
    IN	PDRIVER_OBJECT	DriverObject,
    IN	PUNICODE_STRING	RegistryPath)
{
    NTSTATUS Status;
    BOOLEAN Result;
    WIN32_CALLOUTS_FPNS CalloutData = {0};
    PVOID GlobalUserHeapBase = NULL;

    /*
     * Register user mode call interface
     * (system service table index = 1)
     */
    Result = KeAddSystemServiceTable(Win32kSSDT,
                                     NULL,
                                     Win32kNumberOfSysCalls,
                                     Win32kSSPT,
                                     1);
    if (Result == FALSE)
    {
        DPRINT1("Adding system services failed!\n");
        return STATUS_UNSUCCESSFUL;
    }

    hModuleWin = MmPageEntireDriver(DriverEntry);
    DPRINT("Win32k hInstance 0x%x!\n",hModuleWin);

    /* Register Object Manager Callbacks */
    CalloutData.WindowStationParseProcedure = IntWinStaObjectParse;
    CalloutData.WindowStationDeleteProcedure = IntWinStaObjectDelete;
    CalloutData.DesktopDeleteProcedure = IntDesktopObjectDelete;
    CalloutData.ProcessCallout = Win32kProcessCallback;
    CalloutData.ThreadCallout = Win32kThreadCallback;
    CalloutData.BatchFlushRoutine = NtGdiFlushUserBatch;

    /* Register our per-process and per-thread structures. */
    PsEstablishWin32Callouts((PWIN32_CALLOUTS_FPNS)&CalloutData);

    /* Create the global USER heap */
    GlobalUserHeap = UserCreateHeap(&GlobalUserHeapSection,
                                    &GlobalUserHeapBase,
                                    1 * 1024 * 1024); /* FIXME - 1 MB for now... */
    if (GlobalUserHeap == NULL)
    {
        DPRINT1("Failed to initialize the global heap!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Allocate global server info structure */
    gpsi = UserHeapAlloc(sizeof(SERVERINFO));
    if (!gpsi)
    {
        DPRINT1("Failed allocate server info structure!\n");
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory(gpsi, sizeof(SERVERINFO));
    DPRINT("Global Server Data -> %x\n", gpsi);

    NT_ROF(InitGdiHandleTable());
    NT_ROF(InitPaletteImpl());

    /* Create stock objects, ie. precreated objects commonly
       used by win32 applications */
    CreateStockObjects();
    CreateSysColorObjects();

    NT_ROF(InitXlateImpl());
    NT_ROF(InitPDEVImpl());
    NT_ROF(InitLDEVImpl());
    NT_ROF(InitDeviceImpl());
    NT_ROF(InitDcImpl());
    NT_ROF(InitUserImpl());
    NT_ROF(InitHotkeyImpl());
    NT_ROF(InitWindowStationImpl());
    NT_ROF(InitDesktopImpl());
    NT_ROF(InitWindowImpl());
    NT_ROF(InitMenuImpl());
    NT_ROF(InitInputImpl());
    NT_ROF(InitKeyboardImpl());
    NT_ROF(InitMonitorImpl());
    NT_ROF(MsqInitializeImpl());
    NT_ROF(InitTimerImpl());
    NT_ROF(InitAcceleratorImpl());
    NT_ROF(InitGuiCheckImpl());

    /* Initialize FreeType library */
    if (!InitFontSupport())
    {
        DPRINT1("Unable to initialize font support\n");
        return Status;
    }

    gusLanguageID = IntGdiGetLanguageID();

    return STATUS_SUCCESS;
}

/* EOF */
