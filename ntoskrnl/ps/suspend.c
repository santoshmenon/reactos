/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/suspend.c
 * PURPOSE:         Thread managment
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

ULONG
STDCALL
KeResumeThread(PKTHREAD Thread);

/* FUNCTIONS *****************************************************************/

/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtResumeThread(IN HANDLE ThreadHandle,
               IN PULONG SuspendCount  OPTIONAL)
{
    PETHREAD Thread;
    ULONG Prev;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
  
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();

    DPRINT("NtResumeThead(ThreadHandle %lx  SuspendCount %p)\n",
           ThreadHandle, SuspendCount);
    
    /* Check buffer validity */
    if(SuspendCount && PreviousMode == UserMode) {
        
        _SEH_TRY {
            
            ProbeForWrite(SuspendCount,
                          sizeof(ULONG),
                          sizeof(ULONG));
         } _SEH_HANDLE {
             
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        
        return Status;
    }
    
    /* Call the Kernel Function */
    Prev = KeResumeThread(&Thread->Tcb);
    
    /* Return it */        
    if(SuspendCount) {
            
        _SEH_TRY {
                
            *SuspendCount = Prev;
            
        } _SEH_HANDLE {
                
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;
    }

    /* Dereference and Return */
    ObDereferenceObject ((PVOID)Thread);
    return Status;
}

/*
 * FUNCTION: Increments a thread's suspend count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
 * REMARK:
 *        A thread will be suspended if its suspend count is greater than 0. 
 *        This procedure maps to the win32 SuspendThread function. ( 
 *        documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than 
 *        MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */
NTSTATUS 
STDCALL
NtSuspendThread(IN HANDLE ThreadHandle,
                IN PULONG PreviousSuspendCount  OPTIONAL)
{
    PETHREAD Thread;
    ULONG Prev;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
  
    PAGED_CODE();
    
    PreviousMode = ExGetPreviousMode();
    
    /* Check buffer validity */
    if(PreviousSuspendCount && PreviousMode == UserMode) {
        
        _SEH_TRY {
            
            ProbeForWrite(PreviousSuspendCount,
                          sizeof(ULONG),
                          sizeof(ULONG));
         } _SEH_HANDLE {
             
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        
        return Status;
    }
    
    /* Call the Kernel Function */
    Prev = KeSuspendThread(&Thread->Tcb);
    
    /* Return it */        
    if(PreviousSuspendCount) {
            
        _SEH_TRY {
                
            *PreviousSuspendCount = Prev;
            
        } _SEH_HANDLE {
                
            Status = _SEH_GetExceptionCode();
            
        } _SEH_END;
    }

    /* Dereference and Return */
    ObDereferenceObject((PVOID)Thread);
    return Status;
}

/* EOF */
