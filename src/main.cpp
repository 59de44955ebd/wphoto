#include "stdafx.h"
#include <strsafe.h>

// Globals
ULONG g_TargetValue = 0;
wchar_t g_ObjID[MAX_PATH] = L"";

// Device enumeration
DWORD EnumerateAllDevices();
int FindDevice(wchar_t* friendlyName, DWORD* cPnPDeviceIDs);
BOOL SelectDevice(IPortableDevice** ppDevice, DWORD cPnPDeviceIDs, UINT uiCurrentDevice);

// Device capabilities
BOOL ListFunctionalCategories(IPortableDevice* pDevice);

// Content enumeration
BOOL EnumerateAllContent(IPortableDevice* pDevice);
BOOL TakePicture(IPortableDevice* pDevice);

// Content transfer
BOOL TransferContentFromDevice(IPortableDevice* pDevice, WCHAR* szSelection);

// Content deletion
BOOL DeleteContentFromDevice(IPortableDevice* pDevice, WCHAR* szSelection);

// Device events
//void ListSupportedEvents(IPortableDevice* pDevice);
void RegisterForEventNotifications(IPortableDevice* pDevice);
void UnregisterForEventNotifications(IPortableDevice* pDevice);

int _cdecl wmain(int argc, wchar_t* argv[])
{
	if (argc < 2)
	{
		printf(
			"Usage: wphoto [--list-devices] [--device=DEVICE] [--list-features] [--list-files]\n"
			"        [--download-file=FILE] [--delete-file=FILE] [--download-and-delete-file=FILE]\n"
			"        [--capture-image] [--capture-image-and-download]"
			"\n"
		);
		return 0;
	}

	int res = 23; // For now only a single generic error code

    // Enable the heap manager to terminate the process on heap error.
    (void)HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    // Initialize COM for COINIT_MULTITHREADED
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
		CComPtr<IPortableDevice> pIPortableDevice = NULL;
		DWORD cPnPDeviceIDs = 0;
		int i;
		wchar_t *key, *value;

		for( i = 1; i < argc; i++ )
		{
			key = argv[i];
			value = wcschr(key, L'=');

			if (wcscmp(key, L"--list-devices") == 0)
			{
				EnumerateAllDevices();
				res = 0;
				break;
			}

			if (value != NULL)
			{
				*value++ = 0;

				if (wcscmp(argv[i], L"--device") == 0 && value)
				{
					int device_id = FindDevice(value, &cPnPDeviceIDs);
					if (device_id >= 0)
					{
						if (!SelectDevice(&pIPortableDevice, cPnPDeviceIDs, (UINT)device_id))
							break;
					}
				}
				if (wcscmp(argv[i], L"--download-file") == 0 && value)
				{
					if (pIPortableDevice != NULL)
					{
						if (TransferContentFromDevice(pIPortableDevice, value))
							res = 0;
					}
				}
				if (wcscmp(argv[i], L"--delete-file") == 0 && value)
				{
					if (pIPortableDevice != NULL)
					{
						if (DeleteContentFromDevice(pIPortableDevice, value))
							res = 0;
					}
				}
				if (wcscmp(argv[i], L"--download-and-delete-file") == 0 && value)
				{
					if (pIPortableDevice != NULL)
					{
						if (TransferContentFromDevice(pIPortableDevice, value))
						{
							if (DeleteContentFromDevice(pIPortableDevice, value))
								res = 0;
						}
					}
				}
			}
			else if (wcscmp(argv[i], L"--list-features") == 0)
			{
				if (pIPortableDevice != NULL)
				{
					if(ListFunctionalCategories(pIPortableDevice))
						res = 0;
				}
			}
			else if (wcscmp(argv[i], L"--capture-image") == 0)
			{
				if (pIPortableDevice != NULL)
				{
					if (TakePicture(pIPortableDevice))
						res = 0;
				}
			}

			else if (wcscmp(argv[i], L"--capture-image-and-download") == 0)
			{
				if (pIPortableDevice != NULL)
				{
					RegisterForEventNotifications(pIPortableDevice);

					if (TakePicture(pIPortableDevice))
					{
						ULONG CapturedValue;
						ULONG UndesiredValue;

						UndesiredValue = 0;
						CapturedValue = g_TargetValue;
						while (CapturedValue == UndesiredValue)
						{
							WaitOnAddress(&g_TargetValue, &UndesiredValue, sizeof(ULONG), 10000);
							CapturedValue = g_TargetValue;
						}

						UnregisterForEventNotifications(pIPortableDevice);

						if (g_ObjID[0] && TransferContentFromDevice(pIPortableDevice, g_ObjID))
						{
							if (DeleteContentFromDevice(pIPortableDevice, g_ObjID))
								res = 0;
						}
					}
				}
			}

			else if (wcscmp(argv[i], L"--list-files") == 0)
			{
				if (pIPortableDevice != NULL)
				{
					if (EnumerateAllContent(pIPortableDevice))
						res = 0;
				}
			}
		}

        // Uninitialize COM
        CoUninitialize();
    }

    return 1;
}
