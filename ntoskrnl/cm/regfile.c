/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/regfile.c
 * PURPOSE:         Registry file manipulation routines
 *
 * PROGRAMMERS:     Casper Hornstrup
 *                  Eric Kohl
 *                  Filip Navara
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

/* uncomment to enable hive checks (incomplete and probably buggy) */
//#define HIVE_CHECK

/* LOCAL MACROS *************************************************************/

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))

/* FUNCTIONS ****************************************************************/

PVOID CMAPI
CmpAllocate(
   ULONG Size,
   BOOLEAN Paged)
{
   return ExAllocatePoolWithTag(Paged ? PagedPool : NonPagedPool,
                                Size, TAG('R','E','G',' '));
}

VOID CMAPI
CmpFree(
   PVOID Ptr)
{
   ExFreePool(Ptr);
}


BOOLEAN CMAPI
CmpFileRead(
   PHHIVE RegistryHive,
   ULONG FileType,
   ULONG FileOffset,
   PVOID Buffer,
   ULONG BufferLength)
{
   PEREGISTRY_HIVE CmHive = (PEREGISTRY_HIVE)RegistryHive;
   HANDLE HiveHandle = FileType == HV_TYPE_PRIMARY ? CmHive->HiveHandle : CmHive->LogHandle;
   LARGE_INTEGER _FileOffset;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   _FileOffset.QuadPart = FileOffset;
   Status = ZwReadFile(HiveHandle, 0, 0, 0, &IoStatusBlock,
                       Buffer, BufferLength, &_FileOffset, 0);
   return NT_SUCCESS(Status) ? TRUE : FALSE;
}


BOOLEAN CMAPI
CmpFileWrite(
   PHHIVE RegistryHive,
   ULONG FileType,
   ULONG FileOffset,
   PVOID Buffer,
   ULONG BufferLength)
{
   PEREGISTRY_HIVE CmHive = (PEREGISTRY_HIVE)RegistryHive;
   HANDLE HiveHandle = FileType == HV_TYPE_PRIMARY ? CmHive->HiveHandle : CmHive->LogHandle;
   LARGE_INTEGER _FileOffset;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   _FileOffset.QuadPart = FileOffset;
   Status = ZwWriteFile(HiveHandle, 0, 0, 0, &IoStatusBlock,
                       Buffer, BufferLength, &_FileOffset, 0);
   return NT_SUCCESS(Status) ? TRUE : FALSE;
}


BOOLEAN CMAPI
CmpFileSetSize(
   PHHIVE RegistryHive,
   ULONG FileType,
   ULONG FileSize)
{
   PEREGISTRY_HIVE CmHive = (PEREGISTRY_HIVE)RegistryHive;
   HANDLE HiveHandle = FileType == HV_TYPE_PRIMARY ? CmHive->HiveHandle : CmHive->LogHandle;
   FILE_END_OF_FILE_INFORMATION EndOfFileInfo;
   FILE_ALLOCATION_INFORMATION FileAllocationInfo;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   EndOfFileInfo.EndOfFile.QuadPart = FileSize;
   Status = ZwSetInformationFile(HiveHandle, &IoStatusBlock,
                                 &EndOfFileInfo,
                                 sizeof(FILE_END_OF_FILE_INFORMATION),
                                 FileEndOfFileInformation);
   if (!NT_SUCCESS(Status))
      return FALSE;

   FileAllocationInfo.AllocationSize.QuadPart = FileSize;
   Status = ZwSetInformationFile(HiveHandle, &IoStatusBlock,
                                 &FileAllocationInfo,
                                 sizeof(FILE_ALLOCATION_INFORMATION),
                                 FileAllocationInformation);
   if (!NT_SUCCESS(Status))
      return FALSE;

   return TRUE;
}


BOOLEAN CMAPI
CmpFileFlush(
   PHHIVE RegistryHive,
   ULONG FileType)
{
   PEREGISTRY_HIVE CmHive = (PEREGISTRY_HIVE)RegistryHive;
   HANDLE HiveHandle = FileType == HV_TYPE_PRIMARY ? CmHive->HiveHandle : CmHive->LogHandle;
   IO_STATUS_BLOCK IoStatusBlock;
   NTSTATUS Status;

   Status = ZwFlushBuffersFile(HiveHandle, &IoStatusBlock);
   return NT_SUCCESS(Status) ? TRUE : FALSE;
}


static NTSTATUS
CmiCreateNewRegFile(HANDLE FileHandle)
{
  PEREGISTRY_HIVE CmHive;
  PHHIVE Hive;
  NTSTATUS Status;

  CmHive = CmpAllocate(sizeof(EREGISTRY_HIVE), TRUE);
  CmHive->HiveHandle = FileHandle;
  Status = HvInitialize(&CmHive->Hive, HV_OPERATION_CREATE_HIVE, 0, 0,
                        CmpAllocate, CmpFree,
                        CmpFileRead, CmpFileWrite, CmpFileSetSize,
                        CmpFileFlush, NULL);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }

  /* Init root key cell */
  Hive = &CmHive->Hive;
  if (!CmCreateRootNode(Hive, L""))
    {
      HvFree (Hive);
      return FALSE;
    }

  Status = HvWriteHive(Hive) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;

  HvFree (Hive);

  return(Status);
}


#ifdef HIVE_CHECK

static ULONG
CmiCalcChecksum(PULONG Buffer)
{
  ULONG Sum = 0;
  ULONG i;

  for (i = 0; i < 127; i++)
    Sum ^= Buffer[i];
  if (Sum == (ULONG)-1)
    Sum = (ULONG)-2;
  if (Sum == 0)
    Sum = 1;

  return(Sum);
}

static NTSTATUS
CmiCheckAndFixHive(PEREGISTRY_HIVE RegistryHive)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  FILE_STANDARD_INFORMATION fsi;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE HiveHandle = INVALID_HANDLE_VALUE;
  HANDLE LogHandle = INVALID_HANDLE_VALUE;
  PHBASE_BLOCK HiveHeader = NULL;
  PHBASE_BLOCK LogHeader = NULL;
  LARGE_INTEGER FileOffset;
  ULONG FileSize;
  ULONG BufferSize;
  ULONG BitmapSize;
  RTL_BITMAP BlockBitMap;
  NTSTATUS Status;

  DPRINT("CmiCheckAndFixHive() called\n");

  /* Try to open the hive file */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = ZwCreateFile(&HiveHandle,
			FILE_READ_DATA | FILE_READ_ATTRIBUTES,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
      return(STATUS_SUCCESS);
    }
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Try to open the log file */
  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->LogFileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = ZwCreateFile(&LogHandle,
			FILE_READ_DATA | FILE_READ_ATTRIBUTES,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
      LogHandle = INVALID_HANDLE_VALUE;
    }
  else if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwCreateFile() failed (Status %lx)\n", Status);
      ZwClose(HiveHandle);
      return(Status);
    }

  /* Allocate hive header */
  HiveHeader = ExAllocatePool(PagedPool,
			      sizeof(HBASE_BLOCK));
  if (HiveHeader == NULL)
    {
      DPRINT("ExAllocatePool() failed\n");
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }

  /* Read hive base block */
  FileOffset.QuadPart = 0ULL;
  Status = ZwReadFile(HiveHandle,
		      0,
		      0,
		      0,
		      &IoStatusBlock,
		      HiveHeader,
		      sizeof(HBASE_BLOCK),
		      &FileOffset,
		      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwReadFile() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  if (LogHandle == INVALID_HANDLE_VALUE)
    {
      if (HiveHeader->Checksum != CmiCalcChecksum((PULONG)HiveHeader) ||
	  HiveHeader->Sequence1 != HiveHeader->Sequence2)
	{
	  /* There is no way to fix the hive without log file - BSOD! */
	  DPRINT("Hive header inconsistent and no log file available!\n");
	  KEBUGCHECK(CONFIG_LIST_FAILED);
	}

      Status = STATUS_SUCCESS;
      goto ByeBye;
    }
  else
    {
      /* Allocate hive header */
      LogHeader = ExAllocatePool(PagedPool,
				 HV_LOG_HEADER_SIZE);
      if (LogHeader == NULL)
	{
	  DPRINT("ExAllocatePool() failed\n");
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	  goto ByeBye;
	}

      /* Read log file header */
      FileOffset.QuadPart = 0ULL;
      Status = ZwReadFile(LogHandle,
			  0,
			  0,
			  0,
			  &IoStatusBlock,
			  LogHeader,
			  HV_LOG_HEADER_SIZE,
			  &FileOffset,
			  0);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwReadFile() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* Check log file header integrity */
      if (LogHeader->Checksum != CmiCalcChecksum((PULONG)LogHeader) ||
          LogHeader->Sequence1 != LogHeader->Sequence2)
	{
	  if (HiveHeader->Checksum != CmiCalcChecksum((PULONG)HiveHeader) ||
	      HiveHeader->Sequence1 != HiveHeader->Sequence2)
	    {
	      DPRINT("Hive file and log file are inconsistent!\n");
	      KEBUGCHECK(CONFIG_LIST_FAILED);
	    }

	  /* Log file damaged but hive is okay */
	  Status = STATUS_SUCCESS;
	  goto ByeBye;
	}

      if (HiveHeader->Sequence1 == HiveHeader->Sequence2 &&
	  HiveHeader->Sequence1 == LogHeader->Sequence1)
	{
	  /* Hive and log file are up-to-date */
	  Status = STATUS_SUCCESS;
	  goto ByeBye;
	}

      /*
       * Hive needs an update!
       */

      /* Get file size */
      Status = ZwQueryInformationFile(LogHandle,
				      &IoStatusBlock,
				      &fsi,
				      sizeof(fsi),
				      FileStandardInformation);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwQueryInformationFile() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}
      FileSize = fsi.EndOfFile.u.LowPart;

      /* Calculate bitmap and block size */
      BitmapSize = ROUND_UP((FileSize / HV_BLOCK_SIZE) - 1, sizeof(ULONG) * 8) / 8;
      BufferSize = HV_LOG_HEADER_SIZE + sizeof(ULONG) + BitmapSize;
      BufferSize = ROUND_UP(BufferSize, HV_BLOCK_SIZE);

      /* Reallocate log header block */
      ExFreePool(LogHeader);
      LogHeader = ExAllocatePool(PagedPool,
				 BufferSize);
      if (LogHeader == NULL)
	{
	  DPRINT("ExAllocatePool() failed\n");
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	  goto ByeBye;
	}

      /* Read log file header */
      FileOffset.QuadPart = 0ULL;
      Status = ZwReadFile(LogHandle,
			  0,
			  0,
			  0,
			  &IoStatusBlock,
			  LogHeader,
			  BufferSize,
			  &FileOffset,
			  0);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwReadFile() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* Initialize bitmap */
      RtlInitializeBitMap(&BlockBitMap,
			  (PVOID)((ULONG_PTR)LogHeader + HV_BLOCK_SIZE + sizeof(ULONG)),
			  BitmapSize * 8);

      /* FIXME: Update dirty blocks */


      /* FIXME: Update hive header */


      Status = STATUS_SUCCESS;
    }


  /* Clean up the mess */
ByeBye:
  if (HiveHeader != NULL)
    ExFreePool(HiveHeader);

  if (LogHeader != NULL)
    ExFreePool(LogHeader);

  if (LogHandle != INVALID_HANDLE_VALUE)
    ZwClose(LogHandle);

  ZwClose(HiveHandle);

  return(Status);
}
#endif

static NTSTATUS
CmiInitNonVolatileRegistryHive (PEREGISTRY_HIVE RegistryHive,
				PWSTR Filename)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  ULONG CreateDisposition;
  IO_STATUS_BLOCK IoSB;
  HANDLE FileHandle;
  PVOID SectionObject;
  PUCHAR ViewBase;
  ULONG ViewSize;
  NTSTATUS Status;

  DPRINT("CmiInitNonVolatileRegistryHive(%p, %S) called\n",
	 RegistryHive, Filename);

  /* Duplicate Filename */
  Status = RtlCreateUnicodeString(&RegistryHive->HiveFileName,
                                  Filename);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlCreateUnicodeString() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create log file name */
  RegistryHive->LogFileName.Length = (wcslen(Filename) + 4) * sizeof(WCHAR);
  RegistryHive->LogFileName.MaximumLength = RegistryHive->LogFileName.Length + sizeof(WCHAR);
  RegistryHive->LogFileName.Buffer = ExAllocatePoolWithTag(PagedPool,
						           RegistryHive->LogFileName.MaximumLength,
                                                           TAG('U', 'S', 'T', 'R'));
  if (RegistryHive->LogFileName.Buffer == NULL)
    {
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      DPRINT("ExAllocatePool() failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  wcscpy(RegistryHive->LogFileName.Buffer,
	 Filename);
  wcscat(RegistryHive->LogFileName.Buffer,
	 L".log");

#ifdef HIVE_CHECK
  /* Check and eventually fix a hive */
  Status = CmiCheckAndFixHive(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      DPRINT1("CmiCheckAndFixHive() failed (Status %lx)\n", Status);
      return(Status);
    }
#endif

  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  CreateDisposition = FILE_OPEN_IF;
  Status = ZwCreateFile(&FileHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoSB,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			CreateDisposition,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      DPRINT("ZwCreateFile() failed (Status %lx)\n", Status);
      return(Status);
    }

  if (IoSB.Information != FILE_OPENED)
    {
      Status = CmiCreateNewRegFile(FileHandle);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("CmiCreateNewRegFile() failed (Status %lx)\n", Status);
	  ZwClose(FileHandle);
	  RtlFreeUnicodeString(&RegistryHive->HiveFileName);
	  RtlFreeUnicodeString(&RegistryHive->LogFileName);
	  return(Status);
	}
    }

  /* Create the hive section */
  Status = MmCreateSection(&SectionObject,
			   SECTION_ALL_ACCESS,
			   NULL,
			   NULL,
			   PAGE_READWRITE,
			   SEC_COMMIT,
			   FileHandle,
			   NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmCreateSection() failed (Status %lx)\n", Status);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      return(Status);
    }

  /* Map the hive file */
  ViewBase = NULL;
  ViewSize = 0;
  Status = MmMapViewOfSection(SectionObject,
			      PsGetCurrentProcess(),
			      (PVOID*)&ViewBase,
			      0,
			      ViewSize,
			      NULL,
			      &ViewSize,
			      0,
			      MEM_COMMIT,
			      PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmMapViewOfSection() failed (Status %lx)\n", Status);
      ObDereferenceObject(SectionObject);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      ZwClose(FileHandle);
      return(Status);
    }
  DPRINT("ViewBase %p  ViewSize %lx\n", ViewBase, ViewSize);

  Status = HvInitialize(&RegistryHive->Hive, HV_OPERATION_MEMORY,
                        (ULONG_PTR)ViewBase, ViewSize,
                        CmpAllocate, CmpFree,
                        CmpFileRead, CmpFileWrite, CmpFileSetSize,
                        CmpFileFlush, NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to open hive\n");
      MmUnmapViewOfSection(PsGetCurrentProcess(),
			   ViewBase);
      ObDereferenceObject(SectionObject);
      RtlFreeUnicodeString(&RegistryHive->HiveFileName);
      RtlFreeUnicodeString(&RegistryHive->LogFileName);
      ZwClose(FileHandle);
      return Status;
    }

  CmPrepareHive(&RegistryHive->Hive);

  /* Unmap and dereference the hive section */
  MmUnmapViewOfSection(PsGetCurrentProcess(),
                       ViewBase);
  ObDereferenceObject(SectionObject);

  /* Close the hive file */
  ZwClose(FileHandle);

  DPRINT("CmiInitNonVolatileRegistryHive(%p, %S) - Finished.\n",
	 RegistryHive, Filename);

  return STATUS_SUCCESS;
}


NTSTATUS
CmiCreateTempHive(PEREGISTRY_HIVE *RegistryHive)
{
  PEREGISTRY_HIVE Hive;
  NTSTATUS Status;

  *RegistryHive = NULL;

  Hive = ExAllocatePool (NonPagedPool,
			 sizeof(EREGISTRY_HIVE));
  if (Hive == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory (Hive,
		 sizeof(EREGISTRY_HIVE));

  DPRINT("Hive 0x%p\n", Hive);

  Status = HvInitialize(&Hive->Hive, HV_OPERATION_CREATE_HIVE, 0, 0,
                        CmpAllocate, CmpFree,
                        CmpFileRead, CmpFileWrite, CmpFileSetSize,
                        CmpFileFlush, NULL);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool (Hive);
      return Status;
    }

  if (!CmCreateRootNode (&Hive->Hive, L""))
    {
      HvFree (&Hive->Hive);
      ExFreePool (Hive);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Hive->Flags = HIVE_NO_FILE;

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite (&CmiRegistryLock, TRUE);

  /* Add the new hive to the hive list */
  InsertTailList (&CmiHiveListHead,
		  &Hive->HiveList);

  /* Release hive list lock */
  ExReleaseResourceLite (&CmiRegistryLock);
  KeLeaveCriticalRegion();

  VERIFY_REGISTRY_HIVE (Hive);

  *RegistryHive = Hive;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiCreateVolatileHive(PEREGISTRY_HIVE *RegistryHive)
{
  DPRINT ("CmiCreateVolatileHive() called\n");
  return CmiCreateTempHive(RegistryHive);
}


NTSTATUS
CmiLoadHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
	    IN PUNICODE_STRING FileName,
	    IN ULONG Flags)
{
  PEREGISTRY_HIVE Hive;
  NTSTATUS Status;

  DPRINT ("CmiLoadHive(Filename %wZ)\n", FileName);

  if (Flags & ~REG_NO_LAZY_FLUSH)
    return STATUS_INVALID_PARAMETER;

  Hive = ExAllocatePool (NonPagedPool,
			 sizeof(EREGISTRY_HIVE));
  if (Hive == NULL)
    {
      DPRINT1 ("Failed to allocate hive header.\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }
  RtlZeroMemory (Hive,
		 sizeof(EREGISTRY_HIVE));

  DPRINT ("Hive 0x%p\n", Hive);
  Hive->Flags = (Flags & REG_NO_LAZY_FLUSH) ? HIVE_NO_SYNCH : 0;

  Status = CmiInitNonVolatileRegistryHive (Hive,
					   FileName->Buffer);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1 ("CmiInitNonVolatileRegistryHive() failed (Status %lx)\n", Status);
      ExFreePool (Hive);
      return Status;
    }

  /* Add the new hive to the hive list */
  InsertTailList (&CmiHiveListHead,
		  &Hive->HiveList);

  VERIFY_REGISTRY_HIVE(Hive);

  Status = CmiConnectHive (KeyObjectAttributes,
			   Hive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiConnectHive() failed (Status %lx)\n", Status);
//      CmiRemoveRegistryHive (Hive);
    }

  DPRINT ("CmiLoadHive() done\n");

  return Status;
}


NTSTATUS
CmiRemoveRegistryHive(PEREGISTRY_HIVE RegistryHive)
{
  /* Remove hive from hive list */
  RemoveEntryList (&RegistryHive->HiveList);

  /* Release file names */
  RtlFreeUnicodeString (&RegistryHive->HiveFileName);
  RtlFreeUnicodeString (&RegistryHive->LogFileName);

  /* Release hive */
  HvFree (&RegistryHive->Hive);

  return STATUS_SUCCESS;
}


NTSTATUS
CmOpenHiveFiles(PEREGISTRY_HIVE RegistryHive)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->HiveFileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = ZwCreateFile(&RegistryHive->HiveHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     &RegistryHive->LogFileName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = ZwCreateFile(&RegistryHive->LogHandle,
			FILE_ALL_ACCESS,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_SUPERSEDE,
			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(RegistryHive->HiveHandle);
      return(Status);
    }

  return STATUS_SUCCESS;
}


VOID
CmCloseHiveFiles(PEREGISTRY_HIVE RegistryHive)
{
  ZwClose(RegistryHive->HiveHandle);
  ZwClose(RegistryHive->LogHandle);
}


NTSTATUS
CmiFlushRegistryHive(PEREGISTRY_HIVE RegistryHive)
{
  BOOLEAN Success;
  NTSTATUS Status;

  ASSERT(!IsNoFileHive(RegistryHive));

  if (RtlFindSetBits(&RegistryHive->Hive.DirtyVector, 1, 0) == ~0)
    {
      return(STATUS_SUCCESS);
    }

  Status = CmOpenHiveFiles(RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Success = HvSyncHive(&RegistryHive->Hive);

  CmCloseHiveFiles(RegistryHive);

  return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


ULONG
CmiGetNumberOfSubKeys(PKEY_OBJECT KeyObject)
{
  PCM_KEY_NODE KeyCell;
  ULONG SubKeyCount;

  VERIFY_KEY_OBJECT(KeyObject);

  KeyCell = KeyObject->KeyCell;
  VERIFY_KEY_CELL(KeyCell);

  SubKeyCount = (KeyCell == NULL) ? 0 :
                KeyCell->SubKeyCounts[HvStable] +
                KeyCell->SubKeyCounts[HvVolatile];

  return SubKeyCount;
}


ULONG
CmiGetMaxNameLength(PKEY_OBJECT KeyObject)
{
  PHASH_TABLE_CELL HashBlock;
  PCM_KEY_NODE CurSubKeyCell;
  PCM_KEY_NODE KeyCell;
  ULONG MaxName;
  ULONG NameSize;
  ULONG i;
  ULONG Storage;

  VERIFY_KEY_OBJECT(KeyObject);

  KeyCell = KeyObject->KeyCell;
  VERIFY_KEY_CELL(KeyCell);

  MaxName = 0;
  for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
      if (KeyCell->SubKeyLists[Storage] != HCELL_NULL)
        {
          HashBlock = HvGetCell (&KeyObject->RegistryHive->Hive, KeyCell->SubKeyLists[Storage]);
          ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

          for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
              CurSubKeyCell = HvGetCell (&KeyObject->RegistryHive->Hive,
                                         HashBlock->Table[i].KeyOffset);
              NameSize = CurSubKeyCell->NameSize;
              if (CurSubKeyCell->Flags & REG_KEY_NAME_PACKED)
                 NameSize *= sizeof(WCHAR);
              if (NameSize > MaxName)
                 MaxName = NameSize;
            }
        }
    }

  DPRINT ("MaxName %lu\n", MaxName);

  return MaxName;
}


ULONG
CmiGetMaxClassLength(PKEY_OBJECT  KeyObject)
{
  PHASH_TABLE_CELL HashBlock;
  PCM_KEY_NODE CurSubKeyCell;
  PCM_KEY_NODE KeyCell;
  ULONG MaxClass;
  ULONG i;
  ULONG Storage;

  VERIFY_KEY_OBJECT(KeyObject);

  KeyCell = KeyObject->KeyCell;
  VERIFY_KEY_CELL(KeyCell);

  MaxClass = 0;
  for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
      if (KeyCell->SubKeyLists[Storage] != HCELL_NULL)
        {
          HashBlock = HvGetCell (&KeyObject->RegistryHive->Hive,
                                 KeyCell->SubKeyLists[Storage]);
          ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

          for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
              CurSubKeyCell = HvGetCell (&KeyObject->RegistryHive->Hive,
                                         HashBlock->Table[i].KeyOffset);

              if (MaxClass < CurSubKeyCell->ClassSize)
                {
                  MaxClass = CurSubKeyCell->ClassSize;
                }
            }
        }
    }

  return MaxClass;
}


ULONG
CmiGetMaxValueNameLength(PEREGISTRY_HIVE RegistryHive,
			 PCM_KEY_NODE KeyCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  ULONG MaxValueName;
  ULONG Size;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  if (KeyCell->ValueList.List == HCELL_NULL)
    {
      return 0;
    }

  MaxValueName = 0;
  ValueListCell = HvGetCell (&RegistryHive->Hive,
			     KeyCell->ValueList.List);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive,
				ValueListCell->ValueOffset[i]);
      if (CurValueCell == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	}

      if (CurValueCell != NULL)
	{
	  Size = CurValueCell->NameSize;
	  if (CurValueCell->Flags & REG_VALUE_NAME_PACKED)
	    {
	      Size *= sizeof(WCHAR);
	    }
	  if (MaxValueName < Size)
	    {
	      MaxValueName = Size;
	    }
        }
    }

  return MaxValueName;
}


ULONG
CmiGetMaxValueDataLength(PEREGISTRY_HIVE RegistryHive,
			 PCM_KEY_NODE KeyCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  LONG MaxValueData;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  if (KeyCell->ValueList.List == HCELL_NULL)
    {
      return 0;
    }

  MaxValueData = 0;
  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive,
                                ValueListCell->ValueOffset[i]);
      if ((MaxValueData < (LONG)(CurValueCell->DataSize & REG_DATA_SIZE_MASK)))
        {
          MaxValueData = CurValueCell->DataSize & REG_DATA_SIZE_MASK;
        }
    }

  return MaxValueData;
}


NTSTATUS
CmiScanForSubKey(IN PEREGISTRY_HIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 OUT PCM_KEY_NODE *SubKeyCell,
		 OUT HCELL_INDEX *BlockOffset,
		 IN PUNICODE_STRING KeyName,
		 IN ACCESS_MASK DesiredAccess,
		 IN ULONG Attributes)
{
  PHASH_TABLE_CELL HashBlock;
  PCM_KEY_NODE CurSubKeyCell;
  ULONG Storage;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  DPRINT("Scanning for sub key %wZ\n", KeyName);

  ASSERT(RegistryHive);

  *SubKeyCell = NULL;

  for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
      /* The key does not have any subkeys */
      if (KeyCell->SubKeyLists[Storage] == HCELL_NULL)
        {
          continue;
        }

      /* Get hash table */
      HashBlock = HvGetCell (&RegistryHive->Hive, KeyCell->SubKeyLists[Storage]);
      ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

      for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
        {
          if (Attributes & OBJ_CASE_INSENSITIVE)
            {
              if ((HashBlock->Table[i].HashValue == 0 ||
                   CmiCompareHashI(KeyName, (PCHAR)&HashBlock->Table[i].HashValue)))
                {
                  CurSubKeyCell = HvGetCell (&RegistryHive->Hive,
            				 HashBlock->Table[i].KeyOffset);

                  if (CmiCompareKeyNamesI(KeyName, CurSubKeyCell))
                    {
                      *SubKeyCell = CurSubKeyCell;
                      *BlockOffset = HashBlock->Table[i].KeyOffset;
                      return STATUS_SUCCESS;
                    }
                }
            }
          else
            {
              if ((HashBlock->Table[i].HashValue == 0 ||
                   CmiCompareHash(KeyName, (PCHAR)&HashBlock->Table[i].HashValue)))
                {
                  CurSubKeyCell = HvGetCell (&RegistryHive->Hive,
            				 HashBlock->Table[i].KeyOffset);

                  if (CmiCompareKeyNames(KeyName, CurSubKeyCell))
                    {
                      *SubKeyCell = CurSubKeyCell;
                      *BlockOffset = HashBlock->Table[i].KeyOffset;
                      return STATUS_SUCCESS;
                    }
                }
            }
        }
    }

  return STATUS_OBJECT_NAME_NOT_FOUND;
}


NTSTATUS
CmiAddSubKey(PEREGISTRY_HIVE RegistryHive,
	     PKEY_OBJECT ParentKey,
	     PKEY_OBJECT SubKey,
	     PUNICODE_STRING SubKeyName,
	     ULONG TitleIndex,
	     PUNICODE_STRING Class,
	     ULONG CreateOptions)
{
  PHASH_TABLE_CELL HashBlock;
  HCELL_INDEX NKBOffset;
  PCM_KEY_NODE NewKeyCell;
  ULONG NewBlockSize;
  PCM_KEY_NODE ParentKeyCell;
  PVOID ClassCell;
  NTSTATUS Status;
  USHORT NameSize;
  PWSTR NamePtr;
  BOOLEAN Packable;
  HV_STORAGE_TYPE Storage;
  ULONG i;

  ParentKeyCell = ParentKey->KeyCell;

  VERIFY_KEY_CELL(ParentKeyCell);

  /* Skip leading backslash */
  if (SubKeyName->Buffer[0] == L'\\')
    {
      NamePtr = &SubKeyName->Buffer[1];
      NameSize = SubKeyName->Length - sizeof(WCHAR);
    }
  else
    {
      NamePtr = SubKeyName->Buffer;
      NameSize = SubKeyName->Length;
    }

  /* Check whether key name can be packed */
  Packable = TRUE;
  for (i = 0; i < NameSize / sizeof(WCHAR); i++)
    {
      if (NamePtr[i] & 0xFF00)
	{
	  Packable = FALSE;
	  break;
	}
    }

  /* Adjust name size */
  if (Packable)
    {
      NameSize = NameSize / sizeof(WCHAR);
    }

  DPRINT("Key %S  Length %lu  %s\n", NamePtr, NameSize, (Packable)?"True":"False");

  Status = STATUS_SUCCESS;

  Storage = (CreateOptions & REG_OPTION_VOLATILE) ? HvVolatile : HvStable;
  NewBlockSize = sizeof(CM_KEY_NODE) + NameSize;
  NKBOffset = HvAllocateCell (&RegistryHive->Hive, NewBlockSize, Storage);
  if (NKBOffset == HCELL_NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      NewKeyCell = HvGetCell (&RegistryHive->Hive, NKBOffset);
      NewKeyCell->Id = REG_KEY_CELL_ID;
      if (CreateOptions & REG_OPTION_VOLATILE)
        {
          NewKeyCell->Flags = REG_KEY_VOLATILE_CELL;
        }
      else
        {
          NewKeyCell->Flags = 0;
        }
      KeQuerySystemTime(&NewKeyCell->LastWriteTime);
      NewKeyCell->Parent = HCELL_NULL;
      NewKeyCell->SubKeyCounts[HvStable] = 0;
      NewKeyCell->SubKeyCounts[HvVolatile] = 0;
      NewKeyCell->SubKeyLists[HvStable] = HCELL_NULL;
      NewKeyCell->SubKeyLists[HvVolatile] = HCELL_NULL;
      NewKeyCell->ValueList.Count = 0;
      NewKeyCell->ValueList.List = HCELL_NULL;
      NewKeyCell->SecurityKeyOffset = HCELL_NULL;
      NewKeyCell->ClassNameOffset = HCELL_NULL;

      /* Pack the key name */
      NewKeyCell->NameSize = NameSize;
      if (Packable)
	{
	  NewKeyCell->Flags |= REG_KEY_NAME_PACKED;
	  for (i = 0; i < NameSize; i++)
	    {
	      NewKeyCell->Name[i] = (CHAR)(NamePtr[i] & 0x00FF);
	    }
	}
      else
	{
	  RtlCopyMemory(NewKeyCell->Name,
			NamePtr,
			NameSize);
	}

      VERIFY_KEY_CELL(NewKeyCell);

      if (Class != NULL && Class->Length)
	{
	  NewKeyCell->ClassSize = Class->Length;
	  NewKeyCell->ClassNameOffset = HvAllocateCell(
	    &RegistryHive->Hive, NewKeyCell->ClassSize, HvStable);
	  ASSERT(NewKeyCell->ClassNameOffset != HCELL_NULL); /* FIXME */

	  ClassCell = HvGetCell(&RegistryHive->Hive, NewKeyCell->ClassNameOffset);
	  RtlCopyMemory (ClassCell,
			 Class->Buffer,
			 Class->Length);
	}
    }

  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  SubKey->KeyCell = NewKeyCell;
  SubKey->KeyCellOffset = NKBOffset;

  if (ParentKeyCell->SubKeyLists[Storage] == HCELL_NULL)
    {
      Status = CmiAllocateHashTableCell (RegistryHive,
					 &HashBlock,
					 &ParentKeyCell->SubKeyLists[Storage],
					 REG_INIT_HASH_TABLE_SIZE,
					 Storage);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      HashBlock = HvGetCell (&RegistryHive->Hive,
			     ParentKeyCell->SubKeyLists[Storage]);
      ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

      if (((ParentKeyCell->SubKeyCounts[Storage] + 1) >= HashBlock->HashTableSize))
	{
	  PHASH_TABLE_CELL NewHashBlock;
	  HCELL_INDEX HTOffset;

	  /* Reallocate the hash table cell */
	  Status = CmiAllocateHashTableCell (RegistryHive,
					     &NewHashBlock,
					     &HTOffset,
					     HashBlock->HashTableSize +
					       REG_EXTEND_HASH_TABLE_SIZE,
					     Storage);
	  if (!NT_SUCCESS(Status))
	    {
	      return Status;
	    }

	  RtlZeroMemory(&NewHashBlock->Table[0],
			sizeof(NewHashBlock->Table[0]) * NewHashBlock->HashTableSize);
	  RtlCopyMemory(&NewHashBlock->Table[0],
			&HashBlock->Table[0],
			sizeof(NewHashBlock->Table[0]) * HashBlock->HashTableSize);
	  HvFreeCell (&RegistryHive->Hive, ParentKeyCell->SubKeyLists[Storage]);
	  ParentKeyCell->SubKeyLists[Storage] = HTOffset;
	  HashBlock = NewHashBlock;
	}
    }

  Status = CmiAddKeyToHashTable(RegistryHive,
				HashBlock,
				ParentKeyCell,
				Storage,
				NewKeyCell,
				NKBOffset);
  if (NT_SUCCESS(Status))
    {
      ParentKeyCell->SubKeyCounts[Storage]++;
    }

  KeQuerySystemTime (&ParentKeyCell->LastWriteTime);
  HvMarkCellDirty (&RegistryHive->Hive, ParentKey->KeyCellOffset);

  return(Status);
}


NTSTATUS
CmiRemoveSubKey(PEREGISTRY_HIVE RegistryHive,
		PKEY_OBJECT ParentKey,
		PKEY_OBJECT SubKey)
{
  PHASH_TABLE_CELL HashBlock;
  PVALUE_LIST_CELL ValueList;
  PCM_KEY_VALUE ValueCell;
  HV_STORAGE_TYPE Storage;
  ULONG i;

  DPRINT("CmiRemoveSubKey() called\n");

  Storage = (SubKey->KeyCell->Flags & REG_KEY_VOLATILE_CELL) ? HvVolatile : HvStable;

  /* Remove all values */
  if (SubKey->KeyCell->ValueList.Count != 0)
    {
      /* Get pointer to the value list cell */
      ValueList = HvGetCell (&RegistryHive->Hive, SubKey->KeyCell->ValueList.List);

      /* Enumerate all values */
      for (i = 0; i < SubKey->KeyCell->ValueList.Count; i++)
	{
	  /* Get pointer to value cell */
	  ValueCell = HvGetCell(&RegistryHive->Hive,
				ValueList->ValueOffset[i]);

	  if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET)
              && ValueCell->DataSize > sizeof(HCELL_INDEX)
              && ValueCell->DataOffset != HCELL_NULL)
	    {
	      /* Destroy data cell */
	      HvFreeCell (&RegistryHive->Hive, ValueCell->DataOffset);
	    }

	  /* Destroy value cell */
	  HvFreeCell (&RegistryHive->Hive, ValueList->ValueOffset[i]);
	}

      /* Destroy value list cell */
      HvFreeCell (&RegistryHive->Hive, SubKey->KeyCell->ValueList.List);

      SubKey->KeyCell->ValueList.Count = 0;
      SubKey->KeyCell->ValueList.List = (HCELL_INDEX)-1;

      HvMarkCellDirty(&RegistryHive->Hive, SubKey->KeyCellOffset);
    }

  /* Remove the key from the parent key's hash block */
  if (ParentKey->KeyCell->SubKeyLists[Storage] != HCELL_NULL)
    {
      DPRINT("ParentKey SubKeyLists %lx\n", ParentKey->KeyCell->SubKeyLists[Storage]);
      HashBlock = HvGetCell (&ParentKey->RegistryHive->Hive,
			     ParentKey->KeyCell->SubKeyLists[Storage]);
      ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);
      DPRINT("ParentKey HashBlock %p\n", HashBlock);
      CmiRemoveKeyFromHashTable(ParentKey->RegistryHive,
				HashBlock,
				SubKey->KeyCellOffset);
      HvMarkCellDirty(&ParentKey->RegistryHive->Hive,
                      ParentKey->KeyCell->SubKeyLists[Storage]);
    }

  /* Remove the key's hash block */
  if (SubKey->KeyCell->SubKeyLists[Storage] != HCELL_NULL)
    {
      DPRINT("SubKey SubKeyLists %lx\n", SubKey->KeyCell->SubKeyLists[Storage]);
      HvFreeCell (&RegistryHive->Hive, SubKey->KeyCell->SubKeyLists[Storage]);
      SubKey->KeyCell->SubKeyLists[Storage] = HCELL_NULL;
    }

  /* Decrement the number of the parent key's sub keys */
  if (ParentKey != NULL)
    {
      DPRINT("ParentKey %p\n", ParentKey);
      ParentKey->KeyCell->SubKeyCounts[Storage]--;

      /* Remove the parent key's hash table */
      if (ParentKey->KeyCell->SubKeyCounts[Storage] == 0 &&
          ParentKey->KeyCell->SubKeyLists[Storage] != HCELL_NULL)
	{
	  DPRINT("ParentKey SubKeyLists %lx\n", ParentKey->KeyCell->SubKeyLists);
	  HvFreeCell (&ParentKey->RegistryHive->Hive,
		      ParentKey->KeyCell->SubKeyLists[Storage]);
	  ParentKey->KeyCell->SubKeyLists[Storage] = HCELL_NULL;
	}

      KeQuerySystemTime(&ParentKey->KeyCell->LastWriteTime);
      HvMarkCellDirty(&ParentKey->RegistryHive->Hive,
                      ParentKey->KeyCellOffset);
    }

  /* Destroy key cell */
  HvFreeCell (&RegistryHive->Hive, SubKey->KeyCellOffset);
  SubKey->KeyCell = NULL;
  SubKey->KeyCellOffset = (HCELL_INDEX)-1;

  DPRINT("CmiRemoveSubKey() done\n");

  return STATUS_SUCCESS;
}


NTSTATUS
CmiScanKeyForValue(IN PEREGISTRY_HIVE RegistryHive,
		   IN PCM_KEY_NODE KeyCell,
		   IN PUNICODE_STRING ValueName,
		   OUT PCM_KEY_VALUE *ValueCell,
		   OUT HCELL_INDEX *ValueCellOffset)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  ULONG i;

  *ValueCell = NULL;
  if (ValueCellOffset != NULL)
    *ValueCellOffset = (HCELL_INDEX)-1;

  /* The key does not have any values */
  if (KeyCell->ValueList.List == (HCELL_INDEX)-1)
    {
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive,
				ValueListCell->ValueOffset[i]);

      if (CmiComparePackedNames(ValueName,
				CurValueCell->Name,
				CurValueCell->NameSize,
				(BOOLEAN)((CurValueCell->Flags & REG_VALUE_NAME_PACKED) ? TRUE : FALSE)))
	{
	  *ValueCell = CurValueCell;
	  if (ValueCellOffset != NULL)
	    *ValueCellOffset = ValueListCell->ValueOffset[i];
	  //DPRINT("Found value %s\n", ValueName);
	  return STATUS_SUCCESS;
	}
    }

  return STATUS_OBJECT_NAME_NOT_FOUND;
}


NTSTATUS
CmiGetValueFromKeyByIndex(IN PEREGISTRY_HIVE RegistryHive,
			  IN PCM_KEY_NODE KeyCell,
			  IN ULONG Index,
			  OUT PCM_KEY_VALUE *ValueCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;

  *ValueCell = NULL;

  if (KeyCell->ValueList.List == (HCELL_INDEX)-1)
    {
      return STATUS_NO_MORE_ENTRIES;
    }

  if (Index >= KeyCell->ValueList.Count)
    {
      return STATUS_NO_MORE_ENTRIES;
    }


  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  CurValueCell = HvGetCell (&RegistryHive->Hive, ValueListCell->ValueOffset[Index]);

  *ValueCell = CurValueCell;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiAddValueToKey(IN PEREGISTRY_HIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 IN HCELL_INDEX KeyCellOffset,
		 IN PUNICODE_STRING ValueName,
		 OUT PCM_KEY_VALUE *pValueCell,
		 OUT HCELL_INDEX *pValueCellOffset)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE NewValueCell;
  HCELL_INDEX ValueListCellOffset;
  HCELL_INDEX NewValueCellOffset;
  ULONG CellSize;
  HV_STORAGE_TYPE Storage;
  NTSTATUS Status;

  DPRINT("KeyCell->ValuesOffset %lu\n", (ULONG)KeyCell->ValueList.List);

  Storage = (KeyCell->Flags & REG_KEY_VOLATILE_CELL) ? HvVolatile : HvStable;
  if (KeyCell->ValueList.List == HCELL_NULL)
    {
      CellSize = sizeof(VALUE_LIST_CELL) +
		 (3 * sizeof(HCELL_INDEX));
      ValueListCellOffset = HvAllocateCell (&RegistryHive->Hive, CellSize, Storage);
      if (ValueListCellOffset == HCELL_NULL)
	{
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      ValueListCell = HvGetCell (&RegistryHive->Hive, ValueListCellOffset);
      KeyCell->ValueList.List = ValueListCellOffset;
      HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
    }
  else
    {
      ValueListCell = (PVALUE_LIST_CELL) HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);
      CellSize = ABS_VALUE(HvGetCellSize(&RegistryHive->Hive, ValueListCell));

      if (KeyCell->ValueList.Count >=
	   (CellSize / sizeof(HCELL_INDEX)))
        {
          CellSize *= 2;
          ValueListCellOffset = HvReallocateCell (&RegistryHive->Hive, KeyCell->ValueList.List, CellSize);
          if (ValueListCellOffset == HCELL_NULL)
            {
              return STATUS_INSUFFICIENT_RESOURCES;
            }

          ValueListCell = HvGetCell (&RegistryHive->Hive, ValueListCellOffset);
          KeyCell->ValueList.List = ValueListCellOffset;
          HvMarkCellDirty (&RegistryHive->Hive, KeyCellOffset);
        }
    }

#if 0
  DPRINT("KeyCell->ValueList.Count %lu, ValueListCell->Size %lu (%lu %lx)\n",
	 KeyCell->ValueList.Count,
	 (ULONG)ABS_VALUE(ValueListCell->Size),
	 ((ULONG)ABS_VALUE(ValueListCell->Size) - sizeof(HCELL)) / sizeof(HCELL_INDEX),
	 ((ULONG)ABS_VALUE(ValueListCell->Size) - sizeof(HCELL)) / sizeof(HCELL_INDEX));
#endif

  Status = CmiAllocateValueCell(RegistryHive,
				&NewValueCell,
				&NewValueCellOffset,
				ValueName,
				Storage);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  ValueListCell->ValueOffset[KeyCell->ValueList.Count] = NewValueCellOffset;
  KeyCell->ValueList.Count++;

  HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
  HvMarkCellDirty(&RegistryHive->Hive, KeyCell->ValueList.List);
  HvMarkCellDirty(&RegistryHive->Hive, NewValueCellOffset);

  *pValueCell = NewValueCell;
  *pValueCellOffset = NewValueCellOffset;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiDeleteValueFromKey(IN PEREGISTRY_HIVE RegistryHive,
		      IN PCM_KEY_NODE KeyCell,
		      IN HCELL_INDEX KeyCellOffset,
		      IN PUNICODE_STRING ValueName)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  ULONG i;
  NTSTATUS Status;

  if (KeyCell->ValueList.List == -1)
    {
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive, ValueListCell->ValueOffset[i]);

      if (CmiComparePackedNames(ValueName,
				CurValueCell->Name,
				CurValueCell->NameSize,
				(BOOLEAN)((CurValueCell->Flags & REG_VALUE_NAME_PACKED) ? TRUE : FALSE)))
	{
	  Status = CmiDestroyValueCell(RegistryHive,
				       CurValueCell,
				       ValueListCell->ValueOffset[i]);
	  if (CurValueCell == NULL)
	    {
	      DPRINT1("CmiDestroyValueCell() failed\n");
	      return Status;
	    }

	  if (i < (KeyCell->ValueList.Count - 1))
	    {
	      RtlMoveMemory(&ValueListCell->ValueOffset[i],
			    &ValueListCell->ValueOffset[i + 1],
			    sizeof(HCELL_INDEX) * (KeyCell->ValueList.Count - 1 - i));
	    }
	  ValueListCell->ValueOffset[KeyCell->ValueList.Count - 1] = 0;


	  KeyCell->ValueList.Count--;

	  if (KeyCell->ValueList.Count == 0)
	    {
	      HvFreeCell(&RegistryHive->Hive, KeyCell->ValueList.List);
	      KeyCell->ValueList.List = -1;
	    }
	  else
	    {
	      HvMarkCellDirty(&RegistryHive->Hive,
		              KeyCell->ValueList.List);
	    }

	  HvMarkCellDirty(&RegistryHive->Hive,
			  KeyCellOffset);

	  return STATUS_SUCCESS;
	}
    }

  DPRINT("Couldn't find the desired value\n");

  return STATUS_OBJECT_NAME_NOT_FOUND;
}


NTSTATUS
CmiAllocateHashTableCell (IN PEREGISTRY_HIVE RegistryHive,
	OUT PHASH_TABLE_CELL *HashBlock,
	OUT HCELL_INDEX *HBOffset,
	IN ULONG SubKeyCount,
	IN HV_STORAGE_TYPE Storage)
{
  PHASH_TABLE_CELL NewHashBlock;
  ULONG NewHashSize;
  NTSTATUS Status;

  Status = STATUS_SUCCESS;
  *HashBlock = NULL;
  NewHashSize = sizeof(HASH_TABLE_CELL) +
		(SubKeyCount * sizeof(HASH_RECORD));
  *HBOffset = HvAllocateCell (&RegistryHive->Hive, NewHashSize, Storage);

  if (*HBOffset == HCELL_NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      ASSERT(SubKeyCount <= 0xffff); /* should really be USHORT_MAX or similar */
      NewHashBlock = HvGetCell (&RegistryHive->Hive, *HBOffset);
      NewHashBlock->Id = REG_HASH_TABLE_CELL_ID;
      NewHashBlock->HashTableSize = (USHORT)SubKeyCount;
      *HashBlock = NewHashBlock;
    }

  return Status;
}


PCM_KEY_NODE
CmiGetKeyFromHashByIndex(PEREGISTRY_HIVE RegistryHive,
			 PHASH_TABLE_CELL HashBlock,
			 ULONG Index)
{
  HCELL_INDEX KeyOffset;
  PCM_KEY_NODE KeyCell;

  KeyOffset =  HashBlock->Table[Index].KeyOffset;
  KeyCell = HvGetCell (&RegistryHive->Hive, KeyOffset);

  return KeyCell;
}


NTSTATUS
CmiAddKeyToHashTable(PEREGISTRY_HIVE RegistryHive,
		     PHASH_TABLE_CELL HashCell,
		     PCM_KEY_NODE KeyCell,
		     HV_STORAGE_TYPE StorageType,
		     PCM_KEY_NODE NewKeyCell,
		     HCELL_INDEX NKBOffset)
{
  ULONG i = KeyCell->SubKeyCounts[StorageType];

  HashCell->Table[i].KeyOffset = NKBOffset;
  HashCell->Table[i].HashValue = 0;
  if (NewKeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      RtlCopyMemory(&HashCell->Table[i].HashValue,
                    NewKeyCell->Name,
                    min(NewKeyCell->NameSize, sizeof(ULONG)));
    }
  HvMarkCellDirty(&RegistryHive->Hive, KeyCell->SubKeyLists[StorageType]);
  return STATUS_SUCCESS;
}


NTSTATUS
CmiRemoveKeyFromHashTable(PEREGISTRY_HIVE RegistryHive,
			  PHASH_TABLE_CELL HashBlock,
			  HCELL_INDEX NKBOffset)
{
  ULONG i;

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == NKBOffset)
	{
	  RtlMoveMemory(HashBlock->Table + i,
	                HashBlock->Table + i + 1,
	                (HashBlock->HashTableSize - i - 1) *
	                sizeof(HashBlock->Table[0]));
	  return STATUS_SUCCESS;
	}
    }

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
CmiAllocateValueCell(PEREGISTRY_HIVE RegistryHive,
		     PCM_KEY_VALUE *ValueCell,
		     HCELL_INDEX *VBOffset,
		     IN PUNICODE_STRING ValueName,
		     IN HV_STORAGE_TYPE Storage)
{
  PCM_KEY_VALUE NewValueCell;
  NTSTATUS Status;
  BOOLEAN Packable;
  ULONG NameSize;
  ULONG i;

  Status = STATUS_SUCCESS;

  NameSize = CmiGetPackedNameLength(ValueName,
				    &Packable);

  DPRINT("ValueName->Length %lu  NameSize %lu\n", ValueName->Length, NameSize);

  *VBOffset = HvAllocateCell (&RegistryHive->Hive, sizeof(CM_KEY_VALUE) + NameSize, Storage);
  if (*VBOffset == HCELL_NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      ASSERT(NameSize <= 0xffff); /* should really be USHORT_MAX or similar */
      NewValueCell = HvGetCell (&RegistryHive->Hive, *VBOffset);
      NewValueCell->Id = REG_VALUE_CELL_ID;
      NewValueCell->NameSize = (USHORT)NameSize;
      if (Packable)
	{
	  /* Pack the value name */
	  for (i = 0; i < NameSize; i++)
	    NewValueCell->Name[i] = (CHAR)ValueName->Buffer[i];
	  NewValueCell->Flags |= REG_VALUE_NAME_PACKED;
	}
      else
	{
	  /* Copy the value name */
	  RtlCopyMemory(NewValueCell->Name,
			ValueName->Buffer,
			NameSize);
	  NewValueCell->Flags = 0;
	}
      NewValueCell->DataType = 0;
      NewValueCell->DataSize = 0;
      NewValueCell->DataOffset = (HCELL_INDEX)-1;
      *ValueCell = NewValueCell;
    }

  return Status;
}


NTSTATUS
CmiDestroyValueCell(PEREGISTRY_HIVE RegistryHive,
		    PCM_KEY_VALUE ValueCell,
		    HCELL_INDEX ValueCellOffset)
{
  DPRINT("CmiDestroyValueCell(Cell %p  Offset %lx)\n",
	 ValueCell, ValueCellOffset);

  VERIFY_VALUE_CELL(ValueCell);

  /* Destroy the data cell */
  if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET)
      && ValueCell->DataSize > sizeof(HCELL_INDEX)
      && ValueCell->DataOffset != HCELL_NULL)
    {
      HvFreeCell (&RegistryHive->Hive, ValueCell->DataOffset);
    }

  /* Destroy the value cell */
  HvFreeCell (&RegistryHive->Hive, ValueCellOffset);

  return STATUS_SUCCESS;
}


ULONG
CmiGetPackedNameLength(IN PUNICODE_STRING Name,
		       OUT PBOOLEAN Packable)
{
  ULONG i;

  if (Packable != NULL)
    *Packable = TRUE;

  for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
    {
      if (Name->Buffer[i] & 0xFF00)
	{
	  if (Packable != NULL)
	    *Packable = FALSE;
	  return Name->Length;
	}
    }

  return (Name->Length / sizeof(WCHAR));
}


BOOLEAN
CmiComparePackedNames(IN PUNICODE_STRING Name,
		      IN PUCHAR NameBuffer,
		      IN USHORT NameBufferSize,
		      IN BOOLEAN NamePacked)
{
  PWCHAR UNameBuffer;
  ULONG i;

  if (NamePacked == TRUE)
    {
      if (Name->Length != NameBufferSize * sizeof(WCHAR))
	return(FALSE);

      for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar((WCHAR)NameBuffer[i]))
	    return(FALSE);
	}
    }
  else
    {
      if (Name->Length != NameBufferSize)
	return(FALSE);

      UNameBuffer = (PWCHAR)NameBuffer;

      for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar(UNameBuffer[i]))
	    return(FALSE);
	}
    }

  return(TRUE);
}


VOID
CmiCopyPackedName(PWCHAR NameBuffer,
		  PUCHAR PackedNameBuffer,
		  ULONG PackedNameSize)
{
  ULONG i;

  for (i = 0; i < PackedNameSize; i++)
    NameBuffer[i] = (WCHAR)PackedNameBuffer[i];
}


BOOLEAN
CmiCompareHash(PUNICODE_STRING KeyName,
	       PCHAR HashString)
{
  CHAR Buffer[4];

  Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
  Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
  Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
  Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

  return (strncmp(Buffer, HashString, 4) == 0);
}


BOOLEAN
CmiCompareHashI(PUNICODE_STRING KeyName,
	        PCHAR HashString)
{
  CHAR Buffer[4];

  Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
  Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
  Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
  Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

  return (_strnicmp(Buffer, HashString, 4) == 0);
}


BOOLEAN
CmiCompareKeyNames(PUNICODE_STRING KeyName,
		   PCM_KEY_NODE KeyCell)
{
  PWCHAR UnicodeName;
  USHORT i;

  DPRINT("Flags: %hx\n", KeyCell->Flags);

  if (KeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      if (KeyName->Length != KeyCell->NameSize * sizeof(WCHAR))
	return FALSE;

      for (i = 0; i < KeyCell->NameSize; i++)
	{
	  if (KeyName->Buffer[i] != (WCHAR)KeyCell->Name[i])
	    return FALSE;
	}
    }
  else
    {
      if (KeyName->Length != KeyCell->NameSize)
	return FALSE;

      UnicodeName = (PWCHAR)KeyCell->Name;
      for (i = 0; i < KeyCell->NameSize / sizeof(WCHAR); i++)
	{
	  if (KeyName->Buffer[i] != UnicodeName[i])
	    return FALSE;
	}
    }

  return TRUE;
}


BOOLEAN
CmiCompareKeyNamesI(PUNICODE_STRING KeyName,
		    PCM_KEY_NODE KeyCell)
{
  PWCHAR UnicodeName;
  USHORT i;

  DPRINT("Flags: %hx\n", KeyCell->Flags);

  if (KeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      if (KeyName->Length != KeyCell->NameSize * sizeof(WCHAR))
	return FALSE;

      for (i = 0; i < KeyCell->NameSize; i++)
	{
	  if (RtlUpcaseUnicodeChar(KeyName->Buffer[i]) !=
	      RtlUpcaseUnicodeChar((WCHAR)KeyCell->Name[i]))
	    return FALSE;
	}
    }
  else
    {
      if (KeyName->Length != KeyCell->NameSize)
	return FALSE;

      UnicodeName = (PWCHAR)KeyCell->Name;
      for (i = 0; i < KeyCell->NameSize / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(KeyName->Buffer[i]) !=
	      RtlUpcaseUnicodeChar(UnicodeName[i]))
	    return FALSE;
	}
    }

  return TRUE;
}


NTSTATUS
CmiCopyKey (PEREGISTRY_HIVE DstHive,
	    PCM_KEY_NODE DstKeyCell,
	    PEREGISTRY_HIVE SrcHive,
	    PCM_KEY_NODE SrcKeyCell)
{
  PCM_KEY_NODE NewKeyCell;
  ULONG NewKeyCellSize;
  HCELL_INDEX NewKeyCellOffset;
  PHASH_TABLE_CELL NewHashTableCell;
  ULONG NewHashTableSize;
  HCELL_INDEX NewHashTableOffset;
  ULONG i;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT ("CmiCopyKey() called\n");

  if (DstKeyCell == NULL)
    {
      /* Allocate and copy key cell */
      NewKeyCellSize = sizeof(CM_KEY_NODE) + SrcKeyCell->NameSize;
      NewKeyCellOffset = HvAllocateCell (&DstHive->Hive, NewKeyCellSize, HvStable);
      if (NewKeyCellOffset == HCELL_NULL)
	{
	  DPRINT1 ("Failed to allocate a key cell\n");
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      NewKeyCell = HvGetCell (&DstHive->Hive, NewKeyCellOffset);
      RtlCopyMemory (NewKeyCell,
		     SrcKeyCell,
		     NewKeyCellSize);

      DstHive->Hive.HiveHeader->RootCell = NewKeyCellOffset;

      /* Copy class name */
      if (SrcKeyCell->ClassNameOffset != (HCELL_INDEX) -1)
	{
	  PVOID SrcClassNameCell;
	  PVOID NewClassNameCell;
	  HCELL_INDEX NewClassNameOffset;

	  SrcClassNameCell = HvGetCell (&SrcHive->Hive, SrcKeyCell->ClassNameOffset);

	  NewKeyCell->ClassSize = SrcKeyCell->ClassSize;
	  NewClassNameOffset = HvAllocateCell (&DstHive->Hive, NewKeyCell->ClassSize, HvStable);
	  if (NewClassNameOffset == HCELL_NULL)
	    {
	      DPRINT1 ("CmiAllocateBlock() failed\n");
	      return STATUS_INSUFFICIENT_RESOURCES;
	    }

	  NewClassNameCell = HvGetCell (&DstHive->Hive, NewClassNameOffset);
	  RtlCopyMemory (NewClassNameCell,
			 SrcClassNameCell,
			 NewKeyCell->ClassSize);
	  NewKeyCell->ClassNameOffset = NewClassNameOffset;
	}
    }
  else
    {
      NewKeyCell = DstKeyCell;
    }

  /* Allocate hash table */
  if (SrcKeyCell->SubKeyCounts[HvStable] > 0)
    {
      NewHashTableSize = ROUND_UP(SrcKeyCell->SubKeyCounts[HvStable] + 1, 4) - 1;
      Status = CmiAllocateHashTableCell (DstHive,
					 &NewHashTableCell,
					 &NewHashTableOffset,
					 NewHashTableSize,
					 HvStable);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1 ("CmiAllocateHashTableBlock() failed (Status %lx)\n", Status);
	  return Status;
	}
      NewKeyCell->SubKeyLists[HvStable] = NewHashTableOffset;
    }
  else
    {
      NewHashTableCell = NULL;
    }

  /* Allocate and copy value list and values */
  if (SrcKeyCell->ValueList.Count != 0)
    {
      PVALUE_LIST_CELL NewValueListCell;
      PVALUE_LIST_CELL SrcValueListCell;
      PCM_KEY_VALUE NewValueCell;
      PCM_KEY_VALUE SrcValueCell;
      PVOID SrcValueDataCell;
      PVOID NewValueDataCell;
      HCELL_INDEX ValueCellOffset;
      HCELL_INDEX ValueDataCellOffset;
      ULONG NewValueListCellSize;
      ULONG NewValueCellSize;


      NewValueListCellSize =
	ROUND_UP(SrcKeyCell->ValueList.Count, 4) * sizeof(HCELL_INDEX);
      NewKeyCell->ValueList.List = HvAllocateCell (&DstHive->Hive,
				                    NewValueListCellSize,
				                    HvStable);
      if (NewKeyCell->ValueList.List == HCELL_NULL)
	{
	  DPRINT1 ("HvAllocateCell() failed\n");
	  return STATUS_INSUFFICIENT_RESOURCES;
	}
      DPRINT1("KeyCell->ValueList.List: %x\n", NewKeyCell->ValueList.List);

      NewValueListCell = HvGetCell (&DstHive->Hive, NewKeyCell->ValueList.List);
      RtlZeroMemory (NewValueListCell,
		     NewValueListCellSize);

      /* Copy values */
      SrcValueListCell = HvGetCell (&SrcHive->Hive, SrcKeyCell->ValueList.List);
      for (i = 0; i < SrcKeyCell->ValueList.Count; i++)
	{
	  /* Copy value cell */
	  SrcValueCell = HvGetCell (&SrcHive->Hive, SrcValueListCell->ValueOffset[i]);

	  NewValueCellSize = sizeof(CM_KEY_VALUE) + SrcValueCell->NameSize;
	  ValueCellOffset = HvAllocateCell (&DstHive->Hive, NewValueCellSize, HvStable);
	  if (ValueCellOffset == HCELL_NULL)
	    {
	      DPRINT1 ("HvAllocateCell() failed (Status %lx)\n", Status);
	      return STATUS_INSUFFICIENT_RESOURCES;
	    }

	  NewValueCell = HvGetCell (&DstHive->Hive, ValueCellOffset);
	  NewValueListCell->ValueOffset[i] = ValueCellOffset;
	  RtlCopyMemory (NewValueCell,
			 SrcValueCell,
			 NewValueCellSize);

	  /* Copy value data cell */
	  if (SrcValueCell->DataSize > (LONG) sizeof(PVOID))
	    {
	      SrcValueDataCell = HvGetCell (&SrcHive->Hive, SrcValueCell->DataOffset);

	      ValueDataCellOffset = HvAllocateCell (&DstHive->Hive, SrcValueCell->DataSize, HvStable);
	      if (ValueDataCellOffset == HCELL_NULL)
		{
		  DPRINT1 ("HvAllocateCell() failed\n");
		  return STATUS_INSUFFICIENT_RESOURCES;
		}
	      NewValueDataCell = HvGetCell (&DstHive->Hive, ValueDataCellOffset);
	      RtlCopyMemory (NewValueDataCell,
			     SrcValueDataCell,
			     SrcValueCell->DataSize);
	      NewValueCell->DataOffset = ValueDataCellOffset;
	    }
	}
    }

  /* Copy subkeys */
  if (SrcKeyCell->SubKeyCounts[HvStable] > 0)
    {
      PHASH_TABLE_CELL SrcHashTableCell;
      PCM_KEY_NODE SrcSubKeyCell;
      PCM_KEY_NODE NewSubKeyCell;
      ULONG NewSubKeyCellSize;
      HCELL_INDEX NewSubKeyCellOffset;
      PHASH_RECORD SrcHashRecord;

      SrcHashTableCell = HvGetCell (&SrcHive->Hive, SrcKeyCell->SubKeyLists[HvStable]);

      for (i = 0; i < SrcKeyCell->SubKeyCounts[HvStable]; i++)
	{
	  SrcHashRecord = &SrcHashTableCell->Table[i];
	  SrcSubKeyCell = HvGetCell (&SrcHive->Hive, SrcHashRecord->KeyOffset);

	  /* Allocate and copy key cell */
	  NewSubKeyCellSize = sizeof(CM_KEY_NODE) + SrcSubKeyCell->NameSize;
	  NewSubKeyCellOffset = HvAllocateCell (&DstHive->Hive, NewSubKeyCellSize, HvStable);
	  if (NewSubKeyCellOffset == HCELL_NULL)
	    {
	      DPRINT1 ("Failed to allocate a sub key cell\n");
	      return STATUS_INSUFFICIENT_RESOURCES;
	    }

	  NewSubKeyCell = HvGetCell (&DstHive->Hive, NewSubKeyCellOffset);
	  NewHashTableCell->Table[i].KeyOffset = NewSubKeyCellOffset;
	  NewHashTableCell->Table[i].HashValue = SrcHashRecord->HashValue;

	  RtlCopyMemory (NewSubKeyCell,
			 SrcSubKeyCell,
			 NewSubKeyCellSize);

	  /* Copy class name */
	  if (SrcSubKeyCell->ClassNameOffset != (HCELL_INDEX) -1)
	    {
	      PVOID SrcClassNameCell;
	      PVOID NewClassNameCell;
	      HCELL_INDEX NewClassNameOffset;

	      SrcClassNameCell = HvGetCell (&SrcHive->Hive,
					    SrcSubKeyCell->ClassNameOffset);

	      NewSubKeyCell->ClassSize = SrcSubKeyCell->ClassSize;
	      NewClassNameOffset = HvAllocateCell (&DstHive->Hive,
			                           NewSubKeyCell->ClassSize,
			                           HvStable);
	      if (NewClassNameOffset == HCELL_NULL)
		{
		  DPRINT1 ("HvAllocateCell() failed (Status %lx)\n", Status);
		  return STATUS_INSUFFICIENT_RESOURCES;
		}

	      NewClassNameCell = HvGetCell (&DstHive->Hive, NewClassNameOffset);
	      NewSubKeyCell->ClassNameOffset = NewClassNameOffset;
	      RtlCopyMemory (NewClassNameCell,
			     SrcClassNameCell,
			     NewSubKeyCell->ClassSize);
	    }

	  /* Copy subkey data and subkeys */
	  Status = CmiCopyKey (DstHive,
			       NewSubKeyCell,
			       SrcHive,
			       SrcSubKeyCell);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1 ("CmiAllocateBlock() failed (Status %lx)\n", Status);
	      return Status;
	    }
	}
    }

  return STATUS_SUCCESS;
}


NTSTATUS
CmiSaveTempHive (PEREGISTRY_HIVE Hive,
		 HANDLE FileHandle)
{
  Hive->HiveHandle = FileHandle;
  return HvWriteHive(&Hive->Hive) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/* EOF */
