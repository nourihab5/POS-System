#pragma once
#include "utils/SystemManager.h"
namespace AdminDashboard {
    void Open();
    void Close();
    bool IsOpen();
    void Render(SystemManager& manager, float scale_factor = 1.5f);
}