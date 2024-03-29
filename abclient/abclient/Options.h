#pragma once

enum class WindowMode
{
    Windowed,
    Fullcreeen,
    Borderless,
    Maximized
};

enum class AntiAliasingMode
{
    None,
    FXAA3,             /// Fast approx. AA
    MSAAx2,            /// 2x multi sample
    MSAAx4,
    MSAAx8,
    MSAAx16
};

static constexpr float MIN_FOV = 45.0;
static constexpr float MAX_FOV = 120.0;

struct Environment
{
    String name;
    String host;
    uint16_t port;
    bool selected;
};

class Options : public Object
{
    URHO3D_OBJECT(Options, Object);
public:
    Options(Context* context);
    ~Options();

    String loginHost_;
    uint16_t loginPort_{ 0 };
    String username_;
    String password_;
    Vector<Environment> environments_;

    bool stickCameraToHead_{ true };
    bool disableMouseWalking_{ false };
    bool enableMumble_{ false };
    float mouseSensitivity_{ 0.1f };

    float gainMaster_{ 1.0f };
    float gainEffect_{ 1.0f };
    float gainAmbient_{ 1.0f };
    float gainVoice_{ 1.0f };
    float gainMusic_{ 0.1f };

    void Load();
    void Save();

    Environment* GetEnvironmment(const String& name);
    Environment* GetSelectedEnvironment();
    void SetSelectedEnvironment(const String& name);
    WindowMode GetWindowMode() const;
    void SetWindowMode(WindowMode mode);
    int GetWidth() const
    {
        return width_;
    }
    void SetWidth(int value);
    int GetHeight() const
    {
        return height_;
    }
    void SetHeight(int value);
    bool GetFullscreen() const
    {
        return fullscreen_;
    }
    void SetFullscreen(bool value);
    bool GetBorderless() const
    {
        return borderless_;
    }
    void SetBorderless(bool value);
    bool GetResizeable() const
    {
        return resizeable_;
    }
    void SetResizeable(bool value);
    bool IsMiximized() const
    {
        return maximized_;
    }
    bool GetVSync() const
    {
        return vSync_;
    }
    void SetVSync(bool value);
    int GetMaxFps() const
    {
        return maxFps_ > 0 ? maxFps_ : 200;
    }
    void SetMaxFps(int value);
    bool GetTripleBuffer() const
    {
        return tripleBuffer_;
    }
    void SetTripleBuffer(bool value);
    bool GetHighDPI() const
    {
        return highDPI_;
    }
    void SetHighDPI(bool value);
    int GetMultiSample() const
    {
        switch (antiAliasingMode_)
        {
        case AntiAliasingMode::MSAAx2:
            return 2;
        case AntiAliasingMode::MSAAx4:
            return 4;
        case AntiAliasingMode::MSAAx8:
            return 8;
        case AntiAliasingMode::MSAAx16:
            return 16;
        default:
            return 1;
        }
    }
    void SetShadowQuality(ShadowQuality quality);
    ShadowQuality GetShadowQuality() const
    {
        return shadowQuality_;
    }
    /// Set texture quality level. See the QUALITY constants in GraphicsDefs.h.
    void SetTextureQuality(MaterialQuality quality);
    MaterialQuality GetTextureQuality() const
    {
        return textureQuality_;
    }
    /// Set material quality level. See the QUALITY constants in GraphicsDefs.h.
    void SetMaterialQuality(MaterialQuality quality);
    MaterialQuality GetMaterialQuality() const
    {
        return materialQuality_;
    }
    TextureFilterMode GetTextureFilterMode() const
    {
        return textureFilterMode_;
    }
    void SetTextureFilterMode(TextureFilterMode value);
    int GetTextureAnisotropyLevel() const
    {
        return textureAnisotropyLevel_;
    }
    bool GetSpecularLightning() const
    {
        return specularLightning_;
    }
    void SetSpecularLightning(bool value);
    bool GetHDRRendering() const
    {
        return hdrRendering_;
    }
    void SetHDRRendering(bool value);
    void SetTextureAnisotropyLevel(int value);
    bool GetShadows() const
    {
        return shadows_;
    }
    void SetShadows(bool value);
    float GetCameraFarClip() const
    {
        return cameraFarClip_;
    }
    float GetCameraNearClip() const
    {
        return cameryNearClip_;
    }
    float GetCameraFov() const
    {
        return cameraFov_;
    }
    void SetCameraFov(float value);
    AntiAliasingMode GetAntiAliasingMode() const
    {
        return antiAliasingMode_;
    }
    void SetAntiAliasingMode(AntiAliasingMode mode);
    const IntVector2& GetWindowPos() const
    {
        return windowPos_;
    }

    const String& GetRenderPath() const;

    void UpdateAudio();
    /// Toggle
    void MuteAudio();
    void LoadWindow(UIElement* window);
    void SaveWindow(UIElement* window);

    static const String& GetPrefPath();
    static void SetPrefPath(const String& value);
    static bool CreateDir(const String& path);
private:
    static String prefPath_;
    IntVector2 oldWindowPos_;
    IntVector2 windowPos_;
    int width_{ 0 };
    int height_{ 0 };
    bool fullscreen_{ true };
    bool borderless_{ false };
    bool resizeable_{ false };
    bool maximized_{ false };
    bool internalMaximized_{ false };
    bool vSync_{ false };
    int maxFps_{ 200 };
    bool tripleBuffer_{ false };
    bool highDPI_{ false };
    bool shadows_{ true };
    float cameraFarClip_{ 300.0f };
    float cameryNearClip_{ 0.0f };
    float cameraFov_{ 60.0f };
    ShadowQuality shadowQuality_{ SHADOWQUALITY_VSM };
    MaterialQuality textureQuality_{ QUALITY_HIGH };
    MaterialQuality materialQuality_{ QUALITY_HIGH };
    TextureFilterMode textureFilterMode_{ FILTER_ANISOTROPIC };
    int textureAnisotropyLevel_{ 16 };
    AntiAliasingMode antiAliasingMode_{ AntiAliasingMode::FXAA3 };
    bool specularLightning_{ true };
    bool hdrRendering_{ false };
    // "RenderPaths/Prepass.xml";
    // "RenderPaths/Deferred.xml";
    String renderPath_;

    void UpdateGraphicsMode();
    void LoadSettings();
    void LoadElements(const XMLElement& root);
    void HandleInputFocus(StringHash eventType, VariantMap& eventData);
};

