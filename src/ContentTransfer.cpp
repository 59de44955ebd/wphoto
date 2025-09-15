#include "stdafx.h"
#include <strsafe.h>

// Reads a string property from the IPortableDeviceProperties
// interface and returns it in the form of a CAtlStringW
HRESULT GetStringValue(
    IPortableDeviceProperties* pProperties,
    PCWSTR                     pszObjectID,
    REFPROPERTYKEY             key,
    CAtlStringW&               strStringValue)
{
    CComPtr<IPortableDeviceValues>        pObjectProperties;
    CComPtr<IPortableDeviceKeyCollection> pPropertiesToRead;

    // 1) CoCreate an IPortableDeviceKeyCollection interface to hold the the property key
    // we wish to read.
    HRESULT hr = CoCreateInstance(CLSID_PortableDeviceKeyCollection,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pPropertiesToRead));

    // 2) Populate the IPortableDeviceKeyCollection with the keys we wish to read.
    // NOTE: We are not handling any special error cases here so we can proceed with
    // adding as many of the target properties as we can.
    if (SUCCEEDED(hr))
    {
        if (pPropertiesToRead != NULL)
        {
            HRESULT hrTemp = S_OK;
            hrTemp = pPropertiesToRead->Add(key);
            if (FAILED(hrTemp))
            {
                printf("! Failed to add PROPERTYKEY to IPortableDeviceKeyCollection, hr= 0x%lx\n", hrTemp);
            }
        }
    }

    // 3) Call GetValues() passing the collection of specified PROPERTYKEYs.
    if (SUCCEEDED(hr))
    {
        hr = pProperties->GetValues(pszObjectID,         // The object whose properties we are reading
                                    pPropertiesToRead,   // The properties we want to read
                                    &pObjectProperties); // Driver supplied property values for the specified object

        // The error is handled by the caller, which also displays an error message to the user.
    }

    // 4) Extract the string value from the returned property collection
    if (SUCCEEDED(hr))
    {
        PWSTR pszStringValue = NULL;
        hr = pObjectProperties->GetStringValue(key, &pszStringValue);
        if (SUCCEEDED(hr))
        {
            // Assign the newly read string to the CAtlStringW out
            // parameter.
            strStringValue = pszStringValue;
        }
        else
        {
            printf("! Failed to find property in IPortableDeviceValues, hr = 0x%lx\n",hr);
        }

        CoTaskMemFree(pszStringValue);
        pszStringValue = NULL;
    }

    return hr;
}

// Copies data from a source stream to a destination stream using the
// specified cbTransferSize as the temporary buffer size.
HRESULT StreamCopy(
    IStream*    pDestStream,
    IStream*    pSourceStream,
    DWORD       cbTransferSize,
    DWORD*      pcbWritten)
{

    HRESULT hr = S_OK;

    // Allocate a temporary buffer (of Optimal transfer size) for the read results to
    // be written to.
    BYTE*   pObjectData = new (std::nothrow) BYTE[cbTransferSize];
    if (pObjectData != NULL)
    {
        DWORD cbTotalBytesRead    = 0;
        DWORD cbTotalBytesWritten = 0;

        DWORD cbBytesRead    = 0;
        DWORD cbBytesWritten = 0;

        // Read until the number of bytes returned from the source stream is 0, or
        // an error occured during transfer.
        do
        {
            // Read object data from the source stream
            hr = pSourceStream->Read(pObjectData, cbTransferSize, &cbBytesRead);
            if (FAILED(hr))
            {
                printf("! Failed to read %d bytes from the source stream, hr = 0x%lx\n",cbTransferSize, hr);
            }

            // Write object data to the destination stream
            if (SUCCEEDED(hr))
            {
                cbTotalBytesRead += cbBytesRead; // Calculating total bytes read from device for debugging purposes only

                hr = pDestStream->Write(pObjectData, cbBytesRead, &cbBytesWritten);
                if (FAILED(hr))
                {
                    printf("! Failed to write %d bytes of object data to the destination stream, hr = 0x%lx\n",cbBytesRead, hr);
                }

                if (SUCCEEDED(hr))
                {
                    cbTotalBytesWritten += cbBytesWritten; // Calculating total bytes written to the file for debugging purposes only
                }
            }

            // Output Read/Write operation information only if we have received data and if no error has occured so far.
            //if (SUCCEEDED(hr) && (cbBytesRead > 0))
            //{
            //    printf("Read %d bytes from the source stream...Wrote %d bytes to the destination stream...\n", cbBytesRead, cbBytesWritten);
            //}

        } while (SUCCEEDED(hr) && (cbBytesRead > 0));

        // If the caller supplied a pcbWritten parameter and we
        // and we are successful, set it to cbTotalBytesWritten
        // before exiting.
        if ((SUCCEEDED(hr)) && (pcbWritten != NULL))
        {
            *pcbWritten = cbTotalBytesWritten;
        }

        // Remember to delete the temporary transfer buffer
        delete [] pObjectData;
        pObjectData = NULL;
    }
    else
    {
        printf("! Failed to allocate %d bytes for the temporary transfer buffer.\n", cbTransferSize);
    }

    return hr;
}

// Transfers a selected object's data (WPD_RESOURCE_DEFAULT) to a temporary
// file.
BOOL TransferContentFromDevice(
    IPortableDevice* pDevice,
	WCHAR* szSelection
)
{
    HRESULT                            hr                   = S_OK;
    CComPtr<IPortableDeviceContent>    pContent;
    CComPtr<IPortableDeviceResources>  pResources;
    CComPtr<IPortableDeviceProperties> pProperties;
    CComPtr<IStream>                   pObjectDataStream;
    CComPtr<IStream>                   pFinalFileStream;
    DWORD                              cbOptimalTransferSize = 0;
    CAtlStringW                        strOriginalFileName;

	BOOL ok = FALSE;

    if (pDevice == NULL)
    {
        printf("! A NULL IPortableDevice interface pointer was received\n");
        return FALSE;
    }

	// 1) get an IPortableDeviceContent interface from the IPortableDevice interface to
    // access the content-specific methods.
    if (SUCCEEDED(hr))
    {
        hr = pDevice->Content(&pContent);
        if (FAILED(hr))
        {
            printf("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx\n",hr);
        }
    }

    // 2) Get an IPortableDeviceResources interface from the IPortableDeviceContent interface to
    // access the resource-specific methods.
    if (SUCCEEDED(hr))
    {
        hr = pContent->Transfer(&pResources);
        if (FAILED(hr))
        {
            printf("! Failed to get IPortableDeviceResources from IPortableDeviceContent, hr = 0x%lx\n",hr);
        }
    }

    // 3) Get the IStream (with READ access) and the optimal transfer buffer size
    // to begin the transfer.
    if (SUCCEEDED(hr))
    {
        hr = pResources->GetStream(szSelection,             // Identifier of the object we want to transfer
                                   WPD_RESOURCE_DEFAULT,    // We are transferring the default resource (which is the entire object's data)
                                   STGM_READ,               // Opening a stream in READ mode, because we are reading data from the device.
                                   &cbOptimalTransferSize,  // Driver supplied optimal transfer size
                                   &pObjectDataStream);
        if (FAILED(hr))
        {
            printf("! Failed to get IStream (representing object data on the device) from IPortableDeviceResources, hr = 0x%lx\n",hr);
        }
    }

    // 4) Read the WPD_OBJECT_ORIGINAL_FILE_NAME property so we can properly name the
    // transferred object.  Some content objects may not have this property, so a
    // fall-back case has been provided below. (i.e. Creating a file named <objectID>.data )
    if (SUCCEEDED(hr))
    {
        hr = pContent->Properties(&pProperties);
        if (SUCCEEDED(hr))
        {
            hr = GetStringValue(pProperties,
                                szSelection,
                                WPD_OBJECT_ORIGINAL_FILE_NAME,
                                strOriginalFileName);
            if (FAILED(hr))
            {
                printf("! Failed to read WPD_OBJECT_ORIGINAL_FILE_NAME on object '%ws', hr = 0x%lx\n", szSelection, hr);
                strOriginalFileName.Format(L"%ws.data", szSelection);
                printf("* Creating a filename '%ws' as a default.\n", (PWSTR)strOriginalFileName.GetString());
                // Set the HRESULT to S_OK, so we can continue with our newly generated
                // temporary file name.
                hr = S_OK;
            }
        }
        else
        {
            printf("! Failed to get IPortableDeviceProperties from IPortableDeviceContent, hr = 0x%lx\n", hr);
        }
    }

    // 5) Create a destination for the data to be written to.  In this example we are
    // creating a temporary file which is named the same as the object identifier string.
    if (SUCCEEDED(hr))
    {
        hr = SHCreateStreamOnFile(strOriginalFileName, STGM_CREATE|STGM_WRITE, &pFinalFileStream);
        if (FAILED(hr))
        {
            printf("! Failed to create a temporary file named (%ws) to transfer object (%ws), hr = 0x%lx\n",(PWSTR)strOriginalFileName.GetString(), szSelection, hr);
        }
    }

    // 6) Read on the object's data stream and write to the final file's data stream using the
    // driver supplied optimal transfer buffer size.
    if (SUCCEEDED(hr))
    {
        DWORD cbTotalBytesWritten = 0;

        // Since we have IStream-compatible interfaces, call our helper function
        // that copies the contents of a source stream into a destination stream.
        hr = StreamCopy(pFinalFileStream,       // Destination (The Final File to transfer to)
                        pObjectDataStream,      // Source (The Object's data to transfer from)
                        cbOptimalTransferSize,  // The driver specified optimal transfer buffer size
                        &cbTotalBytesWritten);  // The total number of bytes transferred from device to the finished file
        if (FAILED(hr))
        {
            printf("! Failed to transfer object from device, hr = 0x%lx\n",hr);
        }
        else
        {
            printf("Transferred object '%ws' to '%ws'.\n", szSelection, (PWSTR)strOriginalFileName.GetString());
			ok = TRUE;
        }
    }
	
	return ok;
}


// Deletes a selected object from the device.
void DeleteContentFromDevice(
    IPortableDevice* pDevice,
	WCHAR* szSelection
)
{
    HRESULT                                       hr               = S_OK;
    //WCHAR                                         szSelection[81]  = {0};
    CComPtr<IPortableDeviceContent>               pContent;
    CComPtr<IPortableDevicePropVariantCollection> pObjectsToDelete;
    CComPtr<IPortableDevicePropVariantCollection> pObjectsFailedToDelete;

    if (pDevice == NULL)
    {
        printf("! A NULL IPortableDevice interface pointer was received\n");
        return;
    }

    // 1) get an IPortableDeviceContent interface from the IPortableDevice interface to
    // access the content-specific methods.
    if (SUCCEEDED(hr))
    {
        hr = pDevice->Content(&pContent);
        if (FAILED(hr))
        {
            printf("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx\n",hr);
        }
    }

    // 2) CoCreate an IPortableDevicePropVariantCollection interface to hold the the object identifiers
    // to delete.
    //
    // NOTE: This is a collection interface so more than 1 object can be deleted at a time.
    //       This sample only deletes a single object.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&pObjectsToDelete));
        if (SUCCEEDED(hr))
        {
            if (pObjectsToDelete != NULL)
            {
                PROPVARIANT pv = {0};
                PropVariantInit(&pv);

                // Initialize a PROPVARIANT structure with the object identifier string
                // that the user selected above. Notice we are allocating memory for the
                // PWSTR value.  This memory will be freed when PropVariantClear() is
                // called below.
                pv.vt      = VT_LPWSTR;
                pv.pwszVal = AtlAllocTaskWideString(szSelection);
                if (pv.pwszVal != NULL)
                {
                    // Add the object identifier to the objects-to-delete list
                    // (We are only deleting 1 in this example)
                    hr = pObjectsToDelete->Add(&pv);
                    if (SUCCEEDED(hr))
                    {
                        // Attempt to delete the object from the device
                        hr = pContent->Delete(PORTABLE_DEVICE_DELETE_NO_RECURSION,  // Deleting with no recursion
                                              pObjectsToDelete,                     // Object(s) to delete
                                              NULL);                                // Object(s) that failed to delete (we are only deleting 1, so we can pass NULL here)
                        if (SUCCEEDED(hr))
                        {
                            // An S_OK return lets the caller know that the deletion was successful
                            if (hr == S_OK)
                            {
                                printf("The object '%ws' was deleted from the device.\n", szSelection);
                            }

                            // An S_FALSE return lets the caller know that the deletion failed.
                            // The caller should check the returned IPortableDevicePropVariantCollection
                            // for a list of object identifiers that failed to be deleted.
                            else
                            {
                                printf("The object '%ws' failed to be deleted from the device.\n", szSelection);
                            }
                        }
                        else
                        {
                            printf("! Failed to delete an object from the device, hr = 0x%lx\n",hr);
                        }
                    }
                    else
                    {
                        printf("! Failed to delete an object from the device because we could no add the object identifier string to the IPortableDevicePropVariantCollection, hr = 0x%lx\n",hr);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    printf("! Failed to delete an object from the device because we could no allocate memory for the object identifier string, hr = 0x%lx\n",hr);
                }

                // Free any allocated values in the PROPVARIANT before exiting
                PropVariantClear(&pv);
            }
            else
            {
                printf("! Failed to delete an object from the device because we were returned a NULL IPortableDevicePropVariantCollection interface pointer, hr = 0x%lx\n",hr);
            }
        }
        else
        {
            printf("! Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection, hr = 0x%lx\n",hr);
        }
    }
}


// Moves a selected object (which is already on the device) to another location on the device.
void MoveContentAlreadyOnDevice(
    IPortableDevice* pDevice)
{
    HRESULT                                       hr                              = S_OK;
    WCHAR                                         szSelection[81]                 = {0};
    WCHAR                                         szDestinationFolderObjectID[81] = {0};
    CComPtr<IPortableDeviceContent>               pContent;
    CComPtr<IPortableDevicePropVariantCollection> pObjectsToMove;
    CComPtr<IPortableDevicePropVariantCollection> pObjectsFailedToMove;

    if (pDevice == NULL)
    {
        printf("! A NULL IPortableDevice interface pointer was received\n");
        return;
    }

    // Check if the device supports the move command needed to perform this operation
    if (SupportsCommand(pDevice, WPD_COMMAND_OBJECT_MANAGEMENT_MOVE_OBJECTS) == FALSE)
    {
        printf("! This device does not support the move operation (i.e. The WPD_COMMAND_OBJECT_MANAGEMENT_MOVE_OBJECTS command)\n");
        return;
    }

    // Prompt user to enter an object identifier on the device to move.
    printf("Enter the identifier of the object you wish to move.\n>");
    hr = StringCbGetsW(szSelection,sizeof(szSelection));
    if (FAILED(hr))
    {
        printf("An invalid object identifier was specified, aborting content moving\n");
    }

    // Prompt user to enter an object identifier on the device to move.
    printf("Enter the identifier of the object you wish to move '%ws' to.\n>", szSelection);
    hr = StringCbGetsW(szDestinationFolderObjectID,sizeof(szDestinationFolderObjectID));
    if (FAILED(hr))
    {
        printf("An invalid object identifier was specified, aborting content moving\n");
    }

    // 1) get an IPortableDeviceContent interface from the IPortableDevice interface to
    // access the content-specific methods.
    if (SUCCEEDED(hr))
    {
        hr = pDevice->Content(&pContent);
        if (FAILED(hr))
        {
            printf("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx\n",hr);
        }
    }

    // 2) CoCreate an IPortableDevicePropVariantCollection interface to hold the the object identifiers
    // to move.
    //
    // NOTE: This is a collection interface so more than 1 object can be moved at a time.
    //       This sample only moves a single object.
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_PortableDevicePropVariantCollection,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&pObjectsToMove));
        if (SUCCEEDED(hr))
        {
            if (pObjectsToMove != NULL)
            {
                PROPVARIANT pv = {0};
                PropVariantInit(&pv);

                // Initialize a PROPVARIANT structure with the object identifier string
                // that the user selected above. Notice we are allocating memory for the
                // PWSTR value.  This memory will be freed when PropVariantClear() is
                // called below.
                pv.vt      = VT_LPWSTR;
                pv.pwszVal = AtlAllocTaskWideString(szSelection);
                if (pv.pwszVal != NULL)
                {
                    // Add the object identifier to the objects-to-move list
                    // (We are only moving 1 in this example)
                    hr = pObjectsToMove->Add(&pv);
                    if (SUCCEEDED(hr))
                    {
                        // Attempt to move the object on the device
                        hr = pContent->Move(pObjectsToMove,              // Object(s) to move
                                            szDestinationFolderObjectID, // Folder to move to
                                            NULL);                       // Object(s) that failed to delete (we are only moving 1, so we can pass NULL here)
                        if (SUCCEEDED(hr))
                        {
                            // An S_OK return lets the caller know that the deletion was successful
                            if (hr == S_OK)
                            {
                                printf("The object '%ws' was moved on the device.\n", szSelection);
                            }

                            // An S_FALSE return lets the caller know that the move failed.
                            // The caller should check the returned IPortableDevicePropVariantCollection
                            // for a list of object identifiers that failed to be moved.
                            else
                            {
                                printf("The object '%ws' failed to be moved on the device.\n", szSelection);
                            }
                        }
                        else
                        {
                            printf("! Failed to move an object on the device, hr = 0x%lx\n",hr);
                        }
                    }
                    else
                    {
                        printf("! Failed to move an object on the device because we could no add the object identifier string to the IPortableDevicePropVariantCollection, hr = 0x%lx\n",hr);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    printf("! Failed to move an object on the device because we could no allocate memory for the object identifier string, hr = 0x%lx\n",hr);
                }

                // Free any allocated values in the PROPVARIANT before exiting
                PropVariantClear(&pv);
            }
            else
            {
                printf("! Failed to move an object from the device because we were returned a NULL IPortableDevicePropVariantCollection interface pointer, hr = 0x%lx\n",hr);
            }
        }
        else
        {
            printf("! Failed to CoCreateInstance CLSID_PortableDevicePropVariantCollection, hr = 0x%lx\n",hr);
        }
    }
}


HRESULT GetRequiredPropertiesForFolder(
    PCWSTR                  pszParentObjectID,
    PCWSTR                  pszFolderName,
    IPortableDeviceValues** ppObjectProperties)
{
    CComPtr<IPortableDeviceValues> pObjectProperties;

    // CoCreate an IPortableDeviceValues interface to hold the the object information
    HRESULT hr = CoCreateInstance(CLSID_PortableDeviceValues,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pObjectProperties));
    if (SUCCEEDED(hr))
    {
        if (pObjectProperties != NULL)
        {
            // Set the WPD_OBJECT_PARENT_ID
            if (SUCCEEDED(hr))
            {
                hr = pObjectProperties->SetStringValue(WPD_OBJECT_PARENT_ID, pszParentObjectID);
                if (FAILED(hr))
                {
                    printf("! Failed to set WPD_OBJECT_PARENT_ID, hr = 0x%lx\n",hr);
                }
            }

            // Set the WPD_OBJECT_NAME.
            if (SUCCEEDED(hr))
            {
                hr = pObjectProperties->SetStringValue(WPD_OBJECT_NAME, pszFolderName);
                if (FAILED(hr))
                {
                    printf("! Failed to set WPD_OBJECT_NAME, hr = 0x%lx\n",hr);
                }
            }

            // Set the WPD_OBJECT_CONTENT_TYPE to WPD_CONTENT_TYPE_FOLDER because we are
            // creating contact content on the device.
            if (SUCCEEDED(hr))
            {
                hr = pObjectProperties->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FOLDER);
                if (FAILED(hr))
                {
                    printf("! Failed to set WPD_OBJECT_CONTENT_TYPE to WPD_CONTENT_TYPE_FOLDER, hr = 0x%lx\n",hr);
                }
            }

            // If everything was successful above, QI for the IPortableDeviceValues to return
            // to the caller.  A temporary CComPtr IPortableDeviceValues was used for easy cleanup
            // in case of a failure.
            if (SUCCEEDED(hr))
            {
                hr = pObjectProperties->QueryInterface(IID_PPV_ARGS(ppObjectProperties));
                if (FAILED(hr))
                {
                    printf("! Failed to QueryInterface for IPortableDeviceValues, hr = 0x%lx\n",hr);
                }
            }
        }
        else
        {
            hr = E_UNEXPECTED;
            printf("! Failed to create property information because we were returned a NULL IPortableDeviceValues interface pointer, hr = 0x%lx\n",hr);
        }
    }

    return hr;
}


// Creates a properties-only object on the device which is
// WPD_CONTENT_TYPE_FOLDER specific.
void CreateFolderOnDevice(
    IPortableDevice*    pDevice)
{
    if (pDevice == NULL)
    {
        printf("! A NULL IPortableDevice interface pointer was received\n");
        return;
    }

    HRESULT                             hr = S_OK;
    WCHAR                               szSelection[81]        = {0};
    WCHAR                               szFolderName[81]        = {0};
    CComPtr<IPortableDeviceValues>      pFinalObjectProperties;
    CComPtr<IPortableDeviceContent>     pContent;

    // Prompt user to enter an object identifier for the parent object on the device to transfer.
    printf("Enter the identifier of the parent object which the folder will be created under.\n>");
    hr = StringCbGetsW(szSelection,sizeof(szSelection));
    if (FAILED(hr))
    {
        printf("An invalid object identifier was specified, aborting folder creation\n");
    }

    // Prompt user to enter an object identifier for the parent object on the device to transfer.
    printf("Enter the name of the the folder to create.\n>");
    hr = StringCbGetsW(szFolderName,sizeof(szFolderName));
    if (FAILED(hr))
    {
        printf("An invalid folder name was specified, aborting folder creation\n");
    }

    // 1) Get an IPortableDeviceContent interface from the IPortableDevice interface to
    // access the content-specific methods.
    if (SUCCEEDED(hr))
    {
        hr = pDevice->Content(&pContent);
        if (FAILED(hr))
        {
            printf("! Failed to get IPortableDeviceContent from IPortableDevice, hr = 0x%lx\n",hr);
        }
    }

    // 2) Get the properties that describe the object being created on the device
    if (SUCCEEDED(hr))
    {
        hr = GetRequiredPropertiesForFolder(szSelection,              // Parent to create the folder under
                                            szFolderName,             // Folder Name
                                            &pFinalObjectProperties);  // Returned properties describing the folder
        if (FAILED(hr))
        {
            printf("! Failed to get required properties needed to transfer an image file to the device, hr = 0x%lx\n", hr);
        }
    }

    // 3) Transfer the content to the device by creating a properties-only object
    if (SUCCEEDED(hr))
    {
        PWSTR pszNewlyCreatedObject = NULL;
        hr = pContent->CreateObjectWithPropertiesOnly(pFinalObjectProperties,    // Properties describing the object data
                                                      &pszNewlyCreatedObject);
        if (SUCCEEDED(hr))
        {
            printf("The folder was created on the device.\nThe newly created object's ID is '%ws'\n",pszNewlyCreatedObject);
        }

        if (FAILED(hr))
        {
            printf("! Failed to create a new folder on the device, hr = 0x%lx\n",hr);
        }

        // Free the object identifier string returned from CreateObjectWithPropertiesOnly
        CoTaskMemFree(pszNewlyCreatedObject);
        pszNewlyCreatedObject = NULL;
    }

}
