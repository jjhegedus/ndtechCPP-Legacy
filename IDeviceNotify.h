#pragma once
namespace ndtech
{
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    struct IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };
}