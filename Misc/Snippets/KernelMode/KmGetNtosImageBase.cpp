#include <ntddk.h>
#include <intrin.h>
#include <nt.hpp>

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegPath);
VOID Unload(PDRIVER_OBJECT DriverObject);

template <typename T>
T GetKernelExport(PCWSTR zExportName)
{
	__try
	{
		UNICODE_STRING UExportName;

		RtlInitUnicodeString(&UExportName, zExportName);

		T ExportAddress = (T)MmGetSystemRoutineAddress(&UExportName);

		if (ExportAddress)
		{
			return ExportAddress;
		}

		return T();
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

PVOID GetNtosImageBase1()
{
	__try 
	{
		PVOID NtosImageBase;
		
		PSYSTEM_MODULE_INFORMATION ModInfo;

		_ZwQuerySystemInformation ZwQuerySystemInformation;

		ZwQuerySystemInformation = GetKernelExport<_ZwQuerySystemInformation>(L"ZwQuerySystemInformation");

		ModInfo = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolZero(NonPagedPool, POOL_SIZE, POOL_TAG);

		if (ModInfo)
		{

			if (NT_SUCCESS(ZwQuerySystemInformation(SystemModuleInformation, ModInfo, POOL_SIZE, nullptr)))
			{
				NtosImageBase = ModInfo->Module[0].ImageBase;

				if (NtosImageBase)
				{
					ExFreePoolWithTag(ModInfo, POOL_TAG);
					return NtosImageBase;
				}
			}
		}

		ExFreePoolWithTag(ModInfo, POOL_TAG);
		return nullptr;
	}
	
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

PVOID GetNtosImageBase2()
{
	__try 
	{
		uintptr_t NtosImageBase;

		_RtlLookupFunctionEntry RtlLookupFunctionEntry;

		RtlLookupFunctionEntry = GetKernelExport<_RtlLookupFunctionEntry>(L"RtlLookupFunctionEntry");

		RtlLookupFunctionEntry((DWORD64)&MmCopyMemory, &NtosImageBase, nullptr);

		if (NtosImageBase)
		{
		    return (PVOID)NtosImageBase;
		}

		return nullptr;
	}
	
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}


PVOID GetNtosImageBase3()
{
	__try
	{

		PVOID NtosImageBase;

		_RtlPcToFileHeader RtlPcToFileHeader = GetKernelExport<_RtlPcToFileHeader>(L"RtlPcToFileHeader");

		RtlPcToFileHeader(&MmCopyMemory, &NtosImageBase);

		if (NtosImageBase)
		{
		    return NtosImageBase;
		}
		
		return nullptr;
	}
	
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}


PVOID GetNtosImageBase4()
{
	__try
	{
		PIDT_ENTRY IdtBase = (PIDT_ENTRY)KeGetPcr()->IdtBase;

		auto Page = (uintptr_t)IdtBase[0].InterruptServiceRoutine & ~0xfff;

		for (; Page; Page -= PAGE_SIZE)
		{

			if (*(USHORT*)Page == IMAGE_DOS_SIGNATURE)
			{

				for (auto Bytes = Page; Bytes < Page + 0x400; Bytes += 8)
				{
					if (*(ULONG64*)(Bytes) == PAGELK_PATTERN)
					{
						return (PVOID)Page;
					}
				}
			}
		}

		return nullptr;
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {}
}


PVOID GetNtosImageBase5()
{
	__try
	{
		auto Page = __readmsr(IA32_LSTAR) & ~0xfff;

		for (; Page; Page -= PAGE_SIZE)
		{

			if (*(USHORT*)Page == IMAGE_DOS_SIGNATURE)
			{

				for (auto Bytes = Page; Bytes < Page + 0x400; Bytes += 8)
				{
					if (*(ULONG64*)(Bytes) == PAGELK_PATTERN)
					{
						return (PVOID)Page;
					}
				}
			}
		}

		return nullptr;
	}

	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

PVOID GetNtosImageBase6()
{

	PKLDR_DATA_TABLE_ENTRY NtosKldr;

	PLIST_ENTRY PsLoadedModuleList;

	PsLoadedModuleList = GetKernelExport<PLIST_ENTRY>(L"PsLoadedModuleList");

	NtosKldr = (PKLDR_DATA_TABLE_ENTRY)PsLoadedModuleList->Flink;

	return NtosKldr ? NtosKldr->DllBase : nullptr;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegPath)
{
	UNREFERENCED_PARAMETER(RegPath);
	DbgPrint("ntoskrnl.exe base address: [%p]", GetNtosImageBase1());
	DbgPrint("ntoskrnl.exe base address: [%p]", GetNtosImageBase2());
	DbgPrint("ntoskrnl.exe base address: [%p]", GetNtosImageBase3());
	DbgPrint("ntoskrnl.exe base address: [%p]", GetNtosImageBase4());
	DbgPrint("ntoskrnl.exe base address: [%p]", GetNtosImageBase5());
	DbgPrint("ntoskrnl.exe base address: [%p]", GetNtosImageBase6());
	DriverObject->DriverUnload = Unload;
	return STATUS_SUCCESS;
}

VOID Unload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("Driver Unloaded");
}
