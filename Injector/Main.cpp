#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Libs/Window/WindowHelper.h"
#include <string>
#include "Libs/Config/Config.h"
#include "Libs/Networking/LCEOnlineServices.h"
#include "Managers/VersionManager/VersionManager.h"
#include "Managers/DownloadManager/DownloadManager.h"
#include <thread>
#include "CustomRenderers.h"
#include "Libs/Networking/HTTPSHelper.h"
#include <unordered_set>


static WindowHelper* mainWindow = NULL;
static Config* configSystem = NULL;

static VersionManager* versionManager = NULL;
static std::vector<InstalledVersion> installedVersions;
static std::vector<RemoteVersion> remoteVersions;

static bool showLoginScreen = false;
static std::string rawNewsText = "";

void drawWindowContent();
void drawBackgroundContent();
//std::string GetMachineGuid(); //could be better but i dont give a shit just needs to be basic

void UpdateSocials() {
    LCEOnlineServices::GetSocialLists([](LCEOnlineServices::SocialLists list) {
        LCEOnlineServices::socialCache = list;
    });
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* pCmdLine, int nCmdShow) {
    configSystem = new Config("MCLEMPLauncher");

    versionManager = new VersionManager("MCLEMPLauncher");
    versionManager->FetchVersions(L"raw.githubusercontent.com", L"mclemp/Shared-Resources/refs/heads/main/Launcher/Versions.txt");

    installedVersions = versionManager->GetInstalledVersions();
    remoteVersions = versionManager->GetRemoteVersions();

    HttpsResponse newsResponse = HTTPSHelper::SendBasicRequest(L"raw.githubusercontent.com", L"mclemp/Shared-Resources/refs/heads/main/Launcher/News01.txt", 443, L"GET", "", L"text/plain");

    if (newsResponse.status != 200) {
        rawNewsText = "Unable To Fetch News!";
    } else {
        rawNewsText = newsResponse.body;
    }

    mainWindow = new WindowHelper("Minecraft Legacy MP Launcher", "LAUNCHER");
    mainWindow->onContentDraw = drawWindowContent;
    mainWindow->onBackgroundDraw = drawBackgroundContent;

    (LCEOnlineServices::DoesHaveAccount(configSystem) && LCEOnlineServices::isAccountValid(configSystem));

    UpdateSocials();

    //this will capture main thread and prevent code below from calling till program exit
    mainWindow->StartWindow(hInstance); 

    //do any cleanup here

    delete configSystem;
    delete versionManager;
    delete mainWindow;
}

std::atomic<bool> makingRequest = false;

std::wstring StringToWString(const std::string& str);

void drawHomeScreen();
void drawFriendsScreen();
void drawResourcesScreen();

void drawLoginScreen();

void drawBackgroundContent() { }
void drawWindowContent() {
    ImGuiIO& io = ImGui::GetIO();
    RECT title_bar_rect = WindowHelper::win32_titlebar_rect(mainWindow->getWindowHandle());

    if (showLoginScreen) {
        drawLoginScreen();
    } else {
        ImGui::SetCursorPos(ImVec2(10, 10));
        if (LCEOnlineServices::DoesHaveAccount(configSystem)) {
            std::string username = LCEOnlineServices::accountInfoCache.GetUsername();
            ImVec2 ScreenCursorPos = ImGui::GetCursorScreenPos();
            ImFont* font = ImGui::GetFont();

            ImVec2 TextSize = font->CalcTextSizeA(20, 500, 500, username.c_str());

            ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(ScreenCursorPos.x - 2, (ScreenCursorPos.y - 5) - 2), ImVec2(ScreenCursorPos.x + TextSize.x + 2, (ScreenCursorPos.y) + TextSize.y), ImGui::GetColorU32(ImVec4(0.3, 0.3, 0.3, 0.5)), 10);
            ImGui::GetForegroundDrawList()->AddText(font, 20, ImVec2(ScreenCursorPos.x, ScreenCursorPos.y - 5), ImGui::GetColorU32(ImVec4(1, 1, 1, 0.9f)), username.c_str());
            
            ImGui::SetCursorPos(ImVec2(TextSize.x + 20, 5));
            if (CustomGUI::Button("Logout", 0.4f)) {
                LCEOnlineServices::accountInfoCache = LCEOnlineServices::AccountInformation("", "");
                LCEOnlineServices::SaveAccountInformation(configSystem);
            }
        } else {
            ImGui::SetCursorPos(ImVec2(5, 5));
            if (CustomGUI::Button("Login", 0.5f)) {
                showLoginScreen = true;
            }
        }


        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - 340, 5 ));
        ImVec2 ScreenCursorPos = ImGui::GetCursorScreenPos();
        ImVec2 CursorPos = ImGui::GetCursorPos();

        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(ScreenCursorPos.x, (title_bar_rect.bottom + CursorPos.y)), ImVec2(ScreenCursorPos.x + 335, (title_bar_rect.bottom + CursorPos.y) + 30), ImGui::GetColorU32(ImVec4(0.3, 0.3, 0.3, 0.5)), 10);

        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - 340, 5));
        static int selectedTab = 1;
        if (CustomGUI::ClickableText("Home", 0.85, selectedTab == 1)) {
            selectedTab = 1;
            drawHomeScreen();
        }
        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - 265, 5));
        if (CustomGUI::ClickableText("Friends", 0.85, selectedTab == 2)) {
            selectedTab = 2;
            drawFriendsScreen();
        }
        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - 155, 5));
        if (CustomGUI::ClickableText("Resources", 0.85, selectedTab == 3)) {
            selectedTab = 3;
            drawResourcesScreen();
        }
    }
}

DownloadTask* versionDownloadTask = nullptr;
DownloadTask* versionInstallTask = nullptr; //reused download task

static std::string friendRequestResponse = "";

void UpdateFriendResponse(std::string newString) {
    friendRequestResponse = newString;
}

std::string FormatSize(size_t bytes) {
    const char* suffix[] = { "B", "KB", "MB", "GB", "TB" };
    double size = (double)bytes;
    int i = 0;

    while (size >= 1024 && i < 4) {
        size /= 1024;
        i++;
    }

    return std::format("{:.2f} {}", size, suffix[i]);
}

void drawHomeScreen() {
    ImGuiIO& io = ImGui::GetIO();

    static int currentSubwindow = -1;
    static std::string selectedVersion = "TU19";


    if (currentSubwindow != -1) {
        ImGui::SetCursorPos(ImVec2(20, 50));
        if (CustomGUI::Button("<", 0.50)) {
            currentSubwindow = -1;
        }
    }

    ImGui::SetCursorPos(ImVec2(60, 50));

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2, 0.2, 0.2, 0.5));
    ImGui::BeginChild("SubWindowChild", ImVec2(io.DisplaySize.x - 120, io.DisplaySize.y - 180));
    {
        if (currentSubwindow == -1) {
            RichText::Render(rawNewsText, 600);
        } else {
            if (currentSubwindow == 1) {
                ImGui::Text("Installed Versions");
                ImGui::Columns(6, NULL, false);
                std::unordered_set<std::string> alreadyInstalled;
                for (InstalledVersion installed : installedVersions) {
                    if (CustomGUI::Button(installed.name.c_str(), 0.50, selectedVersion == installed.name)) {
                        selectedVersion = installed.name;
                    }
                    alreadyInstalled.insert(installed.name);
                    ImGui::NextColumn();
                }
                ImGui::EndColumns();
                ImGui::Separator();
                ImGui::Text("Downloadable Versions");
                ImGui::Columns(6, NULL, false);

                for (RemoteVersion remote : remoteVersions) {
                    if (alreadyInstalled.find(remote.name) == alreadyInstalled.end()) {
                        if (CustomGUI::Button(remote.name.c_str(), 0.50, selectedVersion == remote.name)) {
                            selectedVersion = remote.name;
                        }
                    }
                    ImGui::NextColumn();
                }
                ImGui::EndColumns();
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - 55, io.DisplaySize.y - 115));
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(io.DisplaySize.x, io.DisplaySize.y), ImVec2(0, io.DisplaySize.y - 70), ImGui::GetColorU32(ImVec4(0.15, 0.15, 0.15, 0.95)), 0);

    if (versionDownloadTask != nullptr || versionInstallTask != nullptr) {
        float progress = 0.0f;

        if (versionDownloadTask != nullptr) {
            if (versionDownloadTask->total > 0) {
                progress = (float)versionDownloadTask->downloaded / (float)versionDownloadTask->total;
            }

            if (progress > 1.2f || versionDownloadTask->done) {
                delete versionDownloadTask;
                versionDownloadTask = nullptr;

                versionInstallTask = new DownloadTask();
                versionManager->FinishVersionInstall(selectedVersion, versionInstallTask);
            }
        }

        if (versionInstallTask != nullptr) {
            if (versionInstallTask->total > 0) {
                progress = (float)versionInstallTask->downloaded / (float)versionInstallTask->total;
            }

            if (progress > 1.2f || versionInstallTask->done) {
                delete versionInstallTask;
                versionInstallTask = nullptr;

                installedVersions = versionManager->GetInstalledVersions();
            }
        }
        
        float barWidth = (io.DisplaySize.x - 200) * progress;

        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(100, io.DisplaySize.y - 30), ImVec2(io.DisplaySize.x - 100, io.DisplaySize.y - 10), ImGui::GetColorU32(ImVec4(0.4, 0.4, 0.4, 0.95)));
        ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(100, io.DisplaySize.y - 30), ImVec2(100 + barWidth, io.DisplaySize.y - 10), ImGui::GetColorU32(ImVec4(0, 1, 0, 0.95)));

        std::string statusText = "";
        if (versionDownloadTask != nullptr) {
            statusText = std::format("Downloading: {}/{}", FormatSize(versionDownloadTask->downloaded.load()), FormatSize(versionDownloadTask->total.load()));
        }

        if (versionInstallTask != nullptr) {
            if (versionInstallTask->downloaded == versionInstallTask->total && !versionInstallTask->done) {
                statusText = "Removing Temp Files";
            } else {
                statusText = std::format("Installing: {}/{}", versionInstallTask->downloaded.load(), versionInstallTask->total.load());
            }
        }

        ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), 12, ImVec2(100, io.DisplaySize.y - 26), ImGui::GetColorU32(ImVec4(0.1, 0.1, 0.1, 1)), statusText.c_str());
    }
    

    if (CustomGUI::Button("Play", 0.75, (!LCEOnlineServices::DoesHaveAccount(configSystem) || versionDownloadTask != nullptr || versionInstallTask != nullptr))) {
        InstalledVersion installed;
        installed.name = ""; //make sure this is empty string
        for (InstalledVersion version : installedVersions) {
            if (version.name == selectedVersion) {
                installed = version;
                break;
            }
        }

        auto InstallSelected = [](RemoteVersion version) {
            std::filesystem::path downloadPath = versionManager->GetVersionsFolder();
            downloadPath.append(".tempDownload");

            std::filesystem::create_directories(downloadPath);
            downloadPath.append("download.zip");

            versionDownloadTask = new DownloadTask();

            std::thread([version, downloadPath, versionDownloadTaskPtr = versionDownloadTask]() {
                std::wstring convertedHost = StringToWString(version.host);
                std::wstring convertedPath = StringToWString(version.path);

                DownloadManager::DownloadFileSync(convertedHost, convertedPath, 443, downloadPath, versionDownloadTaskPtr);
            }).detach();
        };

        for (RemoteVersion version : remoteVersions) {
            if (installed.name.empty() && version.name == selectedVersion) {
                if (versionDownloadTask == nullptr) {
                    InstallSelected(version);
                    
                }
            } else if (!installed.name.empty() && version.name == installed.name) {
                if (version.version > installed.version) {
                    InstallSelected(version);
                }
            }
        }

        if (versionDownloadTask == nullptr && versionInstallTask == nullptr) {
            std::wstring convertedPath = StringToWString(installed.path);

            if (LCEOnlineServices::isOffline()) {
                STARTUPINFOW si = { sizeof(si) };
                PROCESS_INFORMATION pi = {};

                std::wstring args = L"-offline ";

                args += L"-username ";
                args += StringToWString(LCEOnlineServices::accountInfoCache.GetUsername());
                args += L" ";

                std::wstring exePath = convertedPath + L"\\Minecraft.Client.exe";
                std::wstring cmd = L"\"" + exePath + L"\" " + args;

                std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
                cmdBuffer.push_back(L'\0');

                if (!CreateProcessW(exePath.c_str(), cmdBuffer.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, convertedPath.c_str(), &si, &pi)) {
                    OutputDebugString("Failed To Open Game Instance");
                    installedVersions = versionManager->GetInstalledVersions();
                    remoteVersions = versionManager->GetRemoteVersions();
                    return;
                }

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

            } else {
                LCEOnlineServices::FetchSessionToken([convertedPath](LCEOnlineServices::APIResponse response) {
                    if (!response.wasError) {
                        STARTUPINFOW si = { sizeof(si) };
                        PROCESS_INFORMATION pi = {};

                        std::wstring args = L"-session ";

                        args += StringToWString(response.GetData());

                        std::wstring exePath = convertedPath + L"\\Minecraft.Client.exe";
                        std::wstring cmd = L"\"" + exePath + L"\" " + args;
                        
                        std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
                        cmdBuffer.push_back(L'\0');

                        if (!CreateProcessW(exePath.c_str(), cmdBuffer.data(), nullptr, nullptr, FALSE, DETACHED_PROCESS, nullptr, convertedPath.c_str(), &si, &pi)) {
                            OutputDebugString("Failed To Open Game Instance");
                            installedVersions = versionManager->GetInstalledVersions();
                            remoteVersions = versionManager->GetRemoteVersions();
                            return;
                        }

                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
                });
            }
           
        }
        
    }

    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) + 50, io.DisplaySize.y - 110));
    if (CustomGUI::Button("Switch Version", 0.50, (!LCEOnlineServices::DoesHaveAccount(configSystem) || versionDownloadTask != nullptr || versionInstallTask != nullptr))) {
        currentSubwindow = 1;
    }
}

void drawFriendsScreen() {
    if (!LCEOnlineServices::DoesHaveAccount(configSystem) || LCEOnlineServices::isOffline()) {
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font = ImGui::GetFont();
        std::string text = "Please Login";

        ImVec2 TextSize = font->CalcTextSizeA(font->FontSize, 500, 500, text.c_str());

        ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (TextSize.x / 2), (io.DisplaySize.y / 2) - (TextSize.y / 2)));
        ImGui::Text("%s", text.c_str());
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));

    static int tabIndex = 0;

    // Tabs
    ImGui::SetCursorPos(ImVec2(20, 45));
    if (CustomGUI::Button("Friends List", 0.6f, tabIndex == 0)) tabIndex = 0;

    ImGui::SetCursorPos(ImVec2(180, 45));
    if (CustomGUI::Button("Friend Requests", 0.6f, tabIndex == 1)) tabIndex = 1;

    ImGui::SetCursorPos(ImVec2(480, 45));
    if (CustomGUI::Button("Add Friend", 0.6f, tabIndex == 3)) tabIndex = 3;

    static std::chrono::steady_clock::time_point lastRefresh;
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - lastRefresh).count();

    ImGui::SetCursorPos(ImVec2(655, 45));
    if (CustomGUI::Button("Refresh", 0.6f, elapsedTime < 1500)) { //1.5 second cooldown
        lastRefresh = std::chrono::steady_clock::now();
        UpdateSocials();
    }

    // Friends / Requests panel
    ImGui::SetCursorPos(ImVec2(20, 90));
    ImGui::BeginChild("FriendsList", ImVec2(450, 360));

    if (tabIndex == 0) {
        for (size_t i = 0; i < LCEOnlineServices::socialCache.friends.size(); i++) {
            std::string& f = LCEOnlineServices::socialCache.friends[i];

            ImGui::PushID((int)i);

            ImGui::Text("%s", f.c_str());
            ImGui::SameLine(250);

            if (CustomGUI::Button("Remove")) {
                LCEOnlineServices::RemoveFriendRequest(f, [](LCEOnlineServices::APIResponse respoonse) {
                    UpdateSocials();
                });
                LCEOnlineServices::socialCache.friends.erase(LCEOnlineServices::socialCache.friends.begin() + i);
                i--;
            }

            ImGui::SameLine();

            if (CustomGUI::Button("Block")) {
                LCEOnlineServices::BlockUser(f, [](LCEOnlineServices::APIResponse respoonse) {
                    UpdateSocials();
                });
                LCEOnlineServices::socialCache.friends.erase(LCEOnlineServices::socialCache.friends.begin() + i);
                i--;
            }

            ImGui::PopID();
        }
    }
    else if (tabIndex == 1) {
        for (size_t i = 0; i < LCEOnlineServices::socialCache.requests.size(); i++) {
            std::string& r = LCEOnlineServices::socialCache.requests[i];

            ImGui::PushID((int)i);

            ImGui::Text("%s", r.c_str());
            ImGui::SameLine(250);

            if (CustomGUI::Button("Accept")) {
                LCEOnlineServices::AcceptFriendRequest(r, [](LCEOnlineServices::APIResponse respoonse) {
                    UpdateSocials();
                });
                LCEOnlineServices::socialCache.requests.erase(LCEOnlineServices::socialCache.requests.begin() + i);
                i--;
            }

            ImGui::SameLine();

            if (CustomGUI::Button("Block")) {
                LCEOnlineServices::BlockUser(r, [](LCEOnlineServices::APIResponse respoonse) {
                    UpdateSocials();
                });
                LCEOnlineServices::socialCache.requests.erase(LCEOnlineServices::socialCache.requests.begin() + i);
                i--;
            }

            ImGui::PopID();
        }
    }
    else if (tabIndex == 3) {

        if (!friendRequestResponse.empty()) {
            ImGui::SetCursorPos(ImVec2(20, 5));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
            ImGui::Text(friendRequestResponse.c_str());
            ImGui::PopStyleColor();
        }

        static char username_inputBuffer[16];
        ImGui::SetCursorPos(ImVec2(22, 30));
        ImGui::PushItemWidth(300);
        ImGui::InputText("##username", username_inputBuffer, 16);

        if (std::string(username_inputBuffer).empty()) {
            ImGui::SetCursorPos(ImVec2(22 + 5, 30 + 2));

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 0.75));
            ImGui::Text("Username");
            ImGui::PopStyleColor();
        }

        ImGui::SetCursorPos(ImVec2(20, 70));
        if (CustomGUI::Button("Send Request")) {
            UpdateFriendResponse("");

            LCEOnlineServices::SendFriendRequest(username_inputBuffer, [](LCEOnlineServices::APIResponse response) {
                UpdateFriendResponse(response.GetData());
                UpdateSocials();
            });
        }
    }

    ImGui::EndChild();

    // ================= BLOCKED =================
    ImGui::SetCursorPos(ImVec2(480, 90));
    ImGui::BeginChild("BlockedList", ImVec2(285, 360));

    for (size_t i = 0; i < LCEOnlineServices::socialCache.blocked.size(); i++) {
        std::string& b = LCEOnlineServices::socialCache.blocked[i];

        ImGui::PushID((int)(1000 + i));

        ImGui::Text("%s", b.c_str());
        ImGui::SameLine(180);

        if (CustomGUI::Button("Unblock", 0.50)) {
            LCEOnlineServices::UnblockUser(b, [](LCEOnlineServices::APIResponse respoonse) {
                UpdateSocials();
            });

            LCEOnlineServices::socialCache.blocked.erase(LCEOnlineServices::socialCache.blocked.begin() + i);
            i--;
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    ImGui::PopStyleColor();
}

void drawResourcesScreen() {
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = ImGui::GetFont();
    std::string text = "Coming Soon";

    ImVec2 TextSize = font->CalcTextSizeA(font->FontSize, 500, 500, text.c_str());

    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (TextSize.x / 2), (io.DisplaySize.y / 2) - (TextSize.y / 2)));
    ImGui::Text(text.c_str());
}

void drawLoginScreen() {
    ImGuiIO& io = ImGui::GetIO();

    if (CustomGUI::Button("Back")) {
        showLoginScreen = false;
    }

    static std::string ErrorString = "";

    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - (150), (io.DisplaySize.y / 4)));

    ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), 16, ImGui::GetCursorPos(), ImGui::GetColorU32(ImVec4(1, 0, 0, 0.9f)), ErrorString.c_str());

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3, 0.3, 0.3, 0.75));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.35, 0.35, 0.35, 0.75));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.35, 0.35, 0.35, 0.75));

    static char username_inputBuffer[16];
    static char password_inputBuffer[64];
    static bool setOldUsername = false;
    if (!setOldUsername) {
        std::string wrappedUsername = LCEOnlineServices::accountInfoCache.GetUsername();
        //theres prob a better way to do this but idk
        for (int i = 0; i < min(wrappedUsername.size(), 15); i++) {
            username_inputBuffer[i] = wrappedUsername[i];
        }

        setOldUsername = !setOldUsername;
    }

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

    if (CustomGUI::Button("Register") && !makingRequest) {
        makingRequest = true;
        ErrorString = "";

        LCEOnlineServices::AttemptAccountRegistration(username_inputBuffer, password_inputBuffer, [](LCEOnlineServices::LoginResponse response) {
            makingRequest = false;
            if (response.wasError) {
                ErrorString = response.GetData();
            } else {
                showLoginScreen = false;
            }
        });

    }

    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) - 30, (io.DisplaySize.y / 4) + 70));

    if (CustomGUI::Button("Offline") && !makingRequest) {
        ErrorString = "";
        std::string wrappedUsername(username_inputBuffer);

        if (wrappedUsername.empty()) {
            ErrorString = "Offline Username Cannot Be Empty";
        } else {
            showLoginScreen = false;
            LCEOnlineServices::offlineMode = true;
            LCEOnlineServices::accountInfoCache = LCEOnlineServices::AccountInformation("", wrappedUsername);
        }
    }

    ImGui::SetCursorPos(ImVec2((io.DisplaySize.x / 2) + 82, (io.DisplaySize.y / 4) + 70));

    if (CustomGUI::Button("Login") && !makingRequest) {
        makingRequest = true;
        ErrorString = "";

        LCEOnlineServices::AttemptAccountLogin(username_inputBuffer, password_inputBuffer, [](LCEOnlineServices::LoginResponse response) {
            makingRequest = false;
            if (response.wasError) {
                ErrorString = response.GetData();
            }
            else {
                showLoginScreen = false;
                LCEOnlineServices::SaveAccountInformation(configSystem);
            }
        });
    }
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);

    std::wstring wstr(sizeNeeded, 0);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);

    return wstr;
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