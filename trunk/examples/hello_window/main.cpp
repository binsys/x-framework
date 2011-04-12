
#include <tchar.h>
#include <windows.h>
#include <initguid.h>
#include <oleacc.h>

#include <atlbase.h>

#include "../wanui_res/resource.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/scoped_ptr.h"

#include "message/message_loop.h"

#include "gfx/Image.h"

#include "SkBitmap.h"

#include "view_framework/base/hwnd_util.h"
#include "view_framework/base/resource_bundle.h"
#include "view_framework/clipboard/clipboard.h"
#include "view_framework/controls/button/text_button.h"
#include "view_framework/controls/textfield/textfield.h"
#include "view_framework/focus/accelerator_handler.h"
#include "view_framework/gfx/painter.h"
#include "view_framework/layout/box_layout.h"
#include "view_framework/layout/fill_layout.h"
#include "view_framework/view/view.h"
#include "view_framework/view/view_delegate.h"
#include "view_framework/widget/widget.h"
#include "view_framework/window/dialog_delegate.h"
#include "view_framework/window/window.h"

CComModule _Module;

// ���ж��벼����.
class CenterLayout : public view::LayoutManager
{
public:
    CenterLayout() {}
    virtual ~CenterLayout() {}

    virtual void Layout(view::View* host)
    {
        view::View* child = host->GetChildViewAt(0);
        gfx::Size size = child->GetPreferredSize();
        child->SetBounds((host->width()-size.width())/2,
            (host->height()-size.height())/2,
            size.width(), size.height());
    }

    virtual gfx::Size GetPreferredSize(view::View* host)
    {
        return host->GetPreferredSize();
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CenterLayout);
};

class TestViewsDelegate : public view::ViewDelegate
{
public:
    TestViewsDelegate() {}
    virtual ~TestViewsDelegate() {}

    // Overridden from views::ViewsDelegate:
    virtual view::Clipboard* GetClipboard() const
    {
        if(!clipboard_.get())
        {
            // Note that we need a MessageLoop for the next call to work.
            clipboard_.reset(new view::Clipboard);
        }
        return clipboard_.get();
    }
    virtual void SaveWindowPlacement(view::Window* window,
        const std::wstring& window_name,
        const gfx::Rect& bounds,
        bool maximized) {}
    virtual bool GetSavedWindowBounds(view::Window* window,
        const std::wstring& window_name,
        gfx::Rect* bounds) const { return false; }

    virtual bool GetSavedMaximizedState(view::Window* window,
        const std::wstring& window_name,
        bool* maximized) const { return false; }

    virtual void NotifyAccessibilityEvent(view::View* view,
        AccessibilityTypes::Event event_type) {}

    virtual void NotifyMenuItemFocused(
        const std::wstring& menu_name,
        const std::wstring& menu_item_name,
        int item_index,
        int item_count,
        bool has_submenu) {}

    virtual HICON GetDefaultWindowIcon() const
    {
        return NULL;
    }

    virtual void AddRef() {}
    virtual void ReleaseRef() {}

private:
    mutable scoped_ptr<view::Clipboard> clipboard_;

    DISALLOW_COPY_AND_ASSIGN(TestViewsDelegate);
};

// �Ի��򴰿ڴ���.
class ModalDialog : public view::DialogDelegate
{
    view::View* content_;

public:
    ModalDialog() : content_(NULL) {}

    virtual bool CanResize() const
    {
        return true;
    }

    virtual bool IsModal() const
    {
        return true;
    }

    virtual std::wstring GetWindowTitle() const
    {
        return L"�Ի���";
    }

    virtual void DeleteDelegate() { delete this; }

    virtual view::View* GetContentsView()
    {
        if(!content_)
        {
            content_ = new view::View();
            content_->SetLayoutManager(new view::FillLayout());
            view::Textfield* text_filed = new view::Textfield(
                view::Textfield::STYLE_MULTILINE);
            text_filed->SetTextColor(SkColorSetRGB(107, 221, 149));
            content_->AddChildView(text_filed);
        }
        return content_;
    }
};

// �����ڴ���.
class MainWindow : public view::WindowDelegate,
    public view::ButtonListener
{
    enum
    {
        kButtonTitle,
        kButtonAlwaysOnTop,
        kButtonDialog,
        kButtonWidget,
        kButtonClose,
        kButtonCloseWidget,
    };

    view::View* content_;
    bool use_alpha_;
    std::wstring title_;
    bool always_on_top_;

public:
    MainWindow() : content_(NULL), use_alpha_(false),
        title_(L"Hello Window!"), always_on_top_(false)
    {
    }

    virtual bool CanResize() const
    {
        return true;
    }

    virtual bool CanMaximize() const
    {
        return true;
    }

    virtual std::wstring GetWindowTitle() const
    {
        return title_;
    }

    virtual void WindowClosing()
    {
        MessageLoopForUI::current()->Quit();
    }

    virtual void DeleteDelegate() { delete this; }

    virtual void OnWindowBeginUserBoundsChange()
    {
        window()->SetUseDragFrame(true);
    }

    virtual void OnWindowEndUserBoundsChange()
    {
        window()->SetUseDragFrame(false);
    }

    virtual view::View* GetContentsView()
    {
        if(!content_)
        {
            content_ = new view::View();
            content_->set_background(view::Background::CreateStandardPanelBackground());
            content_->SetLayoutManager(new view::BoxLayout(view::BoxLayout::kVertical,
                0, 0, 0));

            {
                view::TextButton* button = new view::TextButton(this, L"�޸Ĵ��ڱ���");
                button->set_tag(kButtonTitle);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"�����ö�����");
                button->set_tag(kButtonAlwaysOnTop);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"��ʾ�Ի���");
                button->set_tag(kButtonDialog);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"����Widget");
                button->set_tag(kButtonWidget);
                content_->AddChildView(button);
            }
            {
                view::TextButton* button = new view::TextButton(this, L"�رմ���");
                button->set_tag(kButtonClose);
                content_->AddChildView(button);
            }
        }

        return content_;
    }

    virtual void ButtonPressed(view::Button* sender, const view::Event& event)
    {
        switch(sender->tag())
        {
        case kButtonTitle:
            {
                title_ = L"�����Ĵ��幦����ʾ!";
                window()->UpdateWindowTitle();
            }
            break;
        case kButtonAlwaysOnTop:
            {
                always_on_top_ = !always_on_top_;
                window()->SetIsAlwaysOnTop(always_on_top_);
            }
            break;
        case kButtonDialog:
            {
                ModalDialog* dialog = new ModalDialog();
                view::Window::CreateWanWindow(window()->GetNativeWindow(),
                    gfx::Rect(0, 0, 200, 200), dialog);
                view::CenterAndSizeWindow(window()->GetNativeWindow(),
                    dialog->window()->GetNativeWindow(),
                    dialog->window()->GetBounds().size(), false);
                dialog->window()->Show();
            }
            break;
        case kButtonWidget:
            {
                view::Widget* widget = view::Widget::CreatePopupWidget(
                    view::Widget::Transparent,
                    view::Widget::AcceptEvents,
                    view::Widget::DeleteOnDestroy,
                    view::Widget::MirrorOriginInRTL);
                gfx::Point point(0, sender->size().height());
                view::View::ConvertPointToScreen(sender, &point);
                gfx::Rect bounds(point.x(), point.y(), 200, 300);
                widget->InitWithWidget(sender->GetWidget(), bounds);

                view::TextButton* close_button = new view::TextButton(this, L"�ر�");
                close_button->set_tag(kButtonCloseWidget);
                view::View* widget_container = new view::View();
                widget_container->set_background(view::Background::CreateBackgroundPainter(
                    true, view::Painter::CreateImagePainter(
                    ResourceBundle::GetSharedInstance().GetImageNamed(
                    IDR_TUTORIAL_WIDGET_BACKGROUND), gfx::Insets(6, 10, 6, 10), true)));
                widget_container->SetLayoutManager(new CenterLayout);
                widget_container->AddChildView(close_button);
                widget->SetContentsView(widget_container);
                widget->Show();
            }
            break;
        case kButtonClose:
            {
                window()->CloseWindow();
            }
            break;
        case kButtonCloseWidget:
            sender->GetWidget()->Close();
            break;
        }
    }
};

// �������.
int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hRes = OleInitialize(NULL);

    // this resolves ATL window thunking problem when Microsoft Layer
    // for Unicode (MSLU) is used.
    ::DefWindowProc(NULL, 0, 0, 0L);

    base::AtExitManager exit_manager;
    base::CommandLine::Init(0, NULL);

    FilePath res_dll;
    PathProvider(base::DIR_EXE, &res_dll);
    res_dll = res_dll.Append(L"wanui_res.dll");
    ResourceBundle::InitSharedInstance(res_dll);

    FilePath parent_path(L"/Users/johndoe/Library/Application Support");
    FilePath child_path(L"/Users/johndoe/Library/Application Support/Google/Chrome/Default");
    bool par = parent_path.IsParent(child_path);

    view::ViewDelegate::view_delegate = new TestViewsDelegate();
    
    view::AcceleratorHandler handler;
    MessageLoop loop(MessageLoop::TYPE_UI);
    MainWindow* main_window = new MainWindow();
    view::Window::CreateWanWindow(NULL, gfx::Rect(0, 0, 300, 300), main_window);
    view::CenterAndSizeWindow(NULL, main_window->window()->GetNativeWindow(),
        main_window->window()->GetBounds().size(), false);
    main_window->window()->Show();
    MessageLoopForUI::current()->Run(&handler);

    delete view::ViewDelegate::view_delegate;
    view::ViewDelegate::view_delegate = NULL;

    ResourceBundle::CleanupSharedInstance();
    OleUninitialize();

    return 0;
}


// ���������ؼ���ʽ.
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif