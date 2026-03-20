#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Libs/Window/WindowHelper.h"
#include <string>
#include "Libs/Config/Config.h"
#include "Libs/Networking/LCEOnlineServices.h"


static WindowHelper* mainWindow = NULL;
static Config* configSystem = NULL;

static bool showLoginScreen = false;

void drawWindowContent();
void drawBackgroundContent();
//std::string GetMachineGuid(); //could be better but i dont give a shit just needs to be basic

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* pCmdLine, int nCmdShow) {
    configSystem = new Config("MCLEMPLauncher");

    mainWindow = new WindowHelper("Minecraft Legacy MP Launcher", "LAUNCHER");
    mainWindow->onContentDraw = drawWindowContent;
    mainWindow->onBackgroundDraw = drawBackgroundContent;

    showLoginScreen = !(LCEOnlineServices::DoesHaveAccount() && LCEOnlineServices::isAccountValid());

    //this will capture main thread and prevent code below from calling till program exit
    mainWindow->StartWindow(hInstance); 

    //do any cleanup here

    delete configSystem;
    delete mainWindow;
}

namespace CustomGUI {
    bool Button(const char* label, const ImVec2& size = ImVec2(0, 0)) {
        if (size.x == 0 && size.y == 0) {
            ImFont* font = ImGui::GetFont();
            int fontSize = font->FontSize * 0.50f;

            ImVec2 TextSize = font->CalcTextSizeA(fontSize, 5000, 5000, label);
            float padding = 15;

            ImVec2 ButtonSize = ImVec2(TextSize.x + (padding * 2), TextSize.y + padding);
            ImVec2 CursorPos = ImGui::GetCursorPos();

            bool realButtonValue = ImGui::InvisibleButton(label, ButtonSize);

            ImU32 buttonColor = ImGui::GetColorU32(ImVec4(0.8, 0.8, 0.8, 0.9));
            ImU32 borderColor = ImGui::GetColorU32(ImVec4(0.1, 0.1, 0.1, 0.9));

            ImU32 textColor = ImGui::GetColorU32(ImVec4(0.2, 0.2, 0.2, 0.9));

            if (realButtonValue || ImGui::IsItemHovered()) {
                buttonColor = ImGui::GetColorU32(ImVec4(0, 0.8, 0, 0.9));
                borderColor = ImGui::GetColorU32(ImVec4(0.9, 0.9, 0.9, 0.9));

                textColor = ImGui::GetColorU32(ImVec4(0.9, 0.9, 0.9, 0.9));

            }

            ImDrawList* drawList = ImGui::GetForegroundDrawList();

            drawList->AddRectFilled(ImVec2(CursorPos.x, CursorPos.y + 30), ImVec2(CursorPos.x + ButtonSize.x, (CursorPos.y + 32.5) + ButtonSize.y), buttonColor);
            drawList->AddRect(ImVec2(CursorPos.x, CursorPos.y + 30), ImVec2(CursorPos.x + ButtonSize.x, (CursorPos.y + 32.5) + ButtonSize.y), borderColor, 0, 0, 1);

            drawList->AddText(font, fontSize, ImVec2(CursorPos.x + padding, (CursorPos.y + 32.5) + (padding / 2)), textColor, label);

            return realButtonValue;
        }
    }
}

void drawBackgroundContent() { }


void drawWindowContent() {
    ImGuiIO& io = ImGui::GetIO();
    //RECT title_bar_rect = WindowHelper::win32_titlebar_rect(mainWindow->getWindowHandle()); 

    if (showLoginScreen) {
        static std::string ErrorString = "";

        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (150), (io.DisplaySize.y / 4) - 10));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 0.9f));
        if (!ErrorString.empty()) ImGui::Text(ErrorString.c_str());
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3, 0.3, 0.3, 0.75));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.35, 0.35, 0.35, 0.75));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.35, 0.35, 0.35, 0.75));

        static char username_inputBuffer[16];
        static char password_inputBuffer[64];

        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (150), io.DisplaySize.y / 4));
        ImGui::PushItemWidth(300);
        ImGui::InputText("##username", username_inputBuffer, 16);

        if (std::string(username_inputBuffer).empty()) {
            ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (150) + 5, (io.DisplaySize.y / 4) + 2));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 0.75));
            ImGui::Text("Username");
            ImGui::PopStyleColor();
        }

        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (150), (io.DisplaySize.y / 4) + 35));
        ImGui::PushItemWidth(300);
        ImGui::InputText("##password", password_inputBuffer, 64, ImGuiInputTextFlags_Password);

        if (std::string(password_inputBuffer).empty()) {
            ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (150) + 5, ((io.DisplaySize.y / 4) + 35) + 2));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 0.75));
            ImGui::Text("Password");
            ImGui::PopStyleColor();
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - 150, (io.DisplaySize.y / 4) + 70));

        if (CustomGUI::Button("Register")) {


        }

        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) + 82, (io.DisplaySize.y / 4) + 70));

        if (CustomGUI::Button("Login")) {
            LCEOnlineServices::APIResponse response = LCEOnlineServices::AttemptAccountLogin(username_inputBuffer, password_inputBuffer);

            if (response.wasError) {
                ErrorString = response.GetData();
            } else {

            }
        }
    }


    //ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    //RECT title_bar_rect = WindowHelper::win32_titlebar_rect(mainWindow->getWindowHandle());

    //drawList->AddRectFilled(ImVec2(10, title_bar_rect.bottom + 10), ImVec2(io.DisplaySize.x - 10, io.DisplaySize.y - 10), ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 0.5f)));
    //drawList->AddRect(ImVec2(10, title_bar_rect.bottom + 10), ImVec2(io.DisplaySize.x - 10, io.DisplaySize.y - 10), ImGui::GetColorU32(ImVec4(0.2f, 0.2f, 0.2f, 0.5f)), 0, 0, 1);
}


/*std::string GetMachineGuid() {
    HKEY hKey;
    const char* keyPath = "SOFTWARE\\Microsoft\\Cryptography";
    const char* valueName = "MachineGuid";
    char buffer[256];
    DWORD bufferSize = sizeof(buffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
        return "";

    if (RegQueryValueExA(hKey, valueName, nullptr, nullptr, (LPBYTE)buffer, &bufferSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return "";
    }

    RegCloseKey(hKey);
    return std::string(buffer, bufferSize - 1); // strip null
}*/