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

#include "omaha/goopdate/dm_client.h"

#include <memory>

#include "base/scoped_ptr.h"
#include "omaha/base/app_util.h"
#include "omaha/base/constants.h"
#include "omaha/base/debug.h"
#include "omaha/base/logging.h"
#include "omaha/base/omaha_version.h"
#include "omaha/base/safe_format.h"
#include "omaha/base/string.h"
#include "omaha/base/system_info.h"
#include "omaha/common/config_manager.h"
#include "omaha/common/goopdate_utils.h"
#include "omaha/common/url_utils.h"
#include "omaha/goopdate/dm_messages.h"
#include "omaha/goopdate/dm_storage.h"
#include "omaha/net/network_config.h"
#include "omaha/net/network_request.h"
#include "omaha/net/simple_request.h"

namespace omaha {
namespace dm_client {

RegistrationState GetRegistrationState(DmStorage* dm_storage) {
  ASSERT1(dm_storage);

  if (!dm_storage->GetDmToken().IsEmpty()) {
    return kRegistered;
  }

  return dm_storage->GetEnrollmentToken().IsEmpty()
      ? kNotManaged
      : kRegistrationPending;
}

// Log categories are used within this function as follows:
// OPT: diagnostic messages to be included in the log file in release builds.
// REPORT: error messages to be included in the Windows Event Log.
HRESULT RegisterIfNeeded(DmStorage* dm_storage) {
  ASSERT1(dm_storage);
  OPT_LOG(L1, (_T("[DmClient::RegisterIfNeeded]")));

  // No work to be done if a DM token was found.
  CStringA dm_token = dm_storage->GetDmToken();
  if (!dm_token.IsEmpty()) {
    OPT_LOG(L1, (_T("[Device is already registered]")));
    return S_FALSE;
  }

  // No work to be done if no enrollment token was found.
  CString enrollment_token = dm_storage->GetEnrollmentToken();
  if (enrollment_token.IsEmpty()) {
    OPT_LOG(L1, (_T("[No enrollment token found]")));
    return S_FALSE;
  }

  // Cannot register if no device ID was found.
  CString device_id = dm_storage->GetDeviceId();
  if (device_id.IsEmpty()) {
    REPORT_LOG(LE, (_T("[Device ID not found]")));
    return E_FAIL;
  }

  HRESULT hr = internal::RegisterWithRequest(new SimpleRequest,
                                             enrollment_token, device_id,
                                             &dm_token);
  if (FAILED(hr)) {
    return hr;
  }

  hr = dm_storage->StoreDmToken(dm_token);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[StoreDmToken failed][%#x]"), hr));
    return hr;
  }

  OPT_LOG(L1, (_T("[Registration complete]")));

  return S_OK;
}

namespace internal {

HRESULT RegisterWithRequest(HttpRequestInterface* http_request,
                            const CString& enrollment_token,
                            const CString& device_id,
                            CStringA* dm_token) {
  ASSERT1(http_request);
  ASSERT1(dm_token);
  // Get the network configuration.
  NetworkConfig* network_config = NULL;
  NetworkConfigManager& network_config_manager =
      NetworkConfigManager::Instance();
  HRESULT hr = network_config_manager.GetUserNetworkConfig(&network_config);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[GetUserNetworkConfig failed][%#x]"), hr));
    return hr;
  }

  // Create a network request and configure its headers.
  scoped_ptr<NetworkRequest> request(
      new NetworkRequest(network_config->session()));
  // DeviceManagementRequestJobImpl::ConfigureRequest.
  request->AddHeader(L"Authorization",
                     internal::FormatEnrollmentTokenAuthorizationHeader(
                         enrollment_token));

  // Set it up
  request->AddHttpRequest(http_request);

  // Form the request URL with query params.
  CString url;
  hr = ConfigManager::Instance()->GetDeviceManagementUrl(&url);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[GetDeviceManagementUrl failed][%#x]"), hr));
    return hr;
  }

  std::vector<std::pair<CString, CString>> query_params;
  // DeviceManagementRequestJob::DeviceManagementRequestJob.
  // kParamRequest = kValueRequestTokenEnrollment.
  query_params.push_back(std::make_pair(_T("request"),
                                        _T("register_policy_agent")));
  // kParamAppType = kValueAppType.
  query_params.push_back(std::make_pair(_T("apptype"), _T("Chrome")));
  // kParamAgent.
  query_params.push_back(std::make_pair(_T("agent"), internal::GetAgent()));
  // kParamPlatform.
  query_params.push_back(std::make_pair(_T("platform"),
                                        internal::GetPlatform()));

  // DeviceManagementRequestJob::SetClientID.
  // kParamDeviceID.
  query_params.push_back(std::make_pair(_T("deviceid"), device_id));

  hr = internal::AppendQueryParamsToUrl(query_params, &url);
  if (FAILED(hr)) {
    REPORT_LOG(LW, (_T("[AppendQueryParamsToUrl failed][%#x]"), hr));
    return hr;
  }

  // Make the request payload.
  // DeviceManagementRequest.RegisterBrowserRequest:
  CStringA payload = SerializeRegisterBrowserRequest(
      WideToUtf8(app_util::GetHostName()),  // policy::GetMachineName
      CStringA("Windows"),  // policy::GetOSPlatform
      internal::GetOsVersion());  // policy::GetOSVersion
  if (payload.IsEmpty()) {
    REPORT_LOG(LE, (_T("[SerializeRegisterBrowserRequest failed]")));
    return E_FAIL;
  }

  std::vector<uint8> response;
  hr = request->Post(url, payload, payload.GetLength(), &response);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[NetworkRequest::Post failed][%#x, %s]"), hr, url));
    return hr;
  }

  const int http_status_code = request->http_status_code();
  if (http_status_code != 200) {
    REPORT_LOG(LE, (_T("[NetworkRequest::Post failed][status code %d]"),
                    http_status_code));
    CStringA error_message;
    hr = ParseDeviceManagementResponseError(response, &error_message);
    if (SUCCEEDED(hr)) {
      OPT_LOG(LE, (_T("[Server returned: %S]"), error_message));
    }
    return E_FAIL;
  }

  hr = ParseDeviceRegisterResponse(response, dm_token);
  if (FAILED(hr)) {
    REPORT_LOG(LE, (_T("[ParseDeviceRegisterResponse failed][%#x]"), hr));
    return hr;
  }

  return S_OK;
}

CString GetAgent() {
  // DeviceManagementServiceConfiguration::GetAgentParameter.
  CString agent;
  SafeCStringFormat(&agent, _T("%s %s()"), kAppName, GetVersionString());
  return agent;
}

CString GetPlatform() {
  // DeviceManagementServiceConfiguration::GetPlatformParameter.
  const DWORD architecture = SystemInfo::GetProcessorArchitecture();

  int major_version = 0;
  int minor_version = 0;
  int service_pack_major = 0;
  int service_pack_minor = 0;
  if (!SystemInfo::GetSystemVersion(&major_version, &minor_version,
                                    &service_pack_major, &service_pack_minor)) {
    major_version = 0;
    minor_version = 0;
  }

  CString platform;
  SafeCStringFormat(&platform, _T("Windows NT|%s|%d.%d.0"),
                    architecture == PROCESSOR_ARCHITECTURE_AMD64 ?
                        _T("x86_64") :
                        (architecture == PROCESSOR_ARCHITECTURE_INTEL ?
                             _T("x86") :
                             _T("")),
                    major_version, minor_version);
  return platform;
}

CStringA GetOsVersion() {
  CString os_version;
  CString service_pack;

  if (FAILED(goopdate_utils::GetOSInfo(&os_version, &service_pack))) {
    return "0.0.0.0";
  }
  return WideToUtf8(os_version);
}

HRESULT AppendQueryParamsToUrl(
    const std::vector<std::pair<CString,CString>>& query_params,
    CString* url) {
  ASSERT1(url);
  CString query;
  HRESULT hr = BuildQueryString(query_params, &query);
  if (FAILED(hr)) {
    return hr;
  }

  CString result;
  SafeCStringFormat(&result, _T("%s?%s"), *url, query);
  *url = result;
  return S_OK;
}

CString FormatEnrollmentTokenAuthorizationHeader(
    const CString& token) {
  CString header_value;
  SafeCStringFormat(&header_value, _T("GoogleEnrollmentToken token=%s"), token);
  return header_value;
}

}  // namespace internal
}  // namespace dm_client
}  // namespace omaha
