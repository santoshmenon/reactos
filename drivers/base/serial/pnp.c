/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/dd/serial/pnp.c
 * PURPOSE:         Serial IRP_MJ_PNP operations
 *
 * PROGRAMMERS:     Herv� Poussineau (hpoussin@reactos.org)
 */
/* FIXME: call IoAcquireRemoveLock/IoReleaseRemoveLock around each I/O operation */

#define INITGUID
#define NDEBUG
#include "serial.h"

NTSTATUS NTAPI
SerialAddDeviceInternal(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo,
	IN UART_TYPE UartType,
	IN PULONG pComPortNumber OPTIONAL,
	OUT PDEVICE_OBJECT* pFdo OPTIONAL)
{
	PDEVICE_OBJECT Fdo = NULL;
	PSERIAL_DEVICE_EXTENSION DeviceExtension = NULL;
	NTSTATUS Status;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	static ULONG DeviceNumber = 0;
	static ULONG ComPortNumber = 1;

	DPRINT("SerialAddDeviceInternal()\n");

	ASSERT(DriverObject);
	ASSERT(Pdo);

	/* Create new device object */
	swprintf(DeviceNameBuffer, L"\\Device\\Serial%lu", DeviceNumber);
	RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
	Status = IoCreateDevice(DriverObject,
	                        sizeof(SERIAL_DEVICE_EXTENSION),
	                        &DeviceName,
	                        FILE_DEVICE_SERIAL_PORT,
	                        FILE_DEVICE_SECURE_OPEN,
	                        FALSE,
	                        &Fdo);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateDevice() failed with status 0x%08x\n", Status);
		Fdo = NULL;
		goto ByeBye;
	}
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(SERIAL_DEVICE_EXTENSION));

	/* Register device interface */
	Status = IoRegisterDeviceInterface(Pdo, &GUID_DEVINTERFACE_COMPORT, NULL, &DeviceExtension->SerialInterfaceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoRegisterDeviceInterface() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}

	DeviceExtension->SerialPortNumber = DeviceNumber++;
	if (pComPortNumber == NULL)
		DeviceExtension->ComPort = ComPortNumber++;
	else
		DeviceExtension->ComPort = *pComPortNumber;
	DeviceExtension->Pdo = Pdo;
	DeviceExtension->PnpState = dsStopped;
	DeviceExtension->UartType = UartType;
	Status = InitializeCircularBuffer(&DeviceExtension->InputBuffer, 16);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	Status = InitializeCircularBuffer(&DeviceExtension->OutputBuffer, 16);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	IoInitializeRemoveLock(&DeviceExtension->RemoveLock, SERIAL_TAG, 0, 0);
	KeInitializeSpinLock(&DeviceExtension->InputBufferLock);
	KeInitializeSpinLock(&DeviceExtension->OutputBufferLock);
	KeInitializeEvent(&DeviceExtension->InputBufferNotEmpty, NotificationEvent, FALSE);
	KeInitializeDpc(&DeviceExtension->ReceivedByteDpc, SerialReceiveByte, DeviceExtension);
	KeInitializeDpc(&DeviceExtension->SendByteDpc, SerialSendByte, DeviceExtension);
	KeInitializeDpc(&DeviceExtension->CompleteIrpDpc, SerialCompleteIrp, DeviceExtension);
	Fdo->Flags |= DO_POWER_PAGABLE;
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08x\n", Status);
		goto ByeBye;
	}
	Fdo->Flags |= DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	if (pFdo)
	{
		*pFdo = Fdo;
	}

	return STATUS_SUCCESS;

ByeBye:
	if (Fdo)
	{
		FreeCircularBuffer(&DeviceExtension->InputBuffer);
		FreeCircularBuffer(&DeviceExtension->OutputBuffer);
		IoDeleteDevice(Fdo);
	}
	return Status;
}

NTSTATUS NTAPI
SerialAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	/* Serial.sys is a legacy driver. AddDevice is called once
	 * with a NULL Pdo just after the driver initialization.
	 * Detect this case and return success.
	 */
	if (Pdo == NULL)
		return STATUS_SUCCESS;

	/* We have here a PDO not null. It represents a real serial
	 * port. So call the internal AddDevice function.
	 */
	return SerialAddDeviceInternal(DriverObject, Pdo, UartUnknown, NULL, NULL);
}

NTSTATUS NTAPI
SerialPnpStartDevice(
	IN PDEVICE_OBJECT DeviceObject,
	IN PCM_RESOURCE_LIST ResourceList,
	IN PCM_RESOURCE_LIST ResourceListTranslated)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	WCHAR DeviceNameBuffer[32];
	UNICODE_STRING DeviceName;
	WCHAR LinkNameBuffer[32];
	UNICODE_STRING LinkName;
	WCHAR ComPortBuffer[32];
	UNICODE_STRING ComPort;
	ULONG Vector = 0;
	ULONG i, j;
	UCHAR IER;
	KIRQL Dirql;
	KAFFINITY Affinity = 0;
	KINTERRUPT_MODE InterruptMode = Latched;
	BOOLEAN ShareInterrupt = TRUE;
	OBJECT_ATTRIBUTES objectAttributes;
	PUCHAR ComPortBase;
	UNICODE_STRING KeyName;
	HANDLE hKey;
	NTSTATUS Status;

	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(ResourceList);
	ASSERT(DeviceExtension);
	ASSERT(DeviceExtension->PnpState == dsStopped);

	DeviceExtension->BaudRate = 19200;
	DeviceExtension->BaseAddress = 0;
	Dirql = 0;
	for (i = 0; i < ResourceList->Count; i++)
	{
		for (j = 0; j < ResourceList->List[i].PartialResourceList.Count; j++)
		{
			PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor = &ResourceList->List[i].PartialResourceList.PartialDescriptors[j];
			PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptorTranslated = &ResourceListTranslated->List[i].PartialResourceList.PartialDescriptors[j];
			switch (PartialDescriptor->Type)
			{
				case CmResourceTypePort:
					if (PartialDescriptor->u.Port.Length < 8)
						return STATUS_INSUFFICIENT_RESOURCES;
					if (DeviceExtension->BaseAddress != 0)
						return STATUS_UNSUCCESSFUL;
					DeviceExtension->BaseAddress = PartialDescriptor->u.Port.Start.u.LowPart;
					break;
				case CmResourceTypeInterrupt:
					Dirql = (KIRQL)PartialDescriptorTranslated->u.Interrupt.Level;
					Vector = PartialDescriptorTranslated->u.Interrupt.Vector;
					Affinity = PartialDescriptorTranslated->u.Interrupt.Affinity;
					if (PartialDescriptorTranslated->Flags & CM_RESOURCE_INTERRUPT_LATCHED)
						InterruptMode = Latched;
					else
						InterruptMode = LevelSensitive;
					ShareInterrupt = (PartialDescriptorTranslated->ShareDisposition == CmResourceShareShared);
					break;
			}
		}
	}
	DPRINT("New COM port. Base = 0x%lx, Irql = %u\n",
		DeviceExtension->BaseAddress, Dirql);
	if (!DeviceExtension->BaseAddress)
		return STATUS_INSUFFICIENT_RESOURCES;
	if (!Dirql)
		return STATUS_INSUFFICIENT_RESOURCES;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;

	/* Test if we are trying to start the serial port used for debugging */
	if (KdComPortInUse && *KdComPortInUse == ULongToPtr(DeviceExtension->BaseAddress))
	{
		DPRINT("Failing IRP_MN_START_DEVICE as this serial port is used for debugging\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	if (DeviceExtension->UartType == UartUnknown)
		DeviceExtension->UartType = SerialDetectUartType(ComPortBase);

	/* Get current settings */
	DeviceExtension->MCR = READ_PORT_UCHAR(SER_MCR(ComPortBase));
	DeviceExtension->MSR = READ_PORT_UCHAR(SER_MSR(ComPortBase));
	DeviceExtension->WaitMask = 0;

	/* Set baud rate */
	Status = SerialSetBaudRate(DeviceExtension, DeviceExtension->BaudRate);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("SerialSetBaudRate() failed with status 0x%08x\n", Status);
		return Status;
	}

	/* Set line control */
	DeviceExtension->SerialLineControl.StopBits = STOP_BIT_1;
	DeviceExtension->SerialLineControl.Parity = NO_PARITY;
	DeviceExtension->SerialLineControl.WordLength = 8;
	Status = SerialSetLineControl(DeviceExtension, &DeviceExtension->SerialLineControl);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("SerialSetLineControl() failed with status 0x%08x\n", Status);
		return Status;
	}

	/* Clear receive/transmit buffers */
	if (DeviceExtension->UartType >= Uart16550A)
	{
		/* 16550 UARTs also have FIFO queues, but they are unusable due to a bug */
		WRITE_PORT_UCHAR(SER_FCR(ComPortBase),
			SR_FCR_CLEAR_RCVR | SR_FCR_CLEAR_XMIT);
	}

	/* Create link \DosDevices\COMX -> \Device\SerialX */
	swprintf(DeviceNameBuffer, L"\\Device\\Serial%lu", DeviceExtension->SerialPortNumber);
	swprintf(LinkNameBuffer, L"\\DosDevices\\COM%lu", DeviceExtension->ComPort);
	swprintf(ComPortBuffer, L"COM%lu", DeviceExtension->ComPort);
	RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);
	RtlInitUnicodeString(&LinkName, LinkNameBuffer);
	RtlInitUnicodeString(&ComPort, ComPortBuffer);
	Status = IoCreateSymbolicLink(&LinkName, &DeviceName);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoCreateSymbolicLink() failed with status 0x%08x\n", Status);
		return Status;
	}

	/* Connect interrupt and enable them */
	Status = IoConnectInterrupt(
		&DeviceExtension->Interrupt, SerialInterruptService,
		DeviceObject, NULL,
		Vector, Dirql, Dirql,
		InterruptMode, ShareInterrupt,
		Affinity, FALSE);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("IoConnectInterrupt() failed with status 0x%08x\n", Status);
		IoSetDeviceInterfaceState(&DeviceExtension->SerialInterfaceName, FALSE);
		IoDeleteSymbolicLink(&LinkName);
		return Status;
	}

	/* Write an entry value under HKLM\HARDWARE\DeviceMap\SERIALCOMM */
	/* This step is not mandatory, so don't exit in case of error */
	RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\HARDWARE\\DeviceMap\\SERIALCOMM");
	InitializeObjectAttributes(&objectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateKey(&hKey, KEY_SET_VALUE, &objectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
	if (NT_SUCCESS(Status))
	{
		/* Key = \Device\Serialx, Value = COMx */
		ZwSetValueKey(hKey, &DeviceName, 0, REG_SZ, ComPortBuffer, ComPort.Length + sizeof(WCHAR));
		ZwClose(hKey);
	}

	DeviceExtension->PnpState = dsStarted;

	/* Activate interrupt modes */
	IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
	IER |= SR_IER_DATA_RECEIVED | SR_IER_THR_EMPTY | SR_IER_LSR_CHANGE | SR_IER_MSR_CHANGE;
	WRITE_PORT_UCHAR(SER_IER(ComPortBase), IER);

	/* Activate DTR, RTS */
	DeviceExtension->MCR |= SR_MCR_DTR | SR_MCR_RTS;
	WRITE_PORT_UCHAR(SER_MCR(ComPortBase), DeviceExtension->MCR);

	/* Activate serial interface */
	IoSetDeviceInterfaceState(&DeviceExtension->SerialInterfaceName, TRUE);
	/* We don't really care if the call succeeded or not... */

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI
SerialPnp(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG MinorFunction;
	PIO_STACK_LOCATION Stack;
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	MinorFunction = Stack->MinorFunction;

	switch (MinorFunction)
	{
		/* FIXME: do all these minor functions
		IRP_MN_QUERY_REMOVE_DEVICE 0x1
		IRP_MN_REMOVE_DEVICE 0x2
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_REMOVE_DEVICE\n");
			IoAcquireRemoveLock
			IoReleaseRemoveLockAndWait
			pass request to DeviceExtension-LowerDriver
			disable interface
			IoDeleteDevice(Fdo) and/or IoDetachDevice
			break;
		}
		IRP_MN_CANCEL_REMOVE_DEVICE 0x3
		IRP_MN_STOP_DEVICE 0x4
		IRP_MN_QUERY_STOP_DEVICE 0x5
		IRP_MN_CANCEL_STOP_DEVICE 0x6
		IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations (optional) 0x7
		IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations (optional) 0x7
		IRP_MN_QUERY_INTERFACE (optional) 0x8
		IRP_MN_QUERY_CAPABILITIES (optional) 0x9
		IRP_MN_FILTER_RESOURCE_REQUIREMENTS (optional) 0xd
		IRP_MN_QUERY_PNP_DEVICE_STATE (optional) 0x14
		IRP_MN_DEVICE_USAGE_NOTIFICATION (required or optional) 0x16
		IRP_MN_SURPRISE_REMOVAL 0x17
		*/
		case IRP_MN_START_DEVICE: /* 0x0 */
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");

			ASSERT(((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->PnpState == dsStopped);

			/* FIXME: HACK: verify that we have some allocated resources.
			 * It seems not to be always the case on some hardware
			 */
			if (Stack->Parameters.StartDevice.AllocatedResources == NULL)
			{
				DPRINT1("No allocated resources. Can't start COM%lu\n",
					((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->ComPort);
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			/* Call lower driver */
			Status = ForwardIrpAndWait(DeviceObject, Irp);
			if (NT_SUCCESS(Status))
				Status = SerialPnpStartDevice(
					DeviceObject,
					Stack->Parameters.StartDevice.AllocatedResources,
					Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
			break;
		}
		case IRP_MN_QUERY_DEVICE_RELATIONS: /* (optional) 0x7 */
		{
			switch (Stack->Parameters.QueryDeviceRelations.Type)
			{
				case BusRelations:
				{
					DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				case RemovalRelations:
				{
					DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
					return ForwardIrpAndForget(DeviceObject, Irp);
				}
				default:
					DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
						Stack->Parameters.QueryDeviceRelations.Type);
					return ForwardIrpAndForget(DeviceObject, Irp);
			}
			break;
		}
		default:
		{
			DPRINT1("Unknown minor function 0x%x\n", MinorFunction);
			return ForwardIrpAndForget(DeviceObject, Irp);
		}
	}

	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
