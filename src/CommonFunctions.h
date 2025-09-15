#pragma once

// Implemented in ContentProperties.cpp
void DisplayStringProperty(
    IPortableDeviceValues*  pProperties,
    REFPROPERTYKEY          key,
    PCWSTR                  pszKey);

// Implemented in ContentProperties.cpp
void DisplayGuidProperty(
    IPortableDeviceValues*  pProperties,
    REFPROPERTYKEY          key,
    PCWSTR                  pszKey);

// Determines if a device supports a particular functional category.
BOOL SupportsCommand(
    IPortableDevice*  pDevice,
    REFPROPERTYKEY    keyCommand);

// Helper class to convert a GUID to a string
class CGuidToString
{
private:        
    WCHAR _szGUID[64];

public:
    CGuidToString(REFGUID guid)
    {
        if (!::StringFromGUID2(guid,  _szGUID, 64))
        {
            _szGUID[0] = L'\0';
        }
    }

    operator PWSTR()
    {
        return _szGUID;
    }
};