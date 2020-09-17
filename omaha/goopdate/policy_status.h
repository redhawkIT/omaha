// Copyright 2008-2010 Google Inc.
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
// ========================================================================
//
// Contains PolicyStatus class to launch a process using a COM interface. This
// COM object can be created by medium integrity callers.

#ifndef OMAHA_GOOPDATE_POLICY_STATUS_H__
#define OMAHA_GOOPDATE_POLICY_STATUS_H__

#include <windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include "omaha/base/atlregmapex.h"
#include "omaha/common/const_cmd_line.h"
#include "omaha/common/const_goopdate.h"
#include "omaha/common/goopdate_utils.h"
#include "omaha/goopdate/com_proxy.h"
#include "omaha/goopdate/non_localized_resource.h"

// Generated by MIDL in the "BUILD_MODE.OBJ_ROOT + SETTINGS.SUBDIR".
#include "goopdate/omaha3_idl.h"

namespace omaha {

const TCHAR* const kPolicyStatusDescription =
    _T("Google Update Policy Status Class");

class ATL_NO_VTABLE PolicyStatus
    : public CComObjectRootEx<CComMultiThreadModel>,
      public CComCoClass<PolicyStatus, &__uuidof(PolicyStatusClass)>,
      public IDispatchImpl<IPolicyStatus2,
                           &__uuidof(IPolicyStatus2),
                           &CAtlModule::m_libid,
                           kMajorTypeLibVersion,
                           kMinorTypeLibVersion>,
      public StdMarshalInfo {
 public:
  PolicyStatus();
  virtual ~PolicyStatus();

  DECLARE_NOT_AGGREGATABLE(PolicyStatus)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

  DECLARE_REGISTRY_RESOURCEID_EX(IDR_LOCAL_SERVER_RGS)

  BEGIN_REGISTRY_MAP()
    REGMAP_ENTRY(_T("HKROOT"),       goopdate_utils::GetHKRoot())
    REGMAP_MODULE2(_T("MODULE"),     kOmahaOnDemandFileName)
    REGMAP_ENTRY(_T("VERSION"),      _T("1.0"))
    REGMAP_ENTRY(_T("PROGID"),       kProgIDPolicyStatus)
    REGMAP_ENTRY(_T("DESCRIPTION"),  kPolicyStatusDescription)
    REGMAP_UUID(_T("CLSID"),         __uuidof(PolicyStatusClass))
  END_REGISTRY_MAP()

  BEGIN_COM_MAP(PolicyStatus)
    COM_INTERFACE_ENTRY(IPolicyStatus)
    COM_INTERFACE_ENTRY(IPolicyStatus2)
    COM_INTERFACE_ENTRY(IStdMarshalInfo)
  END_COM_MAP()

  // IPolicyStatus/IPolicyStatus2.
  // Global Update Policies.
  STDMETHOD(get_lastCheckPeriodMinutes)(DWORD* minutes);
  STDMETHOD(get_updatesSuppressedTimes)(DWORD* start_hour,
                                        DWORD* start_min,
                                        DWORD* duration_min,
                                        VARIANT_BOOL* are_updates_suppressed);

  STDMETHOD(get_downloadPreferenceGroupPolicy)(BSTR* pref);
  STDMETHOD(get_packageCacheSizeLimitMBytes)(DWORD* limit);
  STDMETHOD(get_packageCacheExpirationTimeDays)(DWORD* days);

  // Application Update Policies.
  STDMETHOD(get_effectivePolicyForAppInstalls)(BSTR app_id, DWORD* policy);
  STDMETHOD(get_effectivePolicyForAppUpdates)(BSTR app_id, DWORD* policy);
  STDMETHOD(get_targetVersionPrefix)(BSTR app_id, BSTR* prefix);
  STDMETHOD(get_isRollbackToTargetVersionAllowed)(
      BSTR app_id,
      VARIANT_BOOL* rollback_allowed);
  STDMETHOD(get_targetChannel)(BSTR app_id, BSTR* channel);

 private:
  DISALLOW_COPY_AND_ASSIGN(PolicyStatus);
};

}  // namespace omaha

#endif  // OMAHA_GOOPDATE_POLICY_STATUS_H__
