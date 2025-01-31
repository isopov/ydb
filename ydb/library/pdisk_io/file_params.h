#pragma once

#include <ydb/core/blobstorage/pdisk/blobstorage_pdisk_drivedata.h>

#include <util/folder/path.h>
#include <optional>

namespace NKikimr {

void DetectFileParameters(TString path, ui64 &outDiskSizeBytes, bool &outIsBlockDevice);

std::optional<NPDisk::TDriveData> FindDeviceBySerialNumber(const TString& serial, bool partlabelOnly);

TVector<NPDisk::TDriveData> ListDevicesWithPartlabel();

TVector<NPDisk::TDriveData> ListAllDevices();

} // NKikimr
