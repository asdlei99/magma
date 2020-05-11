/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2019 Victor Coda.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "pch.h"
#pragma hdrstop
#include "resourceCache.h"

namespace magma
{
ResourceCache::ResourceCache()
{}

void ResourceCache::addResource(Buffer *buffer)
{
    const auto weakRef = std::shared_ptr<Buffer>(buffer, [](Buffer *){});
    buffers.emplace_back(std::move(weakRef));
}

void ResourceCache::addResource(Image *image)
{
    const auto weakRef = std::shared_ptr<Image>(image, [](Image *){});
    images.emplace_back(std::move(weakRef));
}
} // namespace magma
