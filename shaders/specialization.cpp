/*
Magma - abstraction layer to facilitate usage of Khronos Vulkan API.
Copyright (C) 2018-2022 Victor Coda.

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
#include "specialization.h"

namespace magma
{
Specialization::Specialization(const Specialization& other) noexcept:
    VkSpecializationInfo{
        other.mapEntryCount,
        core::copyArray(other.pMapEntries, mapEntryCount),
        other.dataSize,
        core::copyBinaryData(other.pData, dataSize)
    }
{}

Specialization::Specialization(Specialization&& other) noexcept:
    VkSpecializationInfo{
        other.mapEntryCount,
        other.pMapEntries,
        other.dataSize,
        other.pData
    }
{
    other.mapEntryCount = 0;
    pMapEntries = nullptr;
    dataSize = 0;
    pData = nullptr;
}

Specialization& Specialization::operator=(const Specialization& other) noexcept
{
    if (this != &other)
    {
        mapEntryCount = other.mapEntryCount;
        delete[] pMapEntries;
        pMapEntries = core::copyArray(other.pMapEntries, other.mapEntryCount);
        dataSize = other.dataSize;
        delete[] reinterpret_cast<const char *>(pData);
        pData = core::copyBinaryData(other.pData, other.dataSize);
    }
    return *this;
}

Specialization::~Specialization()
{
    delete[] pMapEntries;
    delete[] reinterpret_cast<const char *>(pData);
}

hash_t Specialization::getHash() const noexcept
{
    hash_t hash = 0;
    for (uint32_t i = 0; i < mapEntryCount; ++i)
    {
        const VkSpecializationMapEntry& mapEntry = pMapEntries[i];
        hash = core::hashCombine(hash, core::hashArgs(
            mapEntry.constantID,
            mapEntry.offset,
            mapEntry.size));
    }
    const uint8_t *byteData = reinterpret_cast<const uint8_t *>(pData);
    return core::hashCombine(hash, core::hashArray(
        byteData, dataSize));
}
} // namespace magma
