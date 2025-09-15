## wphoto - poor man's native gphoto2 for Windows

`wphoto` is a *very simple* command line tool for Windows for a single purpose: to programmatically take a picture with a digital camera connected to a Windows PC via USB, and optionally directly download (and delete) the image file from the camera's storage.

It's based on the `Windows Portable Device (WPD)` API, and only supports camera models that provide the `WPD_FUNCTIONAL_CATEGORY_STILL_IMAGE_CAPTURE` function.

I only tested it so far with an about 15 years old Nikon COOLPIX camera in Windows 11. I have no idea if e.g. recent Canon cameras support `WPD_FUNCTIONAL_CATEGORY_STILL_IMAGE_CAPTURE` or not.


## Interface

```
Usage: wphoto [--list-devices] [--device=DEVICE] [--list-features] [--list-files]
        [--download-file=FILE] [--delete-file=FILE] [--download-and-delete-file=FILE]
        [--capture-image] [--capture-image-and-download]
``` 

## Example session (in a CMD shell)

List the currently attached WPD devices:
```
$ wphoto --list-devices

2 Windows Portable Device(s) found on the system

S5200
    Manufacturer:  Nikon Corporation
    Description:   S5200

ALCATEL 3C
    Manufacturer:  TCL
    Description:   ALCATEL 3C
```
Let's check the Nikon camera's features:
```
$ wphoto --device=S5200 --list-features

2 Functional Categories Found on the device

WPD_FUNCTIONAL_CATEGORY_STORAGE
WPD_FUNCTIONAL_CATEGORY_STILL_IMAGE_CAPTURE
```
YES, it supports `WPD_FUNCTIONAL_CATEGORY_STILL_IMAGE_CAPTURE`!  
  
So let's take a picture, and download it immediately to the current working directory on the PC. If the download was successfull, the image will be deleted from the camera's storage after the download.
```
$ wphoto --device=S5200 --capture-image-and-download

Transferred object 'oC' to 'DSCN6616.JPG'.
The object 'oC' was deleted from the device.
```
We successfully took a picture, and now have a file called 'DSCN6616.JPG' in the current working directory on the PC.

## Notes

- Command line arguments are evaluated from left to right, so any command other than `--list-devices` will only work if it is preceded by a `--device=<YOUR ACTUAL DEVICE>` argument, where `<YOUR ACTUAL DEVICE>` is the device's "friendly name", as reported by the `--list-devices`  command.

- If execution was successfull, the exit code (%ERRORLEVEL% in CMD) is 0, otherwise it's 23.

- The file-based commands `--list-files`, `--download-file=FILE`, `--delete-file=FILE` and `--download-and-delete-file=FILE` don't show/use real filenames (like the 'DSCN6616.JPG' in the sample session above), but instead unique internal file IDs.

- `wphoto` is meant for digital cameras, not for smartphones. But in particular, don't try to run `--list-files` with a smartphone. This command tries to list the complete public storage (directories and files), which can be a lot of stuff in case of a smartphone. The scenario it's actually meant for is a digital camera with a flat storage, containing hardly anything. I guess, in those situations where you want to take pictures programmatically (and then delete them from camera storage), it makes sense to have an almost empty storage on the camera in the first place.

## Compiling

Compiling `wphoto` requires MS Visual Studio 2017 or later.
