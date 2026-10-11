#include "TurnEngine.hpp"
#include "KeyNames.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <iostream>
#include <chrono>

TurnEngine::TurnEngine(QObject* parent)
    : QObject(parent) {
    // Initialize default profiles
    profiles_.push_back({nextProfileId_++, 210.0, 0});
    profiles_.push_back({nextProfileId_++, 350.0, 0});
    activeProfileId_ = profiles_.front().id;
}

TurnEngine::~TurnEngine() {
    shutdown();
}

bool TurnEngine::init() {
    if (!mouse_.init()) {
        emit engineStateChanged(false, "Cannot open /dev/uinput (check permissions/udev)");
        return false;
    }

    openInputDevices();
    if (inputFds_.empty()) {
        emit engineStateChanged(false, "No input devices found in /dev/input (check 'input' group)");
        return false;
    }

    running_ = true;
    inputThread_ = std::thread(&TurnEngine::inputListenerLoop, this);
    turnThread_ = std::thread(&TurnEngine::turnWorkerLoop, this);

    emit engineStateChanged(true, "Ready and listening");
    return true;
}

void TurnEngine::shutdown() {
    running_ = false;
    if (inputThread_.joinable()) {
        inputThread_.join();
    }
    if (turnThread_.joinable()) {
        turnThread_.join();
    }
    closeInputDevices();
    mouse_.destroy();
}

void TurnEngine::setActive(bool active) {
    active_ = active;
    if (!active) {
        leftPressed_ = false;
        rightPressed_ = false;
        emit turningStateChanged(0);
    }
    emit engineStateChanged(active, active ? "Engine Active" : "Engine Paused");
}

void TurnEngine::setWindowFocused(bool focused) {
    windowFocused_ = focused;
    if (focused) {
        leftPressed_ = false;
        rightPressed_ = false;
        emit turningStateChanged(0);
    }
}

void TurnEngine::toggleActive() {
    setActive(!active_.load());
}

void TurnEngine::setToggleActiveKey(int keycode) {
    toggleActiveKey_ = keycode;
}

void TurnEngine::setTickRate(int hz) {
    tickRate_ = std::max(10, hz);
}

void TurnEngine::setSensitivity(double sens) {
    if (sens > 0.01) {
        sensitivity_ = sens;
    }
}

void TurnEngine::setDpi(int dpi) {
    if (dpi > 50) {
        dpi_ = dpi;
    }
}

void TurnEngine::setLeftKey(int keycode) {
    leftKey_ = keycode;
}

void TurnEngine::setRightKey(int keycode) {
    rightKey_ = keycode;
}

void TurnEngine::setCycleKey(int keycode) {
    cycleKey_ = keycode;
}

std::vector<YawProfile> TurnEngine::getProfiles() const {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    return profiles_;
}

int TurnEngine::getActiveProfileId() const {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    return activeProfileId_;
}

int TurnEngine::addProfile(double yawspeed, int keycode) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    if (profiles_.size() >= 5) return -1;

    int newId = nextProfileId_++;
    profiles_.push_back({newId, std::max(10.0, yawspeed), keycode});
    return newId;
}

void TurnEngine::removeProfile(int id) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    if (profiles_.size() <= 1) return; // Keep at least one

    for (auto it = profiles_.begin(); it != profiles_.end(); ++it) {
        if (it->id == id) {
            bool wasActive = (it->id == activeProfileId_);
            profiles_.erase(it);
            if (wasActive && !profiles_.empty()) {
                activeProfileId_ = profiles_.front().id;
                emit activeProfileChanged(activeProfileId_);
            }
            break;
        }
    }
}

void TurnEngine::setProfileSpeed(int id, double speed) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    for (auto& p : profiles_) {
        if (p.id == id) {
            p.yawspeed = std::max(10.0, speed);
            break;
        }
    }
}

void TurnEngine::setProfileKey(int id, int keycode) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    for (auto& p : profiles_) {
        if (p.id == id) {
            p.switchKey = keycode;
            break;
        }
    }
}

void TurnEngine::setActiveProfile(int id) {
    {
        std::lock_guard<std::mutex> lock(profilesMutex_);
        bool found = false;
        for (const auto& p : profiles_) {
            if (p.id == id) {
                activeProfileId_ = id;
                found = true;
                break;
            }
        }
        if (!found) return;
    }
    emit activeProfileChanged(id);
}

void TurnEngine::cycleNextProfile() {
    int nextId = -1;
    {
        std::lock_guard<std::mutex> lock(profilesMutex_);
        if (profiles_.empty()) return;

        size_t currentIdx = 0;
        for (size_t i = 0; i < profiles_.size(); ++i) {
            if (profiles_[i].id == activeProfileId_) {
                currentIdx = i;
                break;
            }
        }
        size_t nextIdx = (currentIdx + 1) % profiles_.size();
        activeProfileId_ = profiles_[nextIdx].id;
        nextId = activeProfileId_;
    }

    if (nextId != -1) {
        emit activeProfileChanged(nextId);
    }
}

void TurnEngine::setProfiles(const std::vector<YawProfile>& profiles, int activeId) {
    std::lock_guard<std::mutex> lock(profilesMutex_);
    profiles_ = profiles;
    activeProfileId_ = activeId;
    int maxId = 0;
    for (const auto& p : profiles_) {
        if (p.id > maxId) maxId = p.id;
    }
    nextProfileId_ = maxId + 1;
}

void TurnEngine::startCapture(CaptureTarget target, int targetId) {
    captureTarget_ = target;
    captureTargetId_ = targetId;
    capturing_ = true;
}

void TurnEngine::cancelCapture() {
    capturing_ = false;
    captureTarget_ = CAPTURE_NONE;
}

void TurnEngine::openInputDevices() {
    closeInputDevices();

    DIR* dir = opendir("/dev/input");
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strncmp(entry->d_name, "event", 5) != 0) continue;

        std::string path = std::string("/dev/input/") + entry->d_name;
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        unsigned long ev_bits[(EV_MAX + 7) / 8] = {0};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) >= 0) {
            bool has_keys = ev_bits[EV_KEY / (sizeof(long) * 8)] & (1UL << (EV_KEY % (sizeof(long) * 8)));
            if (has_keys) {
                inputFds_.push_back(fd);
                continue;
            }
        }
        close(fd);
    }
    closedir(dir);
}

void TurnEngine::closeInputDevices() {
    for (int fd : inputFds_) {
        close(fd);
    }
    inputFds_.clear();
}

void TurnEngine::inputListenerLoop() {
    if (inputFds_.empty()) return;

    std::vector<struct pollfd> pfd(inputFds_.size());
    for (size_t i = 0; i < inputFds_.size(); ++i) {
        pfd[i].fd = inputFds_[i];
        pfd[i].events = POLLIN;
    }

    while (running_) {
        int ret = poll(pfd.data(), pfd.size(), 50);
        if (ret <= 0) continue;

        for (size_t i = 0; i < pfd.size(); ++i) {
            if (pfd[i].revents & POLLIN) {
                struct input_event ev;
                while (read(pfd[i].fd, &ev, sizeof(ev)) == sizeof(ev)) {
                    if (ev.type != EV_KEY) continue;

                    // 1. Key Recording Mode
                    if (capturing_ && ev.value == 1) {
                        int code = ev.code;
                        // Pressing Escape or Backspace unsets the keybind
                        if (code == KEY_ESC || code == KEY_BACKSPACE) {
                            code = 0;
                        }

                        int target = captureTarget_.load();
                        int targetId = captureTargetId_.load();
                        capturing_ = false;
                        emit keyCaptured(target, targetId, code, QString::fromStdString(KeyNames::getName(code)));
                        continue;
                    }

                    // 2. Master Toggle Key (works whether active is true or false)
                    if (toggleActiveKey_.load() != 0 && ev.value == 1 && ev.code == toggleActiveKey_.load()) {
                        toggleActive();
                        continue;
                    }

                    // 3. Normal Gaming Hotkeys (disabled while window is active)
                    if (active_ && !windowFocused_.load()) {
                        // Turning Keys (only active if assigned != 0)
                        if (leftKey_.load() != 0 && ev.code == leftKey_.load()) {
                            leftPressed_ = (ev.value != 0);
                            int dir = leftPressed_ ? -1 : (rightPressed_ ? 1 : 0);
                            emit turningStateChanged(dir);
                        } else if (rightKey_.load() != 0 && ev.code == rightKey_.load()) {
                            rightPressed_ = (ev.value != 0);
                            int dir = rightPressed_ ? 1 : (leftPressed_ ? -1 : 0);
                            emit turningStateChanged(dir);
                        }

                        // Cycle Key
                        if (ev.value == 1 && cycleKey_.load() != 0 && ev.code == cycleKey_.load()) {
                            cycleNextProfile();
                        }

                        // Direct Profile Switch Keys
                        if (ev.value == 1) {
                            int matchedId = -1;
                            {
                                std::lock_guard<std::mutex> lock(profilesMutex_);
                                for (const auto& p : profiles_) {
                                    if (p.switchKey != 0 && p.switchKey == ev.code) {
                                        matchedId = p.id;
                                        break;
                                    }
                                }
                            }
                            if (matchedId != -1) {
                                setActiveProfile(matchedId);
                            }
                        }
                    }
                }
            }
        }
    }
}

void TurnEngine::turnWorkerLoop() {
    double remainder = 0.0;

    while (running_) {
        int hz = tickRate_.load();
        int interval_ms = std::max(1, 1000 / hz);
        double dt = 1.0 / hz;

        if (active_ && !windowFocused_.load() && !capturing_) {
            bool l = leftPressed_.load();
            bool r = rightPressed_.load();

            if (l ^ r) {
                double speed_deg_per_sec = 210.0;
                {
                    std::lock_guard<std::mutex> lock(profilesMutex_);
                    for (const auto& p : profiles_) {
                        if (p.id == activeProfileId_) {
                            speed_deg_per_sec = p.yawspeed;
                            break;
                        }
                    }
                }

                // Mathematical conversion from degrees/second to raw relative mouse counts:
                // Degrees per count in Source engine = m_yaw * sensitivity (standard m_yaw = 0.022)
                // At 800 DPI, if sensitivity is matched, angular speed is exact.
                double sens = std::max(0.01, sensitivity_.load());
                double counts_per_sec = speed_deg_per_sec / (0.022 * sens);
                double counts_this_tick = counts_per_sec * dt + remainder;

                int step = static_cast<int>(counts_this_tick);
                remainder = counts_this_tick - step;

                if (step > 0) {
                    if (l) {
                        mouse_.moveRelative(-step, 0);
                    } else {
                        mouse_.moveRelative(+step, 0);
                    }
                }
            } else {
                remainder = 0.0;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

#include "moc_TurnEngine.cpp"
