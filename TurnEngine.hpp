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
    CAPTURE_LEFT_KEY_1,
    CAPTURE_LEFT_KEY_2,
    CAPTURE_RIGHT_KEY_1,
    CAPTURE_RIGHT_KEY_2,
    CAPTURE_CYCLE_KEY_1,
    CAPTURE_CYCLE_KEY_2,
    CAPTURE_PROFILE_KEY,
    CAPTURE_TOGGLE_ACTIVE_KEY_1,
    CAPTURE_TOGGLE_ACTIVE_KEY_2
};

class TurnEngine : public QObject {
    Q_OBJECT

public:
    explicit TurnEngine(QObject* parent = nullptr);
    ~TurnEngine() override;

    bool init();
    void shutdown();

    int getLeftKey1() const { return leftKey1_.load(); }
    int getLeftKey2() const { return leftKey2_.load(); }
    int getRightKey1() const { return rightKey1_.load(); }
    int getRightKey2() const { return rightKey2_.load(); }
    int getCycleKey1() const { return cycleKey1_.load(); }
    int getCycleKey2() const { return cycleKey2_.load(); }
    int getToggleActiveKey1() const { return toggleActiveKey1_.load(); }
    int getToggleActiveKey2() const { return toggleActiveKey2_.load(); }

    // Legacy getters for backwards compatibility
    int getLeftKey() const { return getLeftKey1(); }
    int getRightKey() const { return getRightKey1(); }
    int getCycleKey() const { return getCycleKey1(); }
    int getToggleActiveKey() const { return getToggleActiveKey1(); }

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

    void setLeftKey1(int keycode);
    void setLeftKey2(int keycode);
    void setRightKey1(int keycode);
    void setRightKey2(int keycode);
    void setCycleKey1(int keycode);
    void setCycleKey2(int keycode);
    void setToggleActiveKey1(int keycode);
    void setToggleActiveKey2(int keycode);

    // Legacy slots for backwards compatibility
    void setLeftKey(int keycode) { setLeftKey1(keycode); }
    void setRightKey(int keycode) { setRightKey1(keycode); }
    void setCycleKey(int keycode) { setCycleKey1(keycode); }
    void setToggleActiveKey(int keycode) { setToggleActiveKey1(keycode); }

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

    std::atomic<int> leftKey1_{KEY_H};       // Default Left Key 1: H
    std::atomic<int> leftKey2_{0};           // Default Left Key 2: None
    std::atomic<int> rightKey1_{KEY_L};      // Default Right Key 1: L
    std::atomic<int> rightKey2_{0};          // Default Right Key 2: None
    std::atomic<int> cycleKey1_{0};          // Default Cycle Key 1: None
    std::atomic<int> cycleKey2_{0};          // Default Cycle Key 2: None
    std::atomic<int> toggleActiveKey1_{0};   // Default Master Toggle Key 1: None
    std::atomic<int> toggleActiveKey2_{0};   // Default Master Toggle Key 2: None
    std::atomic<int> tickRate_{125};        // Default: 125 Hz
    std::atomic<double> sensitivity_{1.0};  // In-game sensitivity (default 1.0)
    std::atomic<int> dpi_{800};             // DPI (assumed 800)

    mutable std::mutex profilesMutex_;
    std::vector<YawProfile> profiles_;
    int activeProfileId_{1};
    int nextProfileId_{1};

    std::atomic<bool> left1Pressed_{false};
    std::atomic<bool> left2Pressed_{false};
    std::atomic<bool> right1Pressed_{false};
    std::atomic<bool> right2Pressed_{false};
    std::atomic<bool> leftPressed_{false};
    std::atomic<bool> rightPressed_{false};

    std::atomic<bool> toggleKey1Down_{false};
    std::atomic<bool> toggleKey2Down_{false};

    std::atomic<bool> capturing_{false};
    std::atomic<int> captureTarget_{CAPTURE_NONE};
    std::atomic<int> captureTargetId_{0};

    std::vector<int> inputFds_;
    std::thread inputThread_;
    std::thread turnThread_;
};
