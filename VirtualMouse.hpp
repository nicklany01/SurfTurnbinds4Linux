#pragma once

#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <linux/input.h>

class VirtualMouse {
public:
    VirtualMouse() : fd_(-1) {}
    ~VirtualMouse() { destroy(); }

    bool init() {
        if (fd_ >= 0) return true;

        fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
        if (fd_ < 0) {
            std::cerr << "[VirtualMouse] Failed to open /dev/uinput: " << strerror(errno) << "\n";
            return false;
        }

        // Setup relative movements
        ioctl(fd_, UI_SET_EVBIT, EV_REL);
        ioctl(fd_, UI_SET_RELBIT, REL_X);
        ioctl(fd_, UI_SET_RELBIT, REL_Y);

        // Setup mouse buttons
        ioctl(fd_, UI_SET_EVBIT, EV_KEY);
        ioctl(fd_, UI_SET_KEYBIT, BTN_LEFT);
        ioctl(fd_, UI_SET_KEYBIT, BTN_RIGHT);
        ioctl(fd_, UI_SET_KEYBIT, BTN_MIDDLE);
        ioctl(fd_, UI_SET_KEYBIT, BTN_SIDE);
        ioctl(fd_, UI_SET_KEYBIT, BTN_EXTRA);

        struct uinput_setup usetup;
        std::memset(&usetup, 0, sizeof(usetup));
        usetup.id.bustype = BUS_USB;
        usetup.id.vendor  = 0x1234;
        usetup.id.product = 0x5678;
        std::strcpy(usetup.name, "SurfTurnbinds Virtual Mouse");

        if (ioctl(fd_, UI_DEV_SETUP, &usetup) < 0 || ioctl(fd_, UI_DEV_CREATE) < 0) {
            std::cerr << "[VirtualMouse] Failed to setup/create device: " << strerror(errno) << "\n";
            close(fd_);
            fd_ = -1;
            return false;
        }

        return true;
    }

    void moveRelative(int dx, int dy) {
        if (fd_ < 0) return;

        struct input_event ev[2];
        std::memset(ev, 0, sizeof(ev));

        ev[0].type = EV_REL;
        ev[0].code = REL_X;
        ev[0].value = dx;

        ev[1].type = EV_SYN;
        ev[1].code = SYN_REPORT;
        ev[1].value = 0;

        if (dy != 0) {
            struct input_event ev_y;
            std::memset(&ev_y, 0, sizeof(ev_y));
            ev_y.type = EV_REL;
            ev_y.code = REL_Y;
            ev_y.value = dy;
            write(fd_, &ev_y, sizeof(ev_y));
        }

        write(fd_, ev, sizeof(ev));
    }

    void destroy() {
        if (fd_ >= 0) {
            ioctl(fd_, UI_DEV_DESTROY);
            close(fd_);
            fd_ = -1;
        }
    }

    bool isReady() const { return fd_ >= 0; }

private:
    int fd_;
};
