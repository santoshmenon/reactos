/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/factory.c
 * PURPOSE:         KS Allocator functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsReferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsDereferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
    UNIMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchQuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    ULONG Length;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    if (!CreateItem || !CreateItem->SecurityDescriptor)
    {
        /* no create item */
        Irp->IoStatus.Status = STATUS_NO_SECURITY_ON_OBJECT;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SECURITY_ON_OBJECT;
    }


    /* get input length */
    Length = IoStack->Parameters.QuerySecurity.Length;

    /* clone the security descriptor */
    Status = SeQuerySecurityDescriptorInfo(&IoStack->Parameters.QuerySecurity.SecurityInformation, (PSECURITY_DESCRIPTOR)Irp->UserBuffer, &Length, &CreateItem->SecurityDescriptor);

    DPRINT("SeQuerySecurityDescriptorInfo Status %x\n", Status);
    /* store result */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKSOBJECT_CREATE_ITEM CreateItem;
    PIO_STACK_LOCATION IoStack;
    PGENERIC_MAPPING Mapping;
    PSECURITY_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    if (!CreateItem || !CreateItem->SecurityDescriptor)
    {
        /* no create item */
        Irp->IoStatus.Status = STATUS_NO_SECURITY_ON_OBJECT;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SECURITY_ON_OBJECT;
    }

    /* backup old descriptor */
    Descriptor = CreateItem->SecurityDescriptor;

    /* get generic mapping */
    Mapping = IoGetFileObjectGenericMapping();

    /* change security descriptor */
    Status = SeSetSecurityDescriptorInfo(NULL, /*FIXME */
                                         &IoStack->Parameters.SetSecurity.SecurityInformation,
                                         IoStack->Parameters.SetSecurity.SecurityDescriptor,
                                         &CreateItem->SecurityDescriptor,
                                         NonPagedPool,
                                         Mapping);

    if (NT_SUCCESS(Status))
    {
        /* free old descriptor */
        ExFreePool(Descriptor);

       /* mark create item as changed */
       CreateItem->Flags |= KSCREATE_ITEM_SECURITYCHANGED;
    }

    /* store result */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificProperty(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchSpecificMethod(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsReadFile(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN  ULONG Length,
    IN  ULONG Key OPTIONAL,
    IN  KPROCESSOR_MODE RequestorMode)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsWriteFile(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PVOID Buffer,
    IN  ULONG Length,
    IN  ULONG Key OPTIONAL,
    IN  KPROCESSOR_MODE RequestorMode)
{
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    NTSTATUS Status;
    BOOLEAN Result;
    KEVENT LocalEvent;

    if (Event)
    {
        /* make sure event is reset */
        KeClearEvent(Event);
    }

    if (RequestorMode == UserMode)
    {
        /* probe the user buffer */
        _SEH2_TRY
        {
            ProbeForRead(Buffer, Length, sizeof(UCHAR));
            Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Exception, get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

         if (!NT_SUCCESS(Status))
         {
             DPRINT1("Invalid user buffer provided\n");
             return Status;
         }
    }

    /* get corresponding device object */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    /* fast-io write is only available for kernel mode clients */
    if (RequestorMode == KernelMode && ExGetPreviousMode() == KernelMode &&
        DeviceObject->DriverObject->FastIoDispatch->FastIoWrite)
    {
        /* call fast io write */
        Result = DeviceObject->DriverObject->FastIoDispatch->FastIoWrite(FileObject, &FileObject->CurrentByteOffset, Length, TRUE, Key, Buffer, IoStatusBlock, DeviceObject);

        if (Result && NT_SUCCESS(IoStatusBlock->Status))
        {
            /* request was handeled and succeeded */
            return STATUS_SUCCESS;
        }
    }

    /* do the slow way */

    if (!Event)
    {
        /* initialize temp event */
        KeInitializeEvent(&LocalEvent, NotificationEvent, FALSE);
        Event = &LocalEvent;
    }

    /* build the irp packet */
    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, DeviceObject, Buffer, Length, &FileObject->CurrentByteOffset, Event, IoStatusBlock);
    if (!Irp)
    {
        /* not enough resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* send the packet */
    Status = IoCallDriver(DeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        /* operation is pending, is sync file object */
        if (FileObject->Flags & FO_SYNCHRONOUS_IO)
        {
            /* it is so wait */
            KeWaitForSingleObject(Event, Executive, RequestorMode, FALSE, NULL);
            Status = IoStatusBlock->Status;
        }
    }
    /* return result */
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsQueryInformationFile(
    IN  PFILE_OBJECT FileObject,
    OUT PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsSetInformationFile(
    IN  PFILE_OBJECT FileObject,
    IN  PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamIo(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    IN  PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN  PVOID CompletionContext OPTIONAL,
    IN  KSCOMPLETION_INVOCATION CompletionInvocationFlags OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  OUT PVOID StreamHeaders,
    IN  ULONG Length,
    IN  ULONG Flags,
    IN  KPROCESSOR_MODE RequestorMode)
{
    PIRP Irp;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_OBJECT DeviceObject;
    ULONG Code;
    NTSTATUS Status;
    LARGE_INTEGER Offset;
    PKSIOBJECT_HEADER ObjectHeader;


    if (Flags == KSSTREAM_READ)
        Code = IRP_MJ_READ;
    else if (Flags == KSSTREAM_WRITE)
        Code = IRP_MJ_WRITE;
    else
        return STATUS_INVALID_PARAMETER;

    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!DeviceObject)
        return STATUS_INVALID_PARAMETER;

    if (Event)
    {
        KeResetEvent(Event);
    }

    //ASSERT(DeviceObject->DeviceType == FILE_DEVICE_KS);
    ObjectHeader = (PKSIOBJECT_HEADER)FileObject->FsContext;
    ASSERT(ObjectHeader);
    if (Code == IRP_MJ_READ)
    {
        if (ObjectHeader->DispatchTable.FastRead)
        {
            if (ObjectHeader->DispatchTable.FastRead(FileObject, NULL, Length, FALSE, 0, StreamHeaders, IoStatusBlock, DeviceObject))
            {
                return STATUS_SUCCESS;
            }
        }
    }
    else
    {
        if (ObjectHeader->DispatchTable.FastWrite)
        {
            if (ObjectHeader->DispatchTable.FastWrite(FileObject, NULL, Length, FALSE, 0, StreamHeaders, IoStatusBlock, DeviceObject))
            {
                return STATUS_SUCCESS;
            }
        }
    }

    Offset.QuadPart = 0LL;
    Irp = IoBuildSynchronousFsdRequest(Code, DeviceObject, (PVOID)StreamHeaders, Length, &Offset, Event, IoStatusBlock);
    if (!Irp)
    {
        return STATUS_UNSUCCESSFUL;
    }


    if (CompletionRoutine)
    {
        IoSetCompletionRoutine(Irp,
                               CompletionRoutine,
                               CompletionContext,
                               (CompletionInvocationFlags & KsInvokeOnSuccess),
                               (CompletionInvocationFlags & KsInvokeOnError),
                               (CompletionInvocationFlags & KsInvokeOnCancel));
    }

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->FileObject = FileObject;

    Status = IoCallDriver(DeviceObject, Irp);
    return Status;
}


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsProbeStreamIrp(
    IN  PIRP Irp,
    IN  ULONG ProbeFlags,
    IN  ULONG HeaderSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateExtraData(
    IN  PIRP Irp,
    IN  ULONG ExtraSize,
    OUT PVOID* ExtraBuffer)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsNullDriverUnload(
    IN  PDRIVER_OBJECT DriverObject)
{
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchInvalidDeviceRequest(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDefaultDeviceIoCompletion(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY && 
        IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_METHOD &&
        IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_RESET_STATE)
        {
            /* fake success */
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* request unsupported */
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }
    else
    {
        /* property / method / event not found */
        Status = STATUS_PROPSET_NOT_FOUND;
    }

    /* complete request */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);


    return Status;
}

/*
    @implemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsDispatchFastIoDeviceControlFailure(
    IN  PFILE_OBJECT FileObject,
    IN  BOOLEAN Wait,
    IN  PVOID InputBuffer  OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer  OPTIONAL,
    IN  ULONG OutputBufferLength,
    IN  ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}

/*
    @implemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsDispatchFastReadFailure(
    IN  PFILE_OBJECT FileObject,
    IN  PLARGE_INTEGER FileOffset,
    IN  ULONG Length,
    IN  BOOLEAN Wait,
    IN  ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}


/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsCancelIo(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsReleaseIrpOnCancelableQueue(
    IN  PIRP Irp,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
PIRP
NTAPI
KsRemoveIrpFromCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  KSIRP_REMOVAL_OPERATION RemovalOperation)
{
    PQUEUE_ENTRY Entry = NULL;
    PIRP Irp;
    KIRQL OldIrql;

    if (!QueueHead || !SpinLock)
        return NULL;

    if (ListLocation != KsListEntryTail && ListLocation != KsListEntryHead)
        return NULL;

    if (RemovalOperation != KsAcquireOnly && RemovalOperation != KsAcquireAndRemove)
        return NULL;

    KeAcquireSpinLock(SpinLock, &OldIrql);

    if (!IsListEmpty(QueueHead))
    {
        if (RemovalOperation == KsAcquireOnly)
        {
            if (ListLocation == KsListEntryHead)
                Entry = (PQUEUE_ENTRY)QueueHead->Flink;
            else
                Entry = (PQUEUE_ENTRY)QueueHead->Blink;
        }
        else if (RemovalOperation == KsAcquireAndRemove)
        {
            if (ListLocation == KsListEntryTail)
                Entry = (PQUEUE_ENTRY)RemoveTailList(QueueHead);
            else
                Entry = (PQUEUE_ENTRY)RemoveHeadList(QueueHead);
        }
    }
    KeReleaseSpinLock(SpinLock, OldIrql);

    if (!Entry)
        return NULL;

    Irp = Entry->Irp;

    if (RemovalOperation == KsAcquireAndRemove)
        ExFreePool(Entry);

    return Irp;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsMoveIrpsOnCancelableQueue(
    IN  OUT PLIST_ENTRY SourceList,
    IN  PKSPIN_LOCK SourceLock,
    IN  OUT PLIST_ENTRY DestinationList,
    IN  PKSPIN_LOCK DestinationLock OPTIONAL,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  PFNKSIRPLISTCALLBACK ListCallback,
    IN  PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsRemoveSpecificIrpFromCancelableQueue(
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
}


/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsAddIrpToCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  PIRP Irp,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsCancelRoutine(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGetChildCreateParameter(
    IN  PIRP Irp,
    OUT PVOID* CreateParameter)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTAPI
NTSTATUS
KspCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    ULONG Index;
    NTSTATUS Status;
    KIRQL OldLevel;
    ULONG Length;

    DPRINT("KS / CREATE\n");
    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* acquire list lock */
    KeAcquireSpinLock(&DeviceHeader->ItemListLock, &OldLevel);

    /* sanity check */
    ASSERT(IoStack->FileObject);

    if (IoStack->FileObject->FileName.Buffer == NULL && DeviceHeader->MaxItems == 1)
    {
        /* hack for bug 4566 */
        if (!DeviceHeader->ItemList[0].CreateItem || !DeviceHeader->ItemList[0].CreateItem->Create)
        {
            /* no valid create item */
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            /* release lock */
            KeReleaseSpinLock(&DeviceHeader->ItemListLock, OldLevel);
            /* return status */
            return STATUS_UNSUCCESSFUL;
        }

        /* set object create item */
        KSCREATE_ITEM_IRP_STORAGE(Irp) = DeviceHeader->ItemList[0].CreateItem;

        /* call create function */
        Status = DeviceHeader->ItemList[0].CreateItem->Create(DeviceObject, Irp);

        if (NT_SUCCESS(Status))
        {
            /* increment create item reference count */
            InterlockedIncrement((PLONG)&DeviceHeader->ItemList[0].ReferenceCount);
        }

        /* release lock */
        KeReleaseSpinLock(&DeviceHeader->ItemListLock, OldLevel);
        /* return result */
        return Status;
    }


    /* hack for bug 4566 */
    if (IoStack->FileObject->FileName.Buffer == NULL)
    {
        DPRINT("Using reference string hack\n");
        /* release lock */
        KeReleaseSpinLock(&DeviceHeader->ItemListLock, OldLevel);
        Irp->IoStatus.Information = 0;
        /* set return status */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    /* loop all device items */
    for(Index = 0; Index < DeviceHeader->MaxItems; Index++)
    {
        /* is there a create item */
        if (DeviceHeader->ItemList[Index].CreateItem == NULL)
            continue;

        /* check if the create item is initialized */
        if (!DeviceHeader->ItemList[Index].CreateItem->Create)
            continue;

        ASSERT(DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer);
        DPRINT("CreateItem %p Request %S\n", DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer,
                                              IoStack->FileObject->FileName.Buffer);

        /* get object class length */
        Length = wcslen(DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer);
        /* now check if the object class is the same */
        if (!_wcsnicmp(DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer, &IoStack->FileObject->FileName.Buffer[1], Length) ||
            (DeviceHeader->ItemList[Index].CreateItem->Flags & KSCREATE_ITEM_WILDCARD))
        {
            /* setup create parameters */
            DeviceHeader->DeviceIndex = Index;
             /* set object create item */
            KSCREATE_ITEM_IRP_STORAGE(Irp) = DeviceHeader->ItemList[Index].CreateItem;

            /* call create function */
            Status = DeviceHeader->ItemList[Index].CreateItem->Create(DeviceObject, Irp);

            if (NT_SUCCESS(Status))
            {
                /* increment create item reference count */
                InterlockedIncrement((PLONG)&DeviceHeader->ItemList[Index].ReferenceCount);
            }

            /* release lock */
            KeReleaseSpinLock(&DeviceHeader->ItemListLock, OldLevel);

            /* return result */
            return Status;
        }
    }

    /* release lock */
    KeReleaseSpinLock(&DeviceHeader->ItemListLock, OldLevel);

    Irp->IoStatus.Information = 0;
    /* set return status */
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTAPI
NTSTATUS
KspClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    NTSTATUS Status;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;


    DPRINT("KS / CLOSE\n");

    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        /* get object header */
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;
        /* store create item */
        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        /* call object close method */
        Status = ObjectHeader->DispatchTable.Close(DeviceObject, Irp);
        /* FIXME decrease reference count on original create item */

        /* return result */
        return Status;
    }
    else
    {
#if 0
        DPRINT1("Expected Object Header FileObject %p FsContext %p\n", IoStack->FileObject, IoStack->FileObject->FsContext);
        KeBugCheckEx(0, 0, 0, 0, 0);
#else
        DPRINT("Using reference string hack\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
#endif
        return STATUS_SUCCESS;
    }
}

NTAPI
NTSTATUS
KspDeviceControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSIDEVICE_HEADER DeviceHeader;
    PDEVICE_EXTENSION DeviceExtension;
    ULONG Length, Index;
    LPWSTR Buffer;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* hack for bug 4566 */
    if (IoStack->MajorFunction == IRP_MJ_DEVICE_CONTROL && IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_OBJECT_CLASS)
    {
        /* get device extension */
        DeviceExtension = (PDEVICE_EXTENSION)IoStack->DeviceObject->DeviceExtension;
        /* get device header */
        DeviceHeader = DeviceExtension->DeviceHeader;

        /* retrieve all available reference strings registered */
        Length = 0;

        for(Index = 0; Index < DeviceHeader->MaxItems; Index++)
        {
            if (!DeviceHeader->ItemList[Index].CreateItem || !DeviceHeader->ItemList[Index].CreateItem->Create || !DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer)
                continue;

            Length += wcslen(DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer) + 1;
        }

        /* add extra zero */
        Length += 1;

        /* allocate the buffer */
        Buffer = ExAllocatePool(NonPagedPool, Length * sizeof(WCHAR));
        if (!Buffer)
        {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        *((LPWSTR*)Irp->UserBuffer) = Buffer;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(LPWSTR);

        for(Index = 0; Index < DeviceHeader->MaxItems; Index++)
        {
            if (!DeviceHeader->ItemList[Index].CreateItem || !DeviceHeader->ItemList[Index].CreateItem->Create || !DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer)
                continue;

            wcscpy(Buffer, DeviceHeader->ItemList[Index].CreateItem->ObjectClass.Buffer);
            Buffer += wcslen(Buffer) + 1;
        }
        *Buffer = L'\0';
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    DPRINT("KS / DeviceControl\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;

        return ObjectHeader->DispatchTable.DeviceIoControl(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

NTAPI
NTSTATUS
KspRead(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / Read\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Read(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

NTAPI
NTSTATUS
KspWrite(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / Write\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Write(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header %p\n", IoStack->FileObject);
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

NTAPI
NTSTATUS
KspFlushBuffers(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / FlushBuffers\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.Flush(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

NTAPI
NTSTATUS
KspQuerySecurity(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / QuerySecurity\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.QuerySecurity(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

NTAPI
NTSTATUS
KspSetSecurity(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KS / SetSecurity\n");
    if (IoStack->FileObject && IoStack->FileObject->FsContext)
    {
        ObjectHeader = (PKSIOBJECT_HEADER) IoStack->FileObject->FsContext;

        KSCREATE_ITEM_IRP_STORAGE(Irp) = ObjectHeader->CreateItem;
        return ObjectHeader->DispatchTable.SetSecurity(DeviceObject, Irp);
    }
    else
    {
        DPRINT1("Expected Object Header\n");
        KeBugCheckEx(0, 0, 0, 0, 0);
        return STATUS_SUCCESS;
    }
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsSetMajorFunctionHandler(
    IN  PDRIVER_OBJECT DriverObject,
    IN  ULONG MajorFunction)
{
    switch ( MajorFunction )
    {
        case IRP_MJ_CREATE:
            DriverObject->MajorFunction[MajorFunction] = KspCreate;
            break;
        case IRP_MJ_CLOSE:
            DriverObject->MajorFunction[MajorFunction] = KspClose;
            break;
        case IRP_MJ_DEVICE_CONTROL:
            DriverObject->MajorFunction[MajorFunction] = KspDeviceControl;
            break;
        case IRP_MJ_READ:
            DriverObject->MajorFunction[MajorFunction] = KspRead;
            break;
        case IRP_MJ_WRITE:
            DriverObject->MajorFunction[MajorFunction] = KspWrite;
            break;
        case IRP_MJ_FLUSH_BUFFERS :
            DriverObject->MajorFunction[MajorFunction] = KspFlushBuffers;
            break;
        case IRP_MJ_QUERY_SECURITY:
            DriverObject->MajorFunction[MajorFunction] = KspQuerySecurity;
            break;
        case IRP_MJ_SET_SECURITY:
            DriverObject->MajorFunction[MajorFunction] = KspSetSecurity;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    };

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    //FIXME REWRITE

    switch (IoStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            return KspCreate(DeviceObject, Irp);
        case IRP_MJ_CLOSE:
            return KspClose(DeviceObject, Irp);
            break;
        case IRP_MJ_DEVICE_CONTROL:
            return KspDeviceControl(DeviceObject, Irp);
            break;
        case IRP_MJ_READ:
            return KspRead(DeviceObject, Irp);
            break;
        case IRP_MJ_WRITE:
            return KspWrite(DeviceObject, Irp);
            break;
        case IRP_MJ_FLUSH_BUFFERS:
            return KspFlushBuffers(DeviceObject, Irp);
            break;
        case IRP_MJ_QUERY_SECURITY:
            return KspQuerySecurity(DeviceObject, Irp);
            break;
        case IRP_MJ_SET_SECURITY:
            return KspSetSecurity(DeviceObject, Irp);
            break;
        default:
            return STATUS_INVALID_PARAMETER;    /* is this right? */
    };
}
