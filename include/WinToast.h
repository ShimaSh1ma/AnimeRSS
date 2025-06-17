#pragma once

#include <filesystem>
#include <objbase.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <string>
#include <strsafe.h>
#include <windows.data.xml.dom.h>
#include <windows.h>
#include <windows.ui.notifications.h>
#include <winstring.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

#pragma once

#include <filesystem>
#include <objbase.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <strsafe.h>
#include <windows.h>

#include <windows.data.xml.dom.h>
#include <windows.ui.notifications.h>
#include <winstring.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "runtimeobject.lib")

inline bool createShortcutIfNeeded(const std::wstring& appId, const std::wstring& shortcutName) {
    // 获取当前 exe 路径
    wchar_t exePath[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
        return false;

    // 获取开始菜单目录
    PWSTR startMenuPath = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_StartMenu, 0, nullptr, &startMenuPath)))
        return false;

    std::filesystem::path shortcutPath = std::filesystem::path(startMenuPath) / L"Programs" / (shortcutName + L".lnk");
    CoTaskMemFree(startMenuPath);

    if (std::filesystem::exists(shortcutPath))
        return true; // 已存在

    // 初始化 COM（单独为创建快捷方式用）
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        return false;

    bool result = false;
    IShellLinkW* pShellLink = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pShellLink)))) {
        pShellLink->SetPath(exePath);
        pShellLink->SetArguments(L"");
        pShellLink->SetWorkingDirectory(std::filesystem::path(exePath).parent_path().c_str());
        pShellLink->SetIconLocation(exePath, 0);

        // 设置 AppUserModelID 属性
        IPropertyStore* pPropStore = nullptr;
        if (SUCCEEDED(pShellLink->QueryInterface(IID_PPV_ARGS(&pPropStore)))) {
            PROPVARIANT propvar;
            if (SUCCEEDED(InitPropVariantFromString(appId.c_str(), &propvar))) {
                pPropStore->SetValue(PKEY_AppUserModel_ID, propvar);
                pPropStore->Commit();
                PropVariantClear(&propvar);
            }
            pPropStore->Release();
        }

        IPersistFile* pPersistFile = nullptr;
        if (SUCCEEDED(pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile)))) {
            if (SUCCEEDED(pPersistFile->Save(shortcutPath.c_str(), TRUE)))
                result = true;
            pPersistFile->Release();
        }

        pShellLink->Release();
    }

    CoUninitialize();
    return result;
}

inline bool initializeWinToast(const std::wstring& appId = L"com.shimamura.AnimeRSS", const std::wstring& shortcutName = L"AnimeRSS") {
    using namespace Microsoft::WRL::Wrappers;

    // 设置当前目录为 exe 所在目录
    wchar_t exePath[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, exePath, MAX_PATH))
        return false;
    std::filesystem::current_path(std::filesystem::path(exePath).parent_path());

    // 设置 AppUserModelID（必须）
    if (FAILED(SetCurrentProcessExplicitAppUserModelID(appId.c_str())))
        return false;

    // 创建开始菜单快捷方式（必须）
    if (!createShortcutIfNeeded(appId, shortcutName))
        return false;

    // 初始化 WinRT COM（用于 toast）
    RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    return SUCCEEDED(initialize);
}

inline void showWinToastNotification(const std::wstring& title, const std::wstring& message, const std::wstring& imagePath = L"",
                                     const std::wstring& appId = L"com.shimamura.AnimeRSS") {
    using namespace Microsoft::WRL;
    using namespace Microsoft::WRL::Wrappers;
    using namespace ABI::Windows::Data::Xml::Dom;
    using namespace ABI::Windows::UI::Notifications;

    std::wstring xmlString = LR"(<toast><visual><binding template="ToastGeneric">)"
                             L"<text>" +
                             title +
                             L"</text>"
                             L"<text>" +
                             message + L"</text>" + (imagePath.empty() ? L"" : L"<image placement='appLogoOverride' src='" + imagePath + L"'/>\n") +
                             L"</binding></visual></toast>";

    // 创建 XmlDocument
    ComPtr<IXmlDocument> xmlDoc;
    HRESULT hr = RoActivateInstance(HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument).Get(), &xmlDoc);
    if (FAILED(hr))
        return;

    ComPtr<IXmlDocumentIO> xmlDocIO;
    if (FAILED(xmlDoc.As(&xmlDocIO)))
        return;
    if (FAILED(xmlDocIO->LoadXml(HStringReference(xmlString.c_str()).Get())))
        return;

    // 获取 ToastNotificationFactory
    ComPtr<IToastNotificationFactory> toastFactory;
    hr = Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &toastFactory);
    if (FAILED(hr))
        return;

    // 创建 ToastNotification 对象
    ComPtr<IToastNotification> toast;
    hr = toastFactory->CreateToastNotification(xmlDoc.Get(), &toast);
    if (FAILED(hr))
        return;

    // 获取 ToastNotifier 并展示
    ComPtr<IToastNotificationManagerStatics> toastManager;
    hr = Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &toastManager);
    if (FAILED(hr))
        return;

    ComPtr<IToastNotifier> notifier;
    hr = toastManager->CreateToastNotifierWithId(HStringReference(appId.c_str()).Get(), &notifier);
    if (FAILED(hr))
        return;

    notifier->Show(toast.Get());
}
