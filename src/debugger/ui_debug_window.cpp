﻿#include <control/ui_viewport.h>

#ifndef NDEBUG
#include <control/ui_button.h>
#include <control/ui_checkbox.h>
#include <core/ui_manager.h>
#include <core/ui_window.h>
#include <container/pod_vector.h>


namespace LongUI {
    // debug window view xul
    extern const char* debug_view_xul;
    // debug view
    class CUIDebugView final : public UIViewport {
        // super class
        using Super = UIViewport;
    public:
        // ctor
        CUIDebugView() noexcept :
            Super(nullptr, CUIWindow::Config_ToolWindow) {
            if (this->SetXul(debug_view_xul)) {
                const auto window = this->GetWindow();
                window->SetClearColor({ 1, 1, 1, 1 });
                window->ShowWindow();
                window->SetPos({ 0, 0 });
                this->exit();
                this->recreate();
                this->force_render();
                this->draw_text_cell();
                this->draw_dirty_rect();
                this->link_style_sheet();
                //UIManager.CreateTimeCapsule([this](float t) noexcept {
                //    if (t < 1.f) return;
                //    const auto window = this->GetWindow();
                //    const auto ctrl = window->FindControl("btn-exit");
                //    const auto btn = longui_cast<UIButton*>(ctrl);
                //    btn->SetText(L"EXIT"_sv);
                //}, 5.f, this);
            }
        }
        // dtor
        ~CUIDebugView() noexcept { }
    private:
        // do all render
        template<typename T>
        static void do_all_render(T a, const T b) noexcept {
            while (a != b) {
                a->MarkFullRendering();
                do_all_render(a->begin(), a->end());
                a++;
            }
        }
        // do button
        auto do_button(const char* name) noexcept {
            const auto window = this->GetWindow();
            const auto ctrl = window->FindControl(name);
            assert(ctrl && "404");
            const auto btn = longui_cast<UIButton*>(ctrl);
            return btn;
        }
        // do checkbox
        auto do_checkbox(const char* name) noexcept {
            const auto window = this->GetWindow();
            const auto ctrl = window->FindControl(name);
            assert(ctrl && "404");
            const auto btn = longui_cast<UICheckBox*>(ctrl);
            return btn;
        }
        // force render
        void force_render() noexcept {
            do_button("btn-force")->AddGuiEventListener(
                UIButton::_onCommand(), [](UIControl& control) noexcept {
                //const auto& o = UIManager.GetWindowList();
                //using List = POD::Vector<CUIWindow*>;
                //auto& list = reinterpret_cast<const List&>(o);
                //for (const auto x : list) x->MarkFullRendering();
                do_all_render(UIManager.begin(), UIManager.end());
                return Event_Accept;
            });
        }
        // exit
        void exit() noexcept {
            do_button("btn-exit")->AddGuiEventListener(
                UIButton::_onCommand(), [](UIControl& control) noexcept {
                UIManager.Exit();
                return Event_Accept;
            });
        }
        // recreate_device
        void recreate() noexcept {
            do_button("btn-recreate")->AddGuiEventListener(
                UIButton::_onCommand(), [](UIControl& control) noexcept {
                UIManager.NeedRecreate();
                return Event_Accept;
            });
        }
        // draw dirty rect
        void draw_dirty_rect() noexcept {
            const auto checkbox = do_checkbox("cbx-dirty");
            const auto& flag = UIManager.flag;
            checkbox->SetChecked(!!(flag & ConfigureFlag::Flag_DbgDrawDirtyRect));
            checkbox->AddGuiEventListener(
                checkbox->_onCommand(), [&flag](UIControl& control) noexcept {
                const auto box = longui_cast<UICheckBox*>(&control);
                auto& mflag = const_cast<ConfigureFlag&>(flag);
                if (box->GetChecked()) mflag = mflag | ConfigureFlag::Flag_DbgDrawDirtyRect;
                else mflag = mflag & ConfigureFlag(~ConfigureFlag::Flag_DbgDrawDirtyRect);
                return Event_Accept;
            });
        }
        // draw text cell
        void draw_text_cell() noexcept {
            const auto checkbox = do_checkbox("cbx-cell");
            const auto& flag = UIManager.flag;
            checkbox->SetChecked(!!(flag & ConfigureFlag::Flag_DbgDrawTextCell));
            checkbox->AddGuiEventListener(
                checkbox->_onCommand(), [&flag](UIControl& control) noexcept {
                const auto box = longui_cast<UICheckBox*>(&control);
                auto& mflag = const_cast<ConfigureFlag&>(flag);
                if (box->GetChecked()) mflag = mflag | ConfigureFlag::Flag_DbgDrawTextCell;
                else mflag = mflag & ConfigureFlag(~ConfigureFlag::Flag_DbgDrawTextCell);
                return Event_Accept;
            });
        }
        // link style sheet
        void link_style_sheet() noexcept {
            const auto checkbox = do_checkbox("cbx-style");
            const auto& flag = UIManager.flag;
            checkbox->SetChecked(!(flag & ConfigureFlag::Flag_DbgNoLinkStyle));
#ifdef NDEBUG
            checkbox->AddGuiEventListener(
                checkbox->_stateChanged(), [&flag](UIControl& control) noexcept {
                const auto box = longui_cast<UICheckBox*>(&control);
                auto& mflag = const_cast<CUIManager::ConfigFlag&>(flag);
                if (!box->GetChecked()) mflag = mflag | ConfigureFlag::Flag_DbgNoLinkStyle;
                else mflag = mflag & CUIManager::ConfigFlag(~ConfigureFlag::Flag_DbgNoLinkStyle);
                return Event_Accept;
            });
#endif
        }
    };
}

// LongUI::impl namespace
namespace LongUI { namespace impl {
    // create the debug window
    auto create_debug_window() noexcept -> CUIWindow * {
        const auto ptr = new(std::nothrow) CUIDebugView;
        return ptr ? ptr->GetWindow() : nullptr;
    }
}}


// xul stirng
const char* LongUI::debug_view_xul = u8R"(
<?xml version="1.0"?>
<window title="XUL Layout">
<groupbox flex="1">
    <caption label="Core Debug"/>
    <label href="https://github.com/dustpg/LongUI" value="LongUI on github"/>
    <hbox>
        <button id="btn-force" label="force render" accesskey="f"/>
        <button id="btn-recreate" label="recreate res" accesskey="r"/>
        <button id="btn-exit" label="exit" default="true" accesskey="e"/>
    </hbox>
    <checkbox id="cbx-dirty" label="draw dirty rect"/>
    <checkbox id="cbx-cell" label="draw text cell"/>
    <checkbox id="cbx-style" label="link style sheet"/>
</groupbox>
</window>)";


#endif