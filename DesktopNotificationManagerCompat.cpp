// ******************************************************************
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE CODE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE CODE OR THE USE OR OTHER DEALINGS IN THE CODE.
// ******************************************************************

#include "DesktopNotificationManagerCompat.h"
#include <appmodel.h>
#include <wrl\wrappers\corewrappers.h>

#define RETURN_IF_FAILED(hr) do { HRESULT _hrTemp = hr; if (FAILED(_hrTemp)) { return _hrTemp; } } while (false)

using namespace ABI::Windows::Data::Xml::Dom;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

namespace DesktopNotificationManagerCompat
{
    HRESULT RegisterComServer(std::wstring clsid, const wchar_t exePath[]);
    HRESULT EnsureRegistered();
    bool IsRunningAsUwp();

    bool s_registeredAumidAndComServer = false;
    std::wstring s_aumid;
    bool s_registeredActivator = false;
    bool s_hasCheckedIsRunningAsUwp = false;
    bool s_isRunningAsUwp = false;

    HRESULT RegisterAumidAndComServer(const wchar_t *aumid, GUID clsid)
    {
        // If running as Desktop Bridge
        if (IsRunningAsUwp())
        {
            // Clear the AUMID since Desktop Bridge doesn't use it, and then we're done.
            // Desktop Bridge apps are registered with platform through their manifest.
            // Their LocalServer32 key is also registered through their manifest.
            s_aumid = L"";
            s_registeredAumidAndComServer = true;
            return S_OK;
        }

        // Copy the aumid
        s_aumid = std::wstring(aumid);

        // Get the EXE path
        wchar_t exePath[MAX_PATH];
        DWORD charWritten = ::GetModuleFileNameW(nullptr, exePath, ARRAYSIZE(exePath));
        RETURN_IF_FAILED(charWritten > 0 ? S_OK : HRESULT_FROM_WIN32(::GetLastError()));

        std::wstring display_name = L"i.o";

        // Register Application ID in the registry.
        std::wstring subKey = LR"(SOFTWARE\Classes\AppUserModelId\)" + s_aumid;
        ::RegSetKeyValueW(
            HKEY_CURRENT_USER,
            subKey.c_str(),
            L"DisplayName",
            REG_SZ,
            reinterpret_cast<const BYTE*>(display_name.c_str()),
            static_cast<DWORD>((display_name.length() + 1) * sizeof(WCHAR)));

        std::wstring exe_path = L"V:\\inner_output\\data\\";
        std::wstring icon_location = exe_path + 
            L"149-1494147_ghost-cartoon-cute-clipart-free-images-transparent-cute.png";

        // Register icon location for the app.
        ::RegSetKeyValueW(
            HKEY_CURRENT_USER,
            subKey.c_str(),
            L"IconUri",
            REG_SZ,
            reinterpret_cast<const BYTE*>(icon_location.c_str()),
            static_cast<DWORD>((icon_location.length() + 1) * sizeof(WCHAR)));

        // Turn the GUID into a string
        OLECHAR* clsidOlechar;
        StringFromCLSID(clsid, &clsidOlechar);
        std::wstring clsidStr(clsidOlechar);
        ::CoTaskMemFree(clsidOlechar);

        // clsidStr = L"{" + clsidStr + L"}";

        // Register actiator with application ID.
        ::RegSetKeyValueW(
            HKEY_CURRENT_USER,
            subKey.c_str(),
            L"CustomActivator",
            REG_SZ,
            reinterpret_cast<const BYTE*>(clsidStr.c_str()),
            static_cast<DWORD>((clsidStr.length() + 1) * sizeof(WCHAR)));

        // Register the COM server
        RETURN_IF_FAILED(RegisterComServer(clsidStr, exePath));

        // Register custom URI for io://
        subKey = L"io";
        std::wstring custom_uri_value = L"";
        ::RegSetKeyValueW(
            HKEY_CLASSES_ROOT,
            subKey.c_str(),
            L"URL Protocol",
            REG_SZ,
            reinterpret_cast<const BYTE*>(custom_uri_value.c_str()),
            static_cast<DWORD>((custom_uri_value.length() + 1) * sizeof(WCHAR)));

        // Register executable for custom URI.
        subKey = LR"(io\shell\open\command)";
        std::wstring exe_string(exePath);

#if NEWGAME_SLOW
    // everything the same except pass devenv prefixed to string.
    // note: you may need to change devenv location in registry, something in LOCAL_MACHINE + AppPaths
    // see:  https://learn.microsoft.com/en-us/visualstudio/debugger/debug-multiple-processes?view=vs-2022#BKMK_Automatically_start_an_process_in_the_debugger
    //
    // find a running instance of visual with the correct exe
    // pipe arguments into it (sharelink arguments, toast activation arguments, etc)
    // exe_string.insert(0, L"devenv /debugexe ");
#endif
        exe_string += L" -ToastActivated %1";
        ::RegSetKeyValueW(
            HKEY_CLASSES_ROOT,
            subKey.c_str(),
            nullptr,
            REG_SZ,
            reinterpret_cast<const BYTE*>(exe_string.c_str()),
            static_cast<DWORD>((exe_string.length() + 1) * sizeof(WCHAR)));

        s_registeredAumidAndComServer = true;
        return S_OK;
    }

    HRESULT RegisterActivator()
    {
        // Module<OutOfProc> needs a callback registered before it can be used.
        // Since we don't care about when it shuts down, we'll pass an empty lambda here.
        Module<OutOfProc>::Create([] {});

        // If a local server process only hosts the COM object then COM expects
        // the COM server host to shutdown when the references drop to zero.
        // Since the user might still be using the program after activating the notification,
        // we don't want to shutdown immediately.  Incrementing the object count tells COM that
        // we aren't done yet.
        Module<OutOfProc>::GetModule().IncrementObjectCount();

        RETURN_IF_FAILED(Module<OutOfProc>::GetModule().RegisterObjects());

        s_registeredActivator = true;
        return S_OK;
    }

    HRESULT RegisterComServer(std::wstring clsidStr, const wchar_t exePath[])
    {
        // Create the subkey
        // Something like SOFTWARE\Classes\CLSID\{23A5B06E-20BB-4E7E-A0AC-6982ED6A6041}\LocalServer32
        std::wstring subKey = LR"(SOFTWARE\Classes\CLSID\)" + clsidStr + LR"(\LocalServer32)";

        // Include -ToastActivated launch args on the exe
        std::wstring exePathStr(exePath);
        exePathStr = L"\"" + exePathStr + L"\" " + TOAST_ACTIVATED_LAUNCH_ARG;

        // We don't need to worry about overflow here as ::GetModuleFileName won't
        // return anything bigger than the max file system path (much fewer than max of DWORD).
        DWORD dataSize = static_cast<DWORD>((exePathStr.length() + 1) * sizeof(WCHAR));

        // Register the EXE for the COM server
        // note (mdm): my linker was not able to find RegSetKeyValueW, I had to include Advapi32.lib in build.bat
        return HRESULT_FROM_WIN32(::RegSetKeyValueW(
            HKEY_CURRENT_USER,
            subKey.c_str(),
            nullptr,
            REG_SZ,
            reinterpret_cast<const BYTE*>(exePathStr.c_str()),
            dataSize));
    }

    HRESULT CreateToastNotifier(IToastNotifier **notifier)
    {
        RETURN_IF_FAILED(EnsureRegistered());

        ComPtr<IToastNotificationManagerStatics> toastStatics;
        RETURN_IF_FAILED(Windows::Foundation::GetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            &toastStatics));

        if (s_aumid.empty())
        {
            return toastStatics->CreateToastNotifier(notifier);
        }
        else
        {
            return toastStatics->CreateToastNotifierWithId(HStringReference(s_aumid.c_str()).Get(), notifier);
        }
    }

    HRESULT CreateXmlDocumentFromString(const wchar_t *xmlString, IXmlDocument **doc)
    {
        ComPtr<IXmlDocument> answer;
        RETURN_IF_FAILED(Windows::Foundation::ActivateInstance(HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(), &answer));

        ComPtr<IXmlDocumentIO> docIO;
        RETURN_IF_FAILED(answer.As(&docIO));

        // Load the XML string
        RETURN_IF_FAILED(docIO->LoadXml(HStringReference(xmlString).Get()));

        return answer.CopyTo(doc);
    }

    HRESULT CreateToastNotification(IXmlDocument *content, IToastNotification **notification)
    {
        ComPtr<IToastNotificationFactory> factory;
        RETURN_IF_FAILED(Windows::Foundation::GetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
            &factory));

        return factory->CreateToastNotification(content, notification);
    }

    HRESULT get_History(std::unique_ptr<DesktopNotificationHistoryCompat>* history)
    {
        RETURN_IF_FAILED(EnsureRegistered());

        ComPtr<IToastNotificationManagerStatics> toastStatics;
        RETURN_IF_FAILED(Windows::Foundation::GetActivationFactory(
            HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
            &toastStatics));

        ComPtr<IToastNotificationManagerStatics2> toastStatics2;
        RETURN_IF_FAILED(toastStatics.As(&toastStatics2));

        ComPtr<IToastNotificationHistory> nativeHistory;
        RETURN_IF_FAILED(toastStatics2->get_History(&nativeHistory));

        *history = std::unique_ptr<DesktopNotificationHistoryCompat>(new DesktopNotificationHistoryCompat(s_aumid.c_str(), nativeHistory));
        return S_OK;
    }

    bool CanUseHttpImages()
    {
        return IsRunningAsUwp();
    }

    HRESULT EnsureRegistered()
    {
        // If not registered AUMID yet
        if (!s_registeredAumidAndComServer)
        {
            // Check if Desktop Bridge
            if (IsRunningAsUwp())
            {
                // Implicitly registered, all good!
                s_registeredAumidAndComServer = true;
            }
            else
            {
                // Otherwise, incorrect usage, must call RegisterAumidAndComServer first
                return E_ILLEGAL_METHOD_CALL;
            }
        }

        // If not registered activator yet
        if (!s_registeredActivator)
        {
            // Incorrect usage, must call RegisterActivator first
            return E_ILLEGAL_METHOD_CALL;
        }

        return S_OK;
    }

    bool IsRunningAsUwp()
    {
        if (!s_hasCheckedIsRunningAsUwp)
        {
            // https://stackoverflow.com/questions/39609643/determine-if-c-application-is-running-as-a-uwp-app-in-desktop-bridge-project
            UINT32 length;
            wchar_t packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1];
            LONG result = GetPackageFamilyName(GetCurrentProcess(), &length, packageFamilyName);
            s_isRunningAsUwp = result == ERROR_SUCCESS;
            s_hasCheckedIsRunningAsUwp = true;
        }

        return s_isRunningAsUwp;
    }
}

DesktopNotificationHistoryCompat::DesktopNotificationHistoryCompat(const wchar_t *aumid, ComPtr<IToastNotificationHistory> history)
{
    m_aumid = std::wstring(aumid);
    m_history = history;
}

HRESULT DesktopNotificationHistoryCompat::Clear()
{
    if (m_aumid.empty())
    {
        return m_history->Clear();
    }
    else
    {
        return m_history->ClearWithId(HStringReference(m_aumid.c_str()).Get());
    }
}

HRESULT DesktopNotificationHistoryCompat::GetHistory(ABI::Windows::Foundation::Collections::IVectorView<ToastNotification*> **toasts)
{
    ComPtr<IToastNotificationHistory2> history2;
    RETURN_IF_FAILED(m_history.As(&history2));

    if (m_aumid.empty())
    {
        return history2->GetHistory(toasts);
    }
    else
    {
        return history2->GetHistoryWithId(HStringReference(m_aumid.c_str()).Get(), toasts);
    }
}

HRESULT DesktopNotificationHistoryCompat::Remove(const wchar_t *tag)
{
    if (m_aumid.empty())
    {
        return m_history->Remove(HStringReference(tag).Get());
    }
    else
    {
        return m_history->RemoveGroupedTagWithId(HStringReference(tag).Get(), HStringReference(L"").Get(), HStringReference(m_aumid.c_str()).Get());
    }
}

HRESULT DesktopNotificationHistoryCompat::RemoveGroupedTag(const wchar_t *tag, const wchar_t *group)
{
    if (m_aumid.empty())
    {
        return m_history->RemoveGroupedTag(HStringReference(tag).Get(), HStringReference(group).Get());
    }
    else
    {
        return m_history->RemoveGroupedTagWithId(HStringReference(tag).Get(), HStringReference(group).Get(), HStringReference(m_aumid.c_str()).Get());
    }
}

HRESULT DesktopNotificationHistoryCompat::RemoveGroup(const wchar_t *group)
{
    if (m_aumid.empty())
    {
        return m_history->RemoveGroup(HStringReference(group).Get());
    }
    else
    {
        return m_history->RemoveGroupWithId(HStringReference(group).Get(), HStringReference(m_aumid.c_str()).Get());
    }
}
