#include "stdafx.h"


extern ULONG g_TargetValue;
extern wchar_t g_ObjID[MAX_PATH];


CAtlStringW g_strEventRegistrationCookie;

// IPortableDeviceEventCallback implementation for use with
// device events.
class CPortableDeviceEventsCallback : public IPortableDeviceEventCallback
{
public:
    CPortableDeviceEventsCallback() : m_cRef(1)
    {
    }

    ~CPortableDeviceEventsCallback()
    {
    }

    HRESULT __stdcall QueryInterface(
        REFIID  riid,
        LPVOID* ppvObj)
    {
        HRESULT hr = S_OK;
        if (ppvObj == NULL)
        {
            hr = E_INVALIDARG;
            return hr;
        }

        if ((riid == IID_IUnknown) ||
            (riid == IID_IPortableDeviceEventCallback))
        {
            AddRef();
            *ppvObj = this;
        }
        else
        {
            *ppvObj = NULL;
            hr = E_NOINTERFACE;
        }
        return hr;
    }

    ULONG __stdcall AddRef()
    {
        InterlockedIncrement((long*) &m_cRef);
        return m_cRef;
    }

    ULONG __stdcall Release()
    {
        ULONG ulRefCount = m_cRef - 1;

        if (InterlockedDecrement((long*) &m_cRef) == 0)
        {
            delete this;
            return 0;
        }
        return ulRefCount;
    }

    HRESULT __stdcall OnEvent(
        IPortableDeviceValues* pEventParameters)
    {
        if (pEventParameters != NULL)
        {
            //printf("***************************\n** Device event received **\n***************************\n");
            //DisplayStringProperty(pEventParameters, WPD_EVENT_PARAMETER_PNP_DEVICE_ID, L"WPD_EVENT_PARAMETER_PNP_DEVICE_ID");
            //DisplayGuidProperty(pEventParameters, WPD_EVENT_PARAMETER_EVENT_ID, L"WPD_EVENT_PARAMETER_EVENT_ID");

			PWSTR pszValue = NULL;


			//printf("WPD_OBJECT_ORIGINAL_FILE_NAME ----------------------------\n");
			//HRESULT hr = pEventParameters->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &pszValue);
			//if (SUCCEEDED(hr))
			//{
			//	// Get the length of the string value so we
			//	// can output <empty string value> if one
			//	// is encountered.
			//	CAtlStringW strValue;
			//	strValue = pszValue;
			//	if (strValue.GetLength() > 0)
			//	{
			//		printf("%ws\n", pszValue);
			//	}
			//}


			//printf("WPD_OBJECT_NAME ----------------------------\n");
			//pszValue = NULL;
			//hr = pEventParameters->GetStringValue(WPD_OBJECT_NAME, &pszValue);
			//if (SUCCEEDED(hr))
			//{
			//	// Get the length of the string value so we
			//	// can output <empty string value> if one
			//	// is encountered.
			//	CAtlStringW strValue;
			//	strValue = pszValue;
			//	if (strValue.GetLength() > 0)
			//	{
			//		printf("%ws\n", pszValue);
			//	}
			//}

			pszValue = NULL;
			if (SUCCEEDED(pEventParameters->GetStringValue(WPD_OBJECT_ID, &pszValue)))
			{
				// Get the length of the string value so we
				// can output <empty string value> if one
				// is encountered.
				CAtlStringW strValue;
				strValue = pszValue;
				if (strValue.GetLength() > 0)
				{
					wcscpy_s(g_ObjID, MAX_PATH, pszValue);

					g_TargetValue = 1;
					WakeByAddressAll(&g_TargetValue);
				}
			}
        }

        return S_OK;
    }

    ULONG m_cRef;
};


void RegisterForEventNotifications(IPortableDevice* pDevice)
{
    HRESULT                        hr = S_OK;
    PWSTR                          pszEventCookie = NULL;
    CPortableDeviceEventsCallback* pCallback = NULL;

    if (pDevice == NULL)
    {
        return;
    }

    // Check to see if we already have an event registration cookie.  If so,
    // then avoid registering again.
    // NOTE: An application can register for events as many times as they want.
    //       This sample only keeps a single registration cookie around for
    //       simplicity.
    if (g_strEventRegistrationCookie.GetLength() > 0)
    {
        printf("This application has already registered to receive device events.\n");
        return;
    }

    // Create an instance of the callback object.  This will be called when events
    // are received.
    if (hr == S_OK)
    {
        pCallback = new (std::nothrow) CPortableDeviceEventsCallback();
        if (pCallback == NULL)
        {
            hr = E_OUTOFMEMORY;
            printf("Failed to allocate memory for IPortableDeviceEventsCallback object, hr = 0x%lx\n",hr);
        }
    }

    // Call Advise to register the callback and receive events.
    if (hr == S_OK)
    {
        hr = pDevice->Advise(0, pCallback, NULL, &pszEventCookie);
        if (FAILED(hr))
        {
            printf("! Failed to register for device events, hr = 0x%lx\n",hr);
        }
    }

    // Save the event registration cookie if event registration was successful.
    if (hr == S_OK)
    {
        g_strEventRegistrationCookie = pszEventCookie;
    }

    // Free the event registration cookie, if one was returned.
    if (pszEventCookie != NULL)
    {
        CoTaskMemFree(pszEventCookie);
        pszEventCookie = NULL;
    }

    //if (hr == S_OK)
    //{
    //    printf("This application has registered for device event notifications and was returned the registration cookie '%ws'\n", g_strEventRegistrationCookie.GetString());
    //}

    // If a failure occurs, remember to delete the allocated callback object, if one exists.
    if (pCallback != NULL)
    {
        pCallback->Release();
    }
}


void UnregisterForEventNotifications(IPortableDevice* pDevice)
{
    HRESULT hr = S_OK;

    if (pDevice == NULL)
    {
        return;
    }

    hr = pDevice->Unadvise(g_strEventRegistrationCookie);
    if (FAILED(hr))
    {
        printf("! Failed to unregister for device events using registration cookie '%ws', hr = 0x%lx\n",g_strEventRegistrationCookie.GetString(), hr);
    }

    //if (hr == S_OK)
    //{
    //    printf("This application used the registration cookie '%ws' to unregister from receiving device event notifications", g_strEventRegistrationCookie.GetString());
    //}

    g_strEventRegistrationCookie = L"";
}
