#pragma once

#include <QObject>
#include <QString>
#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include "VirtualMouse.hpp"

struct YawProfile {
    int id;
    double yawspeed;       // in degrees/sec (e.g. 210.0)
    int switchKey;         // Linux keycode (0 = none)
};

enum CaptureTarget {
    CAPTURE_NONE = 0,
    CAPTURE_LEFT_KEY,
    CAPTURE_RIGHT_KEY,
    CAPTURE_CYCLE_KEY,
    CAPTURE_PROFILE_KEY,
    CAPTURE_TOGGLE_ACTIVE_KEY
};

class TurnEngine : public QObject {
    Q_OBJECT

public:
    explicit TurnEngine(QObject* parent = nullptr);
    ~TurnEngine() override;

    bool init();
    void shutdown();

    int getLeftKey() const { return leftKey_.load(); }
    int getRightKey() const { return rightKey_.load(); }
    int getCycleKey() const { return cycleKey_.load(); }
    int getToggleActiveKey() const { return toggleActiveKey_.load(); }
    int getTickRate() const { return tickRate_.load(); }
    double getSensitivity() const { return sensitivity_.load(); }
    int getDpi() const { return dpi_.load(); }
    bool isActive() const { return active_.load(); }
    bool isWindowFocused() const { return windowFocused_.load(); }

    std::vector<YawProfile> getProfiles() const;
    int getActiveProfileId() const;

public slots:
    void setActive(bool active);
    void setWindowFocused(bool focused);
    void toggleActive();
    void setTickRate(int hz);
    void setSensitivity(double sens);
    void setDpi(int dpi);

    void setLeftKey(int keycode);
    void setRightKey(int keycode);
    void setCycleKey(int keycode);
    void setToggleActiveKey(int keycode);

    int addProfile(double yawspeed, int keycode = 0);
    void removeProfile(int id);
    void setProfileSpeed(int id, double speed);
    void setProfileKey(int id, int keycode);
    void setActiveProfile(int id);
    void cycleNextProfile();
    void setProfiles(const std::vector<YawProfile>& profiles, int activeId);

    void startCapture(CaptureTarget target, int targetId = 0);
    void cancelCapture();

signals:
    void engineStateChanged(bool active, const QString& statusMessage);
    void keyCaptured(int target, int targetId, int keycode, const QString& keyName);
    void activeProfileChanged(int profileId);
    void turningStateChanged(int direction); // -1 = left, 0 = idle, 1 = right

private:
    void inputListenerLoop();
    void turnWorkerLoop();
    void openInputDevices();
    void closeInputDevices();

    VirtualMouse mouse_;

    std::atomic<bool> running_{false};
    std::atomic<bool> active_{true};
    std::atomic<bool> windowFocused_{false};

    std::atomic<int> leftKey_{KEY_H};       // Default Left: H
    std::atomic<int> rightKey_{KEY_L};      // Default Right: L
    std::atomic<int> cycleKey_{0};          // Default: None
    std::atomic<int> toggleActiveKey_{0};   // Default: None (Master toggle)
    std::atomic<int> tickRate_{125};        // Default: 125 Hz
    std::atomic<double> sensitivity_{1.0};  // In-game sensitivity (default 1.0)
    std::atomic<int> dpi_{800};             // DPI (assumed 800)

    mutable std::mutex profilesMutex_;
    std::vector<YawProfile> profiles_;
    int activeProfileId_{1};
    int nextProfileId_{1};

    std::atomic<bool> leftPressed_{false};
    std::atomic<bool> rightPressed_{false};

    std::atomic<bool> capturing_{false};
    std::atomic<int> captureTarget_{CAPTURE_NONE};
    std::atomic<int> captureTargetId_{0};

    std::vector<int> inputFds_;
    std::thread inputThread_;
    std::thread turnThread_;
};
