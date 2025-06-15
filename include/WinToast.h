#pragma once

#include <shobjidl.h> // SetCurrentProcessExplicitAppUserModelID
#include <string>
#include <windows.data.xml.dom.h>
#include <windows.h>
#include <windows.ui.notifications.h>
#include <winstring.h>
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "shell32.lib")

inline bool initializeWinToast(const std::wstring& appName, const std::wstring& appId = L"com.shimamura.AnimeRSS") {
    using namespace Microsoft::WRL::Wrappers;

    // 设置 AppUserModelID（必须）
    if (FAILED(SetCurrentProcessExplicitAppUserModelID(appId.c_str())))
        return false;

    // 初始化 COM
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
