﻿#include "LongUI.h"
#include <algorithm>

// ------------------------- UIContainerBuiltIn ------------------------
// UIContainerBuiltIn: 事件处理
bool LongUI::UIContainerBuiltIn::DoEvent(const LongUI::EventArgument& arg) noexcept {
    // 处理窗口事件
    if (arg.sender) {
        switch (arg.event)
        {
        case LongUI::Event::Event_TreeBulidingFinished:
            // 初始化边缘控件 
            Super::DoEvent(arg);
            // 初次完成空间树建立
            for (auto ctrl : (*this)) {
                ctrl->DoEvent(arg);
            }
            return true;
        }
    }
    return Super::DoEvent(arg);
}

// UIContainerBuiltIn: 渲染函数
void LongUI::UIContainerBuiltIn::Render(RenderType type) const noexcept {
    // 查看
    switch (type)
    {
    case LongUI::RenderType::Type_RenderBackground:
        // 父类背景
        Super::Render(LongUI::RenderType::Type_RenderBackground);
        break;
    case LongUI::RenderType::Type_Render:
        // 父类背景
        Super::Render(LongUI::RenderType::Type_RenderBackground);
        // 普通子控件仅仅允许渲染在内容区域上
        {
            D2D1_RECT_F clip_rect; this->GetViewRect(clip_rect);
            UIManager_RenderTarget->PushAxisAlignedClip(&clip_rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }
        // 渲染所有子部件
        for (const auto* ctrl : (*this)) {
            this->child_do_render(ctrl);
        }
        // 弹出
        UIManager_RenderTarget->PopAxisAlignedClip();
        Super::Render(LongUI::RenderType::Type_Render);
        __fallthrough;
    case LongUI::RenderType::Type_RenderForeground:
        // 父类前景
        Super::Render(LongUI::RenderType::Type_RenderForeground);
        break;
    case LongUI::RenderType::Type_RenderOffScreen:
        break;
    }
}

// LongUI内建容器: 刷新
void LongUI::UIContainerBuiltIn::Update() noexcept {
    // 父类刷新呢
    Super::Update();
    // 修改可视化区域
    if (this->IsNeedRefreshWorld()) {
        // 本容器内容限制
        D2D1_RECT_F limit_of_this = {
            this->visible_rect.left + this->margin_rect.left * this->world._11,
            this->visible_rect.top + this->margin_rect.top * this->world._22,
            this->visible_rect.right - this->margin_rect.right * this->world._11,
            this->visible_rect.bottom - this->margin_rect.bottom * this->world._22,
        };
        // 更新一般控件
        for (auto ctrl : (*this)) {
            // 更新世界矩阵
            ctrl->SetControlWorldChanged();
            ctrl->RefreshWorld();
            // 坐标转换
            D2D1_RECT_F clip_rect; ctrl->GetClipRect(clip_rect);
            auto lt = LongUI::TransformPoint(ctrl->world, reinterpret_cast<D2D1_POINT_2F&>(clip_rect.left));
            auto rb = LongUI::TransformPoint(ctrl->world, reinterpret_cast<D2D1_POINT_2F&>(clip_rect.right));
            ctrl->visible_rect.left = std::max(lt.x, limit_of_this.left);
            ctrl->visible_rect.top = std::max(lt.y, limit_of_this.top);
            ctrl->visible_rect.right = std::min(rb.x, limit_of_this.right);
            ctrl->visible_rect.bottom = std::min(rb.y, limit_of_this.bottom);
            // 调试信息
            //UIManager << DL_Hint << ctrl << ctrl->visible_rect << endl;
        }
        // 调试信息
        if (this->IsTopLevel()) {
            //UIManager << DL_Log << "Handle: ControlSizeChanged" << LongUI::endl;
        }
        // 已处理该消息
        this->ControlSizeChangeHandled();
    }
    // 刷新一般子控件
    for (auto ctrl : (*this)) {
        ctrl->Update();
        ctrl->AfterUpdate();
    }
}


// UIContainerBuiltIn: 重建
auto LongUI::UIContainerBuiltIn::Recreate() noexcept -> HRESULT {
    HRESULT hr = S_OK;
    // 重建子控件
    for (auto ctrl : (*this)) {
        hr = ctrl->Recreate();
        assert(SUCCEEDED(hr));
    }
    return Super::Recreate();
}


/// <summary>
/// Find the control via mouse point
/// </summary>
/// <param name="pt">The wolrd mouse point.</param>
/// <returns>the control pointer, maybe nullptr</returns>
auto LongUI::UIContainerBuiltIn::FindControl(const D2D1_POINT_2F& pt) noexcept->UIControl* {
    // 父类(边缘控件)
    auto mctrl = Super::FindControl(pt);
    if (mctrl) return mctrl;
    // 性能警告
#ifdef _DEBUG
    if (this->GetCount() > 100) {
        UIManager << DL_Warning 
            << "Performance Warning: O(n) algorithm"
            << " is not fine for container that over 100 children" 
            << LongUI::endl;
    }
#endif
    for (auto ctrl : (*this)) {
        // 区域内判断
        if (IsPointInRect(ctrl->visible_rect, pt)) {
            return ctrl->FindControl(pt);
        }
    }
    return nullptr;
}


// UIContainerBuiltIn: 推入♂最后
void LongUI::UIContainerBuiltIn::PushBack(UIControl* child) noexcept {
    this->Insert(this->end(), child);
}

// UIContainerBuiltIn: 插入控件
void LongUI::UIContainerBuiltIn::Insert(Iterator itr, UIControl* ctrl) noexcept {
    const auto end_itr = this->end();
    assert(ctrl && "bad arguments");
    if (ctrl->prev) {
        UIManager << DL_Warning
            << L"the 'prev' attr of the control: ["
            << ctrl->GetNameStr()
            << "] that to insert is not null"
            << LongUI::endl;
    }
    if (ctrl->next) {
        UIManager << DL_Warning
            << L"the 'next' attr of the control: ["
            << ctrl->GetNameStr()
            << "] that to insert is not null"
            << LongUI::endl;
    }
    // 插入尾部?
    if (itr == end_itr) {
        // 链接
        force_cast(ctrl->prev) = m_pTail;
        // 无尾?
        if (m_pTail) force_cast(m_pTail->next) = ctrl;
        // 无头?
        if (!m_pHead) m_pHead = ctrl;
        // 设置尾
        m_pTail = ctrl;
    }
    else {
        force_cast(ctrl->next) = itr.Ptr();
        force_cast(ctrl->prev) = itr->prev;
        if (itr->prev) {
            force_cast(itr->prev) = ctrl;
        }
        force_cast(itr->prev) = ctrl;
    }
    ++m_cChildrenCount;
    // 添加之后的处理
    this->after_insert(ctrl);
}


// UIContainerBuiltIn: 仅移除控件
void LongUI::UIContainerBuiltIn::RemoveJust(UIControl* ctrl) noexcept {
    // 检查是否属于本容器
#ifdef _DEBUG
    bool ok = false;
    for (auto i : (*this)) {
        if (ctrl == i) {
            ok = true;
            break;
        }
    }
    if (!ok) {
        UIManager << DL_Error << "control:[" << ctrl->GetNameStr()
            << "] not in this container: " << this->GetNameStr() << LongUI::endl;
        return;
    }
#endif
    {
        // 连接前后节点
        register auto prev_tmp = ctrl->prev;
        register auto next_tmp = ctrl->next;
        // 检查, 头
        (prev_tmp ? force_cast(prev_tmp->next) : m_pHead) = next_tmp;
        // 检查, 尾
        (next_tmp ? force_cast(next_tmp->prev) : m_pTail) = prev_tmp;
        // 减少
        force_cast(ctrl->prev) = force_cast(ctrl->next) = nullptr;
        --m_cChildrenCount;
        // 修改
        this->SetControlSizeChanged();
    }
}


// UIContainerBuiltIn: 析构函数
LongUI::UIContainerBuiltIn::~UIContainerBuiltIn() noexcept {
    // 关闭子控件
    auto ctrl = m_pHead;
    while (ctrl) {
        auto next_ctrl = ctrl->next;
        ctrl->Cleanup();
        ctrl = next_ctrl;
    }
}

// -------------------------- UIVerticalLayout -------------------------
// UIVerticalLayout 创建
auto LongUI::UIVerticalLayout::CreateControl(CreateEventType type, pugi::xml_node node) noexcept ->UIControl* {
    UIControl* pControl = nullptr;
    switch (type)
    {
    case Type_CreateControl:
        if (!node) {
            UIManager << DL_Warning << L"node null" << LongUI::endl;
        }
        // 申请空间
        pControl = LongUI::UIControl::AllocRealControl<LongUI::UIVerticalLayout>(
            node,
            [=](void* p) noexcept { new(p) UIVerticalLayout(node); }
        );
        if (!pControl) {
            UIManager << DL_Error << L"alloc null" << LongUI::endl;
        }
        break;
    case LongUI::Type_Initialize:
        break;
    case LongUI::Type_Recreate:
        break;
    case LongUI::Type_Uninitialize:
        break;
    }
    return pControl;
}

// 更新子控件布局
void LongUI::UIVerticalLayout::RefreshLayout() noexcept {
    // 基本算法:
    // 1. 去除浮动控件影响
    // 2. 一次遍历, 检查指定高度的控件, 计算基本高度/宽度
    // 3. 计算实际高度/宽度
    if (this->IsControlSizeChanged()) {
        // 初始化
        float base_width = 0.f, base_height = 0.f, basic_weight = 0.f;
        // 第一次遍历
        for (auto ctrl : (*this)) {
            // 非浮点控件
            if (!(ctrl->flags & Flag_Floating)) {
                // 宽度固定?
                if (ctrl->flags & Flag_WidthFixed) {
                    base_width = std::max(base_width, ctrl->GetTakingUpWidth());
                }
                // 高度固定?
                if (ctrl->flags & Flag_HeightFixed) {
                    base_height += ctrl->GetTakingUpHeight();
                }
                // 未指定高度?
                else {
                    basic_weight += ctrl->weight;
                }
            }
        }
        // 带入控件本身宽度计算
        base_width = std::max(base_width, this->GetViewWidthZoomed());
        // 剩余高度富余
        register auto height_remain = std::max(this->GetViewHeightZoomed() - base_height, 0.f);
        // 单位权重高度
        auto height_in_unit_weight = basic_weight > 0.f ? height_remain / basic_weight : 0.f;
        // 基线Y
        float position_y = 0.f;
        // 第二次
        for (auto ctrl : (*this)) {
            // 浮点控
            if (ctrl->flags & Flag_Floating) continue;
            // 设置控件宽度
            if (!(ctrl->flags & Flag_WidthFixed)) {
                ctrl->SetWidth(base_width);
            }
            // 设置控件高度
            if (!(ctrl->flags & Flag_HeightFixed)) {
                ctrl->SetHeight(std::max(height_in_unit_weight * ctrl->weight, float(LongUIAutoControlMinSize)));
            }
            // 容器?
            // 不管如何, 修改!
            ctrl->SetControlSizeChanged();
            ctrl->SetLeft(0.f);
            ctrl->SetTop(position_y);
            position_y += ctrl->GetTakingUpHeight();
        }
        // 修改
        m_2fContentSize.width = base_width * m_2fZoom.width;
        m_2fContentSize.height = position_y * m_2fZoom.height;
    }
    this->RefreshWorld();
}

// UIVerticalLayout 关闭控件
void LongUI::UIVerticalLayout::Cleanup() noexcept {
    delete this;
}

// -------------------------- UIHorizontalLayout -------------------------
// UIHorizontalLayout 创建
auto LongUI::UIHorizontalLayout::CreateControl(CreateEventType type, pugi::xml_node node) noexcept ->UIControl* {
    UIControl* pControl = nullptr;
    switch (type)
    {
    case Type_CreateControl:
        if (!node) {
            UIManager << DL_Warning << L"node null" << LongUI::endl;
        }
        // 申请空间
        pControl = LongUI::UIControl::AllocRealControl<LongUI::UIHorizontalLayout>(
            node,
            [=](void* p) noexcept { new(p) UIHorizontalLayout(node); }
        );
        if (!pControl) {
            UIManager << DL_Error << L"alloc null" << LongUI::endl;
        }
        break;
    case LongUI::Type_Initialize:
        break;
    case LongUI::Type_Recreate:
        break;
    case LongUI::Type_Uninitialize:
        break;
    }
    return pControl;
}


// 更新子控件布局
void LongUI::UIHorizontalLayout::RefreshLayout() noexcept {
    // 基本算法:
    // 1. 去除浮动控件影响
    // 2. 一次遍历, 检查指定高度的控件, 计算基本高度/宽度
    // 3. 计算实际高度/宽度
    if (this->IsControlSizeChanged()) {
        // 初始化
        float base_width = 0.f, base_height = 0.f;
        float basic_weight = 0.f;
        // 第一次
        for (auto ctrl : (*this)) {
            // 前向
            ctrl->Update();
            // 非浮点控件
            if (!(ctrl->flags & Flag_Floating)) {
                // 高度固定?
                if (ctrl->flags & Flag_HeightFixed) {
                    base_height = std::max(base_height, ctrl->GetTakingUpHeight());
                }
                // 宽度固定?
                if (ctrl->flags & Flag_WidthFixed) {
                    base_width += ctrl->GetTakingUpWidth();
                }
                // 未指定宽度?
                else {
                    basic_weight += ctrl->weight;
                }
            }
        }
        // 计算
        base_height = std::max(base_height, this->GetViewHeightZoomed());
        // 剩余宽度富余
        register auto width_remain = std::max(this->GetViewWidthZoomed() - base_width, 0.f);
        // 单位权重宽度
        auto width_in_unit_weight = basic_weight > 0.f ? width_remain / basic_weight : 0.f;
        // 基线X
        float position_x = 0.f;
        // 第二次
        for (auto ctrl : (*this)) {
            // 跳过浮动控件
            if (ctrl->flags & Flag_Floating) continue;
            // 设置控件高度
            if (!(ctrl->flags & Flag_HeightFixed)) {
                ctrl->SetHeight(base_height);
            }
            // 设置控件宽度
            if (!(ctrl->flags & Flag_WidthFixed)) {
                ctrl->SetWidth(std::max(width_in_unit_weight * ctrl->weight, float(LongUIAutoControlMinSize)));
            }
            // 不管如何, 修改!
            ctrl->SetControlSizeChanged();
            ctrl->SetLeft(position_x);
            ctrl->SetTop(0.f);
            position_x += ctrl->GetTakingUpWidth();
        }
        // 修改
        m_2fContentSize.width = position_x;
        m_2fContentSize.height = base_height;
        // 已经处理
        this->ControlSizeChangeHandled();
    }
}


// UIHorizontalLayout 关闭控件
void LongUI::UIHorizontalLayout::Cleanup() noexcept {
    delete this;
}
