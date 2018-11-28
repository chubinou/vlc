/*****************************************************************************
 * modules/misc/medialibrary/sd/fs.cpp
 *****************************************************************************
 * Copyright (C) 2018 VLC authors, VideoLAN and VideoLabs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "fs.h"

#include <algorithm>
#include <vlc_services_discovery.h>
#include <medialibrary/IDeviceLister.h>
#include <medialibrary/filesystem/IDevice.h>

#include "device.h"
#include "directory.h"
#include "util.h"

extern "C" {

static void
services_discovery_item_added(services_discovery_t *sd,
                              input_item_t *parent, input_item_t *media,
                              const char *cat)
{
    VLC_UNUSED(parent);
    VLC_UNUSED(cat);
    vlc::medialibrary::SDFileSystemFactory *that =
        static_cast<vlc::medialibrary::SDFileSystemFactory *>(sd->owner.sys);
    that->onDeviceAdded(media);
}

static void
services_discovery_item_removed(services_discovery_t *sd, input_item_t *media)
{
    vlc::medialibrary::SDFileSystemFactory *that =
        static_cast<vlc::medialibrary::SDFileSystemFactory *>(sd->owner.sys);
    that->onDeviceRemoved(media);
}

static const struct services_discovery_callbacks sd_cbs = {
    .item_added = services_discovery_item_added,
    .item_removed = services_discovery_item_removed,
};

}

namespace vlc {
  namespace medialibrary {

using namespace ::medialibrary;

SDFileSystemFactory::SDFileSystemFactory(vlc_object_t *parent,
                                         const std::string &scheme)
    : parent(parent)
    , m_scheme(scheme)
{
}

std::shared_ptr<IDirectory>
SDFileSystemFactory::createDirectory(const std::string &mrl)
{
    return std::make_shared<SDDirectory>(mrl, *this);
}

std::shared_ptr<IDevice>
SDFileSystemFactory::createDevice(const std::string &uuid)
{
    vlc::threads::mutex_locker locker(mutex);

    vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(5);
    while ( true )
    {
        auto it = std::find_if(devices.cbegin(), devices.cend(),
                [&uuid](const Device& device) {
                    return strcasecmp( uuid.c_str(), device.device->uuid().c_str() ) == 0;
                });
        if (it != devices.cend())
            return (*it).device;
        /* wait a bit, maybe the device is not detected yet */
        int timeout = itemAddedCond.timedwait(mutex, deadline);
        if (timeout)
            return nullptr;
    }
    vlc_assert_unreachable();
}

std::shared_ptr<IDevice>
SDFileSystemFactory::createDeviceFromMrl(const std::string &mrl)
{
    vlc::threads::mutex_locker locker(mutex);

    vlc_tick_t deadline = vlc_tick_now() + VLC_TICK_FROM_SEC(5);
    while ( true )
    {
        auto it = std::find_if(devices.cbegin(), devices.cend(),
                [&mrl](const Device& device) {
                    return std::find_if( cbegin( device.mrls ), cend( device.mrls ),
                            [&mrl]( const std::string& deviceMrl ) {
                                return strncasecmp( mrl.c_str(), deviceMrl.c_str(),
                                                    deviceMrl.length() ) == 0;
                            }) != cend( device.mrls );
                });
        if (it != devices.cend())
            return (*it).device;
        /* wait a bit, maybe the device is not detected yet */
        int timeout = itemAddedCond.timedwait(mutex, deadline);
        if (timeout)
            return nullptr;
    }
    vlc_assert_unreachable();
}

void
SDFileSystemFactory::refreshDevices()
{
    /* nothing to do */
}

bool
SDFileSystemFactory::isMrlSupported(const std::string &path) const
{
    auto s = m_scheme + ":";
    return !path.compare(0, s.length(), s);
}

bool
SDFileSystemFactory::isNetworkFileSystem() const
{
    return true;
}

const std::string &
SDFileSystemFactory::scheme() const
{
    return m_scheme;
}

bool
SDFileSystemFactory::start(IFileSystemFactoryCb *callbacks)
{
    this->callbacks = callbacks;
    struct services_discovery_owner_t owner = {
        .cbs = &sd_cbs,
        .sys = this,
    };
    char** sdLongNames;
    int* categories;
    auto releaser = [](char** ptr) {
        for ( auto i = 0u; ptr[i] != nullptr; ++i )
            free( ptr[i] );
        free( ptr );
    };
    auto sdNames = vlc_sd_GetNames( libvlc(), &sdLongNames, &categories );
    if ( sdNames == nullptr )
        return false;
    auto sdNamesPtr = vlc::wrap_carray( sdNames, releaser );
    auto sdLongNamesPtr = vlc::wrap_carray( sdLongNames, releaser );
    auto categoriesPtr = vlc::wrap_carray( categories );
    for ( auto i = 0u; sdNames[i] != nullptr; ++i )
    {
        if ( categories[i] != SD_CAT_LAN )
            continue;
        SdPtr sd{ vlc_sd_Create( libvlc(), sdNames[i], &owner ), &vlc_sd_Destroy };
        if ( sd == nullptr )
            continue;
        m_sds.push_back( std::move( sd ) );
    }
    return m_sds.empty() == false;
}

void
SDFileSystemFactory::stop()
{
    m_sds.clear();
    callbacks = nullptr;
}

libvlc_int_t *
SDFileSystemFactory::libvlc() const
{
    return parent->obj.libvlc;
}

void
SDFileSystemFactory::onDeviceAdded(input_item_t *media)
{
    auto mrl = std::string{ media->psz_uri };
    auto name = media->psz_name;
    if ( *mrl.crbegin() != '/' )
        mrl += '/';

    if ( strncasecmp( mrl.c_str(), m_scheme.c_str(), m_scheme.length() ) != 0 ||
         mrl[m_scheme.length()] != ':' )
        return;

    {
        vlc::threads::mutex_locker locker(mutex);
        auto it = std::find_if(devices.begin(), devices.end(),
                [name](const Device& device) {
                    return strcasecmp( name, device.device->uuid().c_str() ) == 0;
                });
        if (it != devices.end())
        {
            auto& device = (*it);
            auto mrlIt = std::find_if( cbegin( device.mrls ), cend( device.mrls ),
                                       [mrl]( const std::string& deviceMrl ) {
                                            return strcasecmp( deviceMrl.c_str(),
                                                               mrl.c_str() ) == 0;
                                       });
            if ( mrlIt == cend( device.mrls ) )
                device.mrls.push_back( std::move( mrl ) );
            device.device->setPresent( true );
            return; /* already exists */
        }
        Device device;
        device.device = std::make_shared<SDDevice>( name, mrl );
        device.mrls.push_back( std::move( mrl ) );
        devices.push_back( device );
    }

    itemAddedCond.signal();
    callbacks->onDevicePlugged( name );
}

void
SDFileSystemFactory::onDeviceRemoved(input_item_t *media)
{
    auto name = media->psz_name;

    {
        vlc::threads::mutex_locker locker(mutex);
        auto it = std::find_if(devices.cbegin(), devices.cend(),
                [&name](const Device& device) {
                    return strcasecmp( name, device.device->uuid().c_str() ) == 0;
                });
        if ( it != devices.cend() )
            (*it).device->setPresent( false );
    }

    callbacks->onDeviceUnplugged( name );
}

  } /* namespace medialibrary */
} /* namespace vlc */
