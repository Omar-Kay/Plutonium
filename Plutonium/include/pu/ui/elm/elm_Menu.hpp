
/*

    Plutonium library

    @file Menu.hpp
    @brief A Menu is a very useful Element for option browsing or selecting.
    @author XorTroll

    @copyright Plutonium project - an easy-to-use UI framework for Nintendo Switch homebrew

*/

#pragma once
#include <pu/ui/elm/elm_Element.hpp>
#include <chrono>
#include <functional>

namespace pu::ui::elm {

    class MenuItem {
        public:
            using OnKeyCallback = std::function<void()>;
            static constexpr Color DefaultColor = { 10, 10, 10, 0xFF };

        private:
            std::string name;
            Color clr;
            sdl2::TextureHandle::Ref icon;
            std::vector<OnKeyCallback> on_key_cbs;
            std::vector<u64> on_key_cb_keys;

        public:
            MenuItem(const std::string &name) : name(name), clr(DefaultColor) {}
            PU_SMART_CTOR(MenuItem)

            inline std::string GetName() {
                return this->name;
            }
            
            inline void SetName(const std::string &name) {
                this->name = name;
            }

            PU_CLASS_POD_GETSET(Color, clr, Color)

            void AddOnKey(OnKeyCallback on_key_cb, const u64 key = HidNpadButton_A);
            
            inline u32 GetOnKeyCallbackCount() {
                return this->on_key_cbs.size();
            }

            inline OnKeyCallback GetOnKeyCallback(const u32 idx) {
                if(idx < this->on_key_cbs.size()) {
                    return this->on_key_cbs.at(idx);
                }
                else {
                    return {};
                }
            }

            inline u64 GetOnKeyCallbackKey(const u32 idx) {
                if(idx < this->on_key_cb_keys.size()) {
                    return this->on_key_cb_keys.at(idx);
                }
                else {
                    return {};
                }
            }

            inline sdl2::TextureHandle::Ref GetIconTexture() {
                return this->icon;
            }

            void SetIcon(sdl2::TextureHandle::Ref icon);

            inline bool HasIcon() {
                return this->icon != nullptr;
            }
    };

    class Menu : public Element {
        public:
            static constexpr Color DefaultScrollbarColor = { 110, 110, 110, 0xFF };

            static constexpr u8 DefaultItemAlphaIncrementSteps = 15;

            static constexpr float DefaultIconItemSizesFactor = 0.8f;

            static constexpr u32 DefaultIconMargin = 37;
            static constexpr u32 DefaultTextMargin = 37;

            static constexpr u8 DefaultLightScrollbarColorFactor = 30;

            static constexpr u32 DefaultScrollbarWidth = 30;

            static constexpr u32 DefaultShadowHeight = 7;
            static constexpr u8 DefaultShadowBaseAlpha = 160;

            static constexpr s64 DefaultMoveWaitTimeMs = 150;

            enum class MoveStatus : u8 {
                None = 0,
                WaitingUp = 1,
                WaitingDown = 2
            };

            using OnSelectionChangedCallback = std::function<void()>;

        protected:
            u32 selected_item_idx;
            i32 prev_selected_item_idx;

        private:
            i32 x;
            i32 y;
            i32 w;
            i32 items_h;
            u32 items_to_show;
            i32 selected_item_alpha;
            SigmoidIncrementer<i32> selected_item_alpha_incr;
            i32 prev_selected_item_alpha;
            SigmoidIncrementer<i32> prev_selected_item_alpha_incr;
            u32 advanced_item_count;
            Color scrollbar_clr;
            Color items_clr;
            Color items_focus_clr;
            bool cooldown_enabled;
            bool item_touched;
            MoveStatus move_status;
            std::chrono::time_point<std::chrono::steady_clock> move_start_time;
            OnSelectionChangedCallback on_selection_changed_cb;
            std::vector<MenuItem::Ref> items;
            std::string font_name;
            std::vector<sdl2::Texture> loaded_name_texs;
            u8 item_alpha_incr_steps;
            float icon_item_sizes_factor;
            u32 icon_margin;
            u32 text_margin;
            u8 light_scrollbar_color_factor;
            u32 scrollbar_width;
            u32 shadow_height;
            u8 shadow_base_alpha;
            s64 move_wait_time_ms;

            void ReloadItemRenders();
            void MoveUp();
            void MoveDown();

            inline Color MakeItemsFocusColor(const u8 alpha) {
                return this->items_focus_clr.WithAlpha(alpha);
            }

            inline constexpr Color MakeLighterScrollbarColor() {
                i32 base_r = this->scrollbar_clr.r - this->light_scrollbar_color_factor;
                if(base_r < 0) {
                    base_r = 0;
                }
                i32 base_g = this->scrollbar_clr.g - this->light_scrollbar_color_factor;
                if(base_g < 0) {
                    base_g = 0;
                }
                i32 base_b = this->scrollbar_clr.b - this->light_scrollbar_color_factor;
                if(base_b < 0) {
                    base_b = 0;
                }

                return { static_cast<u8>(base_r), static_cast<u8>(base_g), static_cast<u8>(base_b), this->scrollbar_clr.a };
            }

            inline void HandleOnSelectionChanged() {
                if(this->on_selection_changed_cb) {
                    (this->on_selection_changed_cb)();
                }
            }

            inline void RunSelectedItemCallback(const u64 keys) {
                auto item = this->items.at(this->selected_item_idx);
                const auto cb_count = item->GetOnKeyCallbackCount();
                for(u32 i = 0; i < cb_count; i++) {
                    if(keys & item->GetOnKeyCallbackKey(i)) {
                        if(!this->cooldown_enabled) {
                            auto cb = item->GetOnKeyCallback(i);
                            if(cb) {
                                cb();
                            }
                        }
                    }
                }
                this->cooldown_enabled = false;
            }

            inline u32 GetItemCount() {
                auto item_count = this->items_to_show;
                if(item_count > this->items.size()) {
                    item_count = this->items.size();
                }
                if((item_count + this->advanced_item_count) > this->items.size()) {
                    item_count = this->items.size() - this->advanced_item_count;
                }
                return item_count;
            }

        public:
            Menu(const i32 x, const i32 y, const i32 width, const Color items_clr, const Color items_focus_clr, const i32 items_height, const u32 items_to_show);
            PU_SMART_CTOR(Menu)

            inline i32 GetX() override {
                return this->x;
            }

            inline void SetX(const i32 x) {
                this->x = x;
            }

            inline i32 GetY() override {
                return this->y;
            }

            inline void SetY(const i32 y) {
                this->y = y;
            }

            inline i32 GetWidth() override {
                return this->w;
            }

            inline void SetWidth(const i32 width) {
                this->w = width;
            }

            inline i32 GetHeight() override {
                return this->items_h * this->items_to_show;
            }

            PU_CLASS_POD_GETSET(ItemsHeight, items_h, i32)
            PU_CLASS_POD_GETSET(NumberOfItemsToShow, items_to_show, i32)
            PU_CLASS_POD_GETSET(ItemsFocusColor, items_focus_clr, Color)
            PU_CLASS_POD_GETSET(ItemsColor, items_clr, Color)
            PU_CLASS_POD_GETSET(ScrollbarColor, scrollbar_clr, Color)
            PU_CLASS_POD_GETSET(ItemAlphaIncrementSteps, item_alpha_incr_steps, u8)
            PU_CLASS_POD_GETSET(IconItemSizesFactor, icon_item_sizes_factor, float)
            PU_CLASS_POD_GETSET(IconMargin, icon_margin, u32)
            PU_CLASS_POD_GETSET(TextMargin, text_margin, u32)
            PU_CLASS_POD_GETSET(LightScrollbarColorFactor, light_scrollbar_color_factor, u8)
            PU_CLASS_POD_GETSET(ScrollbarWidth, scrollbar_width, u32)
            PU_CLASS_POD_GETSET(ShadowHeight, shadow_height, u32)
            PU_CLASS_POD_GETSET(ShadowBaseAlpha, shadow_base_alpha, u8)
            PU_CLASS_POD_GETSET(MoveWaitTimeMs, move_wait_time_ms, s64)

            inline void SetOnSelectionChanged(OnSelectionChangedCallback on_selection_changed_cb) {
                this->on_selection_changed_cb = on_selection_changed_cb;
            }

            inline void AddItem(MenuItem::Ref &item) {
                this->items.push_back(item);
            }

            void ClearItems();

            inline void ForceReloadItems() {
                this->ReloadItemRenders();
            }

            PU_CLASS_POD_SET(CooldownEnabled, cooldown_enabled, bool)

            inline MenuItem::Ref &GetSelectedItem() {
                return this->items.at(this->selected_item_idx);
            }

            inline std::vector<MenuItem::Ref> &GetItems() {
                return this->items;
            }

            PU_CLASS_POD_GET(SelectedIndex, selected_item_idx, i32)

            void SetSelectedIndex(const u32 idx);

            void OnRender(render::Renderer::Ref &drawer, const i32 x, const i32 y) override;
            void OnInput(const u64 keys_down, const u64 keys_up, const u64 keys_held, const TouchPoint touch_pos) override;
    };
}
