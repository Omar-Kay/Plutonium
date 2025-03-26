#include <pu/ui/render/render_Renderer.hpp>

namespace pu::ui::render {

namespace {

// Global rendering vars
sdl2::Renderer g_Renderer = nullptr;
sdl2::Window g_Window = nullptr;
sdl2::Surface g_WindowSurface = nullptr;

// Global font object
std::vector<std::pair<std::string, std::shared_ptr<ttf::Font>>> g_FontTable;

inline bool ExistsFont(const std::string& font_name) {
    for (const auto& [name, font] : g_FontTable) {
        if (name == font_name) {
            return true;
        }
    }

    return false;
}

}  // namespace

void Renderer::Initialize() {
    if (!this->initialized) {
        this->ttf_init = false;

        if (this->init_opts.init_romfs) {
            this->ok_romfs = R_SUCCEEDED(romfsInit());
        }

        if (!this->init_opts.default_shared_fonts.empty()) {
            // TODO: choose pl service type?
            this->ok_pl = R_SUCCEEDED(plInitialize(PlServiceType_User));
        }

        padConfigureInput(this->init_opts.pad_player_count, this->init_opts.pad_style_tag);
        padInitializeWithMask(&this->input_pad, this->init_opts.pad_id_mask);

        // TODO: check sdl return errcodes!

        SDL_Init(this->init_opts.sdl_flags);
        g_Window = SDL_CreateWindow("Plutonium-SDL2", 0, 0, this->init_opts.width, this->init_opts.height, 0);
        g_Renderer = SDL_CreateRenderer(g_Window, -1, this->init_opts.sdl_render_flags);
        g_WindowSurface = SDL_GetWindowSurface(g_Window);
        SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");

        if (this->init_opts.init_img) {
            IMG_Init(this->init_opts.sdl_img_flags);
        }

        if (!this->init_opts.default_shared_fonts.empty() || !this->init_opts.default_font_paths.empty()) {
            TTF_Init();
            this->ttf_init = true;

#define _CREATE_DEFAULT_FONT_FOR_SIZES(sizes)                              \
    {                                                                      \
        for (const auto size : sizes) {                                    \
            auto default_font = std::make_shared<ttf::Font>(size);         \
            for (const auto& path : this->init_opts.default_font_paths) {  \
                default_font->LoadFromFile(path);                          \
            }                                                              \
            for (const auto type : this->init_opts.default_shared_fonts) { \
                LoadSingleSharedFontInFont(default_font, type);            \
            }                                                              \
            AddDefaultFont(default_font);                                  \
        }                                                                  \
    }

            _CREATE_DEFAULT_FONT_FOR_SIZES(DefaultFontSizes);
            _CREATE_DEFAULT_FONT_FOR_SIZES(this->init_opts.extra_default_font_sizes);
        }

        if (this->init_opts.init_mixer) {
            Mix_Init(this->init_opts.audio_mixer_flags);
            Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
        }

        this->initialized = true;
        this->base_a = TextureRenderOptions::NoAlpha;
        this->base_x = 0;
        this->base_y = 0;
    }
}

void Renderer::Finalize() {
    if (this->initialized) {
        // Close all the fonts before closing TTF
        g_FontTable.clear();

        if (this->ttf_init) {
            TTF_Quit();
        }
        if (this->init_opts.init_img) {
            IMG_Quit();
        }
        if (this->init_opts.init_mixer) {
            Mix_CloseAudio();
        }
        if (this->ok_pl) {
            plExit();
        }
        if (this->ok_romfs) {
            romfsExit();
        }
        SDL_DestroyRenderer(g_Renderer);
        SDL_FreeSurface(g_WindowSurface);
        SDL_DestroyWindow(g_Window);
        SDL_Quit();
        this->initialized = false;
    }
}

void Renderer::InitializeRender(const Color clr) {
    SDL_SetRenderDrawColor(g_Renderer, clr.r, clr.g, clr.b, clr.a);
    SDL_RenderClear(g_Renderer);
}

void Renderer::FinalizeRender() {
    SDL_RenderPresent(g_Renderer);
}

void Renderer::RenderTexture(sdl2::Texture texture, const i32 x, const i32 y, const TextureRenderOptions opts) {
    if (texture == nullptr) {
        return;
    }

    SDL_Rect pos = {.x = x + this->base_x, .y = y + this->base_y};
    if (opts.width != TextureRenderOptions::NoWidth) {
        pos.w = opts.width;
    } else {
        SDL_QueryTexture(texture, nullptr, nullptr, &pos.w, nullptr);
    }
    if (opts.height != TextureRenderOptions::NoHeight) {
        pos.h = opts.height;
    } else {
        SDL_QueryTexture(texture, nullptr, nullptr, nullptr, &pos.h);
    }

    float angle = 0;
    if (opts.rot_angle != TextureRenderOptions::NoRotation) {
        angle = opts.rot_angle;
    }

    const auto has_alpha_mod = opts.alpha_mod != TextureRenderOptions::NoAlpha;
    if (has_alpha_mod) {
        SetAlphaValue(texture, static_cast<u8>(opts.alpha_mod));
    }
    if (this->base_a >= 0) {
        SetAlphaValue(texture, static_cast<u8>(this->base_a));
    }

    SDL_RenderCopyEx(g_Renderer, texture, nullptr, &pos, angle, nullptr, SDL_FLIP_NONE);

    if (has_alpha_mod || (this->base_a >= 0)) {
        // Aka unset alpha value, needed if the same texture is rendered several times with different alphas
        SetAlphaValue(texture, 0xFF);
    }
}

void Renderer::RenderRectangle(const Color clr, const i32 x, const i32 y, const i32 width, const i32 height) {
    const SDL_Rect rect = {.x = x + this->base_x, .y = y + this->base_y, .w = width, .h = height};
    SDL_SetRenderDrawColor(g_Renderer, clr.r, clr.g, clr.b, this->GetActualAlpha(clr.a));
    SDL_RenderDrawRect(g_Renderer, &rect);
}

void Renderer::RenderRectangleFill(const Color clr, const i32 x, const i32 y, const i32 width, const i32 height) {
    const SDL_Rect rect = {.x = x + this->base_x, .y = y + this->base_y, .w = width, .h = height};
    SDL_SetRenderDrawColor(g_Renderer, clr.r, clr.g, clr.b, this->GetActualAlpha(clr.a));
    SDL_RenderFillRect(g_Renderer, &rect);
}

void Renderer::RenderRoundedRectangle(
    const Color clr,
    const i32 x,
    const i32 y,
    const i32 width,
    const i32 height,
    const i32 radius
) {
    auto proper_radius = radius;
    if ((2 * proper_radius) > width) {
        proper_radius = width / 2;
    }
    if ((2 * proper_radius) > height) {
        proper_radius = height / 2;
    }

    roundedRectangleRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        x + this->base_x + width,
        y + this->base_y + height,
        proper_radius,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
}

void Renderer::RenderRoundedRectangleFill(
    const Color clr,
    const i32 x,
    const i32 y,
    const i32 width,
    const i32 height,
    const i32 radius
) {
    auto proper_radius = radius;
    if ((2 * proper_radius) > width) {
        proper_radius = width / 2;
    }
    if ((2 * proper_radius) > height) {
        proper_radius = height / 2;
    }

    roundedBoxRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        x + this->base_x + width,
        y + this->base_y + height,
        proper_radius,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
    SDL_SetRenderDrawBlendMode(g_Renderer, SDL_BLENDMODE_BLEND);
}

void Renderer::RenderCircle(const Color clr, const i32 x, const i32 y, const i32 radius) {
    circleRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        radius - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
    aacircleRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        radius - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
}

void Renderer::RenderCircleFill(const Color clr, const i32 x, const i32 y, const i32 radius) {
    filledCircleRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        radius - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
    aacircleRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        radius - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
}

void Renderer::RenderEllipse(const Color clr, const i32 x, const i32 y, const i32 rx, const i32 ry) {
    ellipseRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        rx - 1,
        ry - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
    aaellipseRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        rx - 1,
        ry - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
}

void Renderer::RenderEllipseFill(const Color clr, const i32 x, const i32 y, const i32 rx, const i32 ry) {
    filledEllipseRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        rx - 1,
        ry - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
    aaellipseRGBA(
        g_Renderer,
        x + this->base_x,
        y + this->base_y,
        rx - 1,
        ry - 1,
        clr.r,
        clr.g,
        clr.b,
        this->GetActualAlpha(clr.a)
    );
}

void Renderer::RenderShadowSimple(
    const i32 x,
    const i32 y,
    const i32 width,
    const i32 height,
    const i32 base_alpha,
    const u8 main_alpha
) {
    auto crop = false;
    auto shadow_width = width;
    auto shadow_x = x;
    auto shadow_y = y;
    for (auto cur_a = base_alpha; cur_a > 0; cur_a -= (180 / height)) {
        const Color shadow_clr = {130, 130, 130, static_cast<u8>(cur_a * (main_alpha / 0xFF))};
        this->RenderRectangleFill(shadow_clr, shadow_x + this->base_x, shadow_y + this->base_y, shadow_width, 1);
        if (crop) {
            shadow_width -= 2;
            shadow_x++;
        }
        crop = !crop;
        shadow_y++;
    }
}

sdl2::Renderer GetMainRenderer() {
    return g_Renderer;
}

sdl2::Window GetMainWindow() {
    return g_Window;
}

sdl2::Surface GetMainSurface() {
    return g_WindowSurface;
}

std::pair<u32, u32> GetDimensions() {
    i32 w = 0;
    i32 h = 0;
    SDL_GetWindowSize(g_Window, &w, &h);
    return {static_cast<u32>(w), static_cast<u32>(h)};
}

bool AddFont(const std::string& font_name, std::shared_ptr<ttf::Font>& font) {
    if (ExistsFont(font_name)) {
        return false;
    }

    g_FontTable.push_back(std::make_pair(font_name, std::move(font)));
    return true;
}

bool LoadSingleSharedFontInFont(std::shared_ptr<ttf::Font>& font, const PlSharedFontType type) {
    // Assume pl services are initialized, and return if anything unexpected happens
    PlFontData data = {};
    if (R_FAILED(plGetSharedFontByType(&data, type))) {
        return false;
    }
    if (!ttf::Font::IsValidFontFaceIndex(
            font->LoadFromMemory(data.address, data.size, ttf::Font::EmptyFontFaceDisposingFunction)
        )) {
        return false;
    }

    return true;
}

bool LoadAllSharedFontsInFont(std::shared_ptr<ttf::Font>& font) {
    for (u32 i = 0; i < PlSharedFontType_Total; i++) {
        if (!LoadSingleSharedFontInFont(font, static_cast<PlSharedFontType>(i))) {
            return false;
        }
    }
    return true;
}

bool GetTextDimensions(const std::string& font_name, const std::string& text, i32& out_width, i32& out_height) {
    for (auto& [name, font] : g_FontTable) {
        if (name == font_name) {
            const auto [w, h] = font->GetTextDimensions(text);
            out_width = w;
            out_height = h;
            return true;
        }
    }
    return false;
}

i32 GetTextWidth(const std::string& font_name, const std::string& text) {
    i32 width = 0;
    i32 dummy;
    GetTextDimensions(font_name, text, width, dummy);
    return width;
}

i32 GetTextHeight(const std::string& font_name, const std::string& text) {
    i32 dummy;
    i32 height = 0;
    GetTextDimensions(font_name, text, dummy, height);
    return height;
}

sdl2::Texture RenderText(
    const std::string& font_name,
    const std::string& text,
    const Color clr,
    const u32 max_width,
    const u32 max_height
) {
    for (auto& [name, font] : g_FontTable) {
        if (name == font_name) {
            auto text_tex = font->RenderText(text, clr);

            if ((max_width > 0) || (max_height > 0)) {
                auto cur_text = text;
                auto cur_width = GetTextureWidth(text_tex);
                auto cur_height = GetTextureHeight(text_tex);
                while (true) {
                    if (cur_text.empty()) {
                        break;
                    }
                    if ((max_width > 0) && (cur_width <= (i32)max_width)) {
                        break;
                    }
                    if ((max_height > 0) && (cur_height <= (i32)max_height)) {
                        break;
                    }

                    cur_text.pop_back();
                    DeleteTexture(text_tex);
                    text_tex = font->RenderText(cur_text + "...", clr);
                    cur_width = GetTextureWidth(text_tex);
                    cur_height = GetTextureHeight(text_tex);
                }
            }

            return text_tex;
        }
    }

    return nullptr;
}

}  // namespace pu::ui::render
