// Copyright 2019 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "omaha/goopdate/dm_storage.h"

#include <set>

#include "omaha/base/const_utils.h"
#include "omaha/base/debug.h"
#include "omaha/base/file.h"
#include "omaha/base/logging.h"
#include "omaha/base/path.h"
#include "omaha/base/reg_key.h"
#include "omaha/base/safe_format.h"
#include "omaha/base/string.h"
#include "omaha/base/utils.h"
#include "omaha/common/app_registry_utils.h"
#include "omaha/common/config_manager.h"
#include "omaha/common/const_goopdate.h"
#include "omaha/common/const_group_policy.h"

namespace omaha {

namespace {

// Returns an enrollment token stored in Omaha's ClientState key, or an empty
// string if not present or in case of failure.
CString LoadEnrollmentTokenFromInstall() {
  CString value;
  HRESULT hr = RegKey::GetValue(
      app_registry_utils::GetAppClientStateKey(true /* is_machine */,
                                               kGoogleUpdateAppId),
      kRegValueCloudManagementEnrollmentToken,
      &value);
  return SUCCEEDED(hr) ? value : CString();
}

// Returns an enrollment token provisioned to the computer via Group Policy, or
// an empty string if not set or in case of failure.
CString LoadEnrollmentTokenFromCompanyPolicy() {
  return ConfigManager::Instance()->GetCloudManagementEnrollmentToken();
}

#if defined(HAS_LEGACY_DM_CLIENT)

// Returns an enrollment token provisioned to the computer via Group Policy for
// an install of Google Chrome, or an empty string if not set or in case of
// failure.
CString LoadEnrollmentTokenFromLegacyPolicy() {
  CString value;
  HRESULT hr = RegKey::GetValue(kRegKeyLegacyGroupPolicy,
                                kRegValueCloudManagementEnrollmentTokenPolicy,
                                &value);
  return SUCCEEDED(hr) ? value : CString();
}

// Returns an enrollment token provisioned to the computer via Group Policy for
// an install of Google Chrome in a deprecated location used by old versions of
// Chrome, or an empty string if not set or in case of failure.
CString LoadEnrollmentTokenFromOldLegacyPolicy() {
  CString value;
  HRESULT hr = RegKey::GetValue(
      kRegKeyLegacyGroupPolicy,
      kRegValueMachineLevelUserCloudPolicyEnrollmentToken,
      &value);
  return SUCCEEDED(hr) ? value : CString();
}

#endif  // defined(HAS_LEGACY_DM_CLIENT)

// Returns the device management token found in the registry key |path|, or an
// empty string if not set or in case of failure.
CStringA LoadDmTokenFromKey(const TCHAR* path) {
  ASSERT1(path);
  RegKey key;
  HRESULT hr = key.Open(path, KEY_QUERY_VALUE);
  if (FAILED(hr)) {
    return CStringA();
  }

  byte* value = NULL;
  size_t byte_count = 0;
  DWORD type = REG_NONE;
  hr = key.GetValue(kRegValueDmToken, &value, &byte_count, &type);
  std::unique_ptr<byte[]> safe_value(value);
  if (FAILED(hr) || type != REG_BINARY || byte_count == 0 ||
      byte_count > 4096 /* kMaxDMTokenLength */ ) {
    return CStringA();
  }
  return CStringA(reinterpret_cast<char*>(value), static_cast<int>(byte_count));
}

// Stores |dm_token| in the registry key |path|.
HRESULT StoreDmTokenInKey(const CStringA& dm_token, const TCHAR* path) {
  ASSERT1(path);
  RegKey key;
  HRESULT hr = key.Create(path, NULL /* reg_class */,
                          REG_OPTION_NON_VOLATILE /* options */,
                          KEY_SET_VALUE);
  if (FAILED(hr)) {
    return hr;
  }

  hr = key.SetValue(kRegValueDmToken,
                    reinterpret_cast<const byte*>(dm_token.GetString()),
                    dm_token.GetLength(), REG_BINARY);
  return hr;
}

HRESULT DeleteObsoletePolicies(const CPath& policy_responses_dir,
                               const std::set<CString>& policy_types_base64) {
  std::vector<CString> files;
  HRESULT hr = FindFiles(policy_responses_dir, _T("*"), &files);
  if (FAILED(hr)) {
    return hr;
  }

  for (const auto& file : files) {
    if (file == _T(".") ||
        file == _T("..") ||
        file == kCachedPublicKeyFileName ||
        policy_types_base64.count(file)) {
      continue;
    }

    CPath path = policy_responses_dir;
    VERIFY1(path.Append(file));
    REPORT_LOG(L1, (_T("[DeleteObsoletePolicies][Deleting][%s]"), path));
    VERIFY1(SUCCEEDED(DeleteBeforeOrAfterReboot(path)));
  }

  return S_OK;
}

HRESULT WriteToFile(const CPath& filename, const char* buf, const size_t len) {
  ASSERT1(buf);
  ASSERT1(len);

  File file;
  HRESULT hr = file.Open(filename, true, false);
  if (FAILED(hr)) {
    REPORT_LOG(LW, (_T("[WriteToFile][Failed Open][%s][%#x]"), filename, hr));
    return hr;
  }

  uint32_t bytes_written = 0;
  hr = file.WriteAt(0,
                    reinterpret_cast<const byte*>(buf),
                    static_cast<uint32_t>(len),
                    0,
                    &bytes_written);
  if (FAILED(hr)) {
    REPORT_LOG(LW, (_T("[WriteToFile][Failed Write][%s][%#x]"), filename, hr));
    return hr;
  }

  ASSERT1(bytes_written == len);
  return file.SetLength(bytes_written, false);
}

}  // namespace

DmStorage* DmStorage::instance_ = NULL;

// There should not be any contention on creation because only GoopdateImpl
// should create DmStorage during its initialization.
HRESULT DmStorage::CreateInstance(const CString& enrollment_token) {
  ASSERT1(!instance_);

  DmStorage* instance = new DmStorage(enrollment_token);
  if (!instance) {
    return E_OUTOFMEMORY;
  }

  instance_ = instance;
  return S_OK;
}

void DmStorage::DeleteInstance() {
  delete instance_;
  instance_ = NULL;
}

DmStorage* DmStorage::Instance() {
  ASSERT1(instance_);
  return instance_;
}

CString DmStorage::GetEnrollmentToken() {
  if (enrollment_token_source_ == kETokenSourceNone) {
    LoadEnrollmentTokenFromStorage();
  }
  ASSERT1((enrollment_token_source_ == kETokenSourceNone) ==
          enrollment_token_.IsEmpty());
  return enrollment_token_;
}

HRESULT DmStorage::StoreRuntimeEnrollmentTokenForInstall() {
  if (enrollment_token_source_ != kETokenSourceRuntime) {
    return S_FALSE;
  }
  HRESULT hr = RegKey::SetValue(
      app_registry_utils::GetAppClientStateKey(true /* is_machine */,
                                               kGoogleUpdateAppId),
      kRegValueCloudManagementEnrollmentToken,
      enrollment_token_);
  if (FAILED(hr)) {
    OPT_LOG(LE, (_T("[StoreRuntimeEnrollmentTokenForInstall failed][%#x]"),
                 hr));
  }
  return hr;
}

CStringA DmStorage::GetDmToken() {
  if (dm_token_source_ == kDmTokenSourceNone) {
    LoadDmTokenFromStorage();
  }
  ASSERT1((dm_token_source_ == kDmTokenSourceNone) == dm_token_.IsEmpty());
  return dm_token_;
}

HRESULT DmStorage::StoreDmToken(const CStringA& dm_token) {
  HRESULT hr = StoreDmTokenInKey(dm_token, kRegKeyCompanyEnrollment);
  if (SUCCEEDED(hr)) {
    dm_token_ = dm_token;
    dm_token_source_ = kDmTokenSourceCompany;
#if defined(HAS_LEGACY_DM_CLIENT)
    hr = StoreDmTokenInKey(dm_token, kRegKeyLegacyEnrollment);
#endif
  }
  return hr;
}

CString DmStorage::GetDeviceId() {
  if (device_id_.IsEmpty()) {
    LoadDeviceIdFromStorage();
  }
  return device_id_;
}

HRESULT DmStorage::PersistPolicies(const CPath& policy_responses_dir,
                                   const PolicyResponses& responses) {
  std::set<CString> policy_types_base64;
  bool is_key_file_initialized = false;

  for (const auto& response : responses.responses) {
    CStringA encoded_policy_response_dirname;
    Base64Escape(response.first.c_str(),
                 static_cast<int>(response.first.length()),
                 &encoded_policy_response_dirname,
                 true);

    CString dirname(encoded_policy_response_dirname);
    policy_types_base64.emplace(dirname);
    CPath policy_response_dir(policy_responses_dir);
    policy_response_dir.Append(dirname);
    HRESULT hr = CreateDir(policy_response_dir, NULL);
    if (FAILED(hr)) {
      REPORT_LOG(LW, (_T("[PersistPolicies][Failed to create dir][%s][%#x]"),
                      policy_response_dir, hr));
      continue;
    }

    CPath policy_response_file(policy_response_dir);
    policy_response_file.Append(kPolicyResponseFileName);

    const char* policy_fetch_response = response.second.c_str();
    const size_t len = response.second.length();
    hr = WriteToFile(policy_response_file, policy_fetch_response, len);
    if (FAILED(hr)) {
      REPORT_LOG(LW, (_T("[PersistPolicies][WriteToFile failed][%s][%#x]"),
                      policy_response_file, hr));
      continue;
    }

    if (responses.has_new_public_key && !is_key_file_initialized) {
      CPath public_key_file(policy_responses_dir);
      public_key_file.Append(kCachedPublicKeyFileName);
      hr = WriteToFile(public_key_file, policy_fetch_response, len);
      if (FAILED(hr)) {
        REPORT_LOG(LW, (_T("[PersistPolicies][WriteToFile failed][%s][%#x]"),
                        public_key_file, hr));
        continue;
      }

      is_key_file_initialized = true;
    }
  }

  VERIFY1(SUCCEEDED(DeleteObsoletePolicies(policy_responses_dir,
                                           policy_types_base64)));
  return S_OK;
}

HRESULT DmStorage::ReadCachedPublicKeyFile(const CPath& policy_responses_dir,
                                           CachedPublicKey* key) {
  ASSERT1(key);

  CPath public_key_file(policy_responses_dir);
  public_key_file.Append(kCachedPublicKeyFileName);

  if (!File::Exists(public_key_file)) {
    return S_FALSE;
  }

  std::vector<byte> raw_policy_response;
  HRESULT hr = ReadEntireFileShareMode(public_key_file,
                                       0,
                                       FILE_SHARE_READ,
                                       &raw_policy_response);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[ReadCachedPublicKeyFile][Read failed][%s][%#x]"),
                    public_key_file, hr));
    return hr;
  }

  hr = GetCachedPublicKeyFromResponse(
      std::string(reinterpret_cast<const char*>(&raw_policy_response[0]),
                  raw_policy_response.size()),
      key);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[ReadCachedPublicKeyFile]")
                    _T("[GetCachedPublicKeyFromResponse failed][%s][%#x]"),
                    public_key_file, hr));
    return hr;
  }

  return S_OK;
}

DmStorage::DmStorage(const CString& runtime_enrollment_token)
    : runtime_enrollment_token_(runtime_enrollment_token),
      enrollment_token_source_(kETokenSourceNone),
      dm_token_source_(kDmTokenSourceNone) {
}

void DmStorage::LoadEnrollmentTokenFromStorage() {
  // Load from most to least preferred, stopping when one is found.
  enrollment_token_ = LoadEnrollmentTokenFromCompanyPolicy();
  if (!enrollment_token_.IsEmpty()) {
    enrollment_token_source_ = kETokenSourceCompanyPolicy;
    return;
  }

#if defined(HAS_LEGACY_DM_CLIENT)
  enrollment_token_ = LoadEnrollmentTokenFromLegacyPolicy();
  if (!enrollment_token_.IsEmpty()) {
    enrollment_token_source_ = kETokenSourceLegacyPolicy;
    return;
  }

  enrollment_token_ = LoadEnrollmentTokenFromOldLegacyPolicy();
  if (!enrollment_token_.IsEmpty()) {
    enrollment_token_source_ = kETokenSourceOldLegacyPolicy;
    return;
  }
#endif  // defined(HAS_LEGACY_DM_CLIENT)

  if (!runtime_enrollment_token_.IsEmpty()) {
    enrollment_token_ = runtime_enrollment_token_;
    enrollment_token_source_ = kETokenSourceRuntime;
    return;
  }

  enrollment_token_ = LoadEnrollmentTokenFromInstall();
  if (!enrollment_token_.IsEmpty()) {
    enrollment_token_source_ = kETokenSourceInstall;
  }
}

void DmStorage::LoadDmTokenFromStorage() {
  // Load from most to least preferred, stopping when one is found.
  dm_token_ = LoadDmTokenFromKey(kRegKeyCompanyEnrollment);
  if (!dm_token_.IsEmpty()) {
    dm_token_source_ = kDmTokenSourceCompany;
    return;
  }

#if defined(HAS_LEGACY_DM_CLIENT)
  dm_token_ = LoadDmTokenFromKey(kRegKeyLegacyEnrollment);
  if (!dm_token_.IsEmpty()) {
    dm_token_source_ = kDmTokenSourceLegacy;
  }
#endif  // defined(HAS_LEGACY_DM_CLIENT)
}

void DmStorage::LoadDeviceIdFromStorage() {
  RegKey key;
  HRESULT hr = key.Open(kRegKeyCryptography, KEY_QUERY_VALUE);
  if (SUCCEEDED(hr)) {
    hr = key.GetValue(kRegValueMachineGuid, &device_id_);
  }
  if (FAILED(hr)) {
    device_id_.Empty();
  }
}

}  // namespace omaha
