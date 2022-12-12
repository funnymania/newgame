// study: what is this?    
_Use_decl_annotations_
HRESULT SetNodeValueString(HSTRING inputString, IXmlNode* node, IXmlDocument* xml)
{
    ComPtr<IXmlText> inputText;
    RETURN_IF_FAILED(xml->CreateTextNode(inputString, &inputText));

    ComPtr<IXmlNode> inputTextNode;
    RETURN_IF_FAILED(inputText.As(&inputTextNode));

    ComPtr<IXmlNode> appendedChild;
    return node->AppendChild(inputTextNode.Get(), &appendedChild);
}

// HRESULT OpenWindowIfNeeded()
// {
//     // If no window exists
//     if (m_hwnd == nullptr)
//     {
//         // If not on main UI thread
//         if (m_threadId != GetCurrentThreadId())
//         {
//             // We have to initialize on UI thread so that the message loop is handled correctly
//             HANDLE h = CreateEvent(NULL, 0, 0, NULL);
//             PostThreadMessage(m_threadId, DesktopToastsApp::WM_USER_OPENWINDOWIFNEEDED, NULL, (LPARAM)h);
//             WaitForSingleObject(h, INFINITE);
//             return S_OK;
//         }
//         else
//         {
//             // Otherwise, create the window
//             return Initialize(m_hInstance);
//         }
//     }
//     else
//     {
//         // Otherwise, ensure window is unminimized and in the foreground
//         ::ShowWindow(m_hwnd, SW_RESTORE);
//         ::SetForegroundWindow(m_hwnd);
//         return S_OK;
//     }
// }

// Set the values of each of the text nodes
_Use_decl_annotations_
HRESULT SetTextValues(const PCWSTR* textValues, UINT32 textValuesCount, IXmlDocument* toastXml)
{
    ComPtr<IXmlNodeList> nodeList;
    RETURN_IF_FAILED(toastXml->GetElementsByTagName(Wrappers::HStringReference(L"text").Get(), &nodeList));

    UINT32 nodeListLength;
    RETURN_IF_FAILED(nodeList->get_Length(&nodeListLength));

    // If a template was chosen with fewer text elements, also change the amount of strings
    // passed to this method.
    RETURN_IF_FAILED(textValuesCount <= nodeListLength ? S_OK : E_INVALIDARG);

    for (UINT32 i = 0; i < textValuesCount; i++)
    {
        ComPtr<IXmlNode> textNode;
        RETURN_IF_FAILED(nodeList->Item(i, &textNode));

        RETURN_IF_FAILED(SetNodeValueString(Wrappers::HStringReference(textValues[i]).Get(), textNode.Get(), toastXml));
    }

    return S_OK;
}

// Create and display the toast
_Use_decl_annotations_
HRESULT ShowToast(IXmlDocument* xml)
{
    // Create the notifier
    // Classic Win32 apps MUST use the compat method to create the notifier
    ComPtr<IToastNotifier> notifier;
    RETURN_IF_FAILED(DesktopNotificationManagerCompat::CreateToastNotifier(&notifier));

    // And create the notification itself
    ComPtr<IToastNotification> toast;
    RETURN_IF_FAILED(DesktopNotificationManagerCompat::CreateToastNotification(xml, &toast));

    // And show it!
    return notifier->Show(toast.Get());
}

HRESULT SendResponse(PCWSTR message)
{
    ComPtr<IXmlDocument> doc;
    RETURN_IF_FAILED(DesktopNotificationManagerCompat::CreateXmlDocumentFromString(LR"(<toast>
    <visual>
        <binding template="ToastGeneric">
            <text></text>
        </binding>
    </visual>
</toast>)", &doc));

    PCWSTR textValues[] = {
        message
    };
    RETURN_IF_FAILED(SetTextValues(textValues, ARRAYSIZE(textValues), doc.Get()));

    return ShowToast(doc.Get());
}

// The UUID CLSID must be unique to your app. Create a new GUID if copying this code.
class DECLSPEC_UUID("9df15801-7c52-483b-a0da-61cbd3cefb11") NotificationActivator WrlSealed WrlFinal
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback>
{
public:
    virtual HRESULT STDMETHODCALLTYPE Activate(
        _In_ LPCWSTR appUserModelId,
        _In_ LPCWSTR invokedArgs,
        _In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA* data,
        ULONG dataCount) override
    {
        std::wstring arguments(invokedArgs);
        HRESULT hr = S_OK;

        // Background: Quick reply to the conversation
        if (arguments.find(L"action=reply") == 0)
        {
            // Get the response user typed.
            // We know this is first and only user input since our toasts only have one input
            LPCWSTR response = data[0].Value;

            hr = SendResponse(L"Hey.");
        }

        else
        {
            // The remaining scenarios are foreground activations,
            // If we don't have a window, or if the window is minimized, or if we are not on main UI thread.
            // hr = app_instance->OpenWindowIfNeeded();
            // if (SUCCEEDED(hr))
            // {
                // Open the image
                if (arguments.find(L"action=viewImage") == 0)
                {
                    // hr = app_instance->OpenImage();
                }

                // Open the app itself
                // User might have clicked on app title in Action Center which launches with empty args
                else
                {
                    // Nothing to do, already launched
                    //std::wstring notification_content = 

                    hr = SendResponse(L"Hey.");
                }
            // }
        }

        if (FAILED(hr))
        {
            // Log failed HRESULT
        }

        return S_OK;
    }

    ~NotificationActivator()
    {
        // If we don't have window open
        // if (!app_instance->HasWindow())
        // {
        //     // Exit (this is for background activation scenarios)
        //     exit(0);
        // }
    }
};

CoCreatableClass(NotificationActivator);
