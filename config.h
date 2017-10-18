#ifndef __CONFIG_H
#define __CONFIG_H

#define AIMBOT_KEY VK_XBUTTON1
#define CHAMS_KEY VK_F8
#define ESP_KEY VK_F9

//

#define DEFAULT_FONT L"Verdana"
#define ENEMY_TEXT_COLOR Color{ 0.9f, 0.9f, 0.15f, 0.95f }

class Config
{
    public:
    bool m_EnableESP = true;
    bool m_EnableChams = true;
    float m_AimbotFieldOfViewPixels = 100.0f;
    float m_MaxAimbotLockDistance = 6000.0f;
    float m_MinAimbotHeadshotDistance = 600.0f;
    float m_AimbotSmoothing = 400.0f;
    float m_MaxESPRange = 8000.0f; // may cause crashes if set to a large value
    unsigned int m_MaxESPLabelsCount = 256u;
    int m_AutofireShotDelayMilliseconds = 25;
};

#endif
