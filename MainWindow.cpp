#include "MainWindow.hpp"
#include "KeyNames.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <QCloseEvent>
#include <QMessageBox>
#include <QEnterEvent>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      engine_(new TurnEngine(this)) {

    setupUi();

    connect(engine_, &TurnEngine::keyCaptured, this, &MainWindow::onKeyCaptured);
    connect(engine_, &TurnEngine::activeProfileChanged, this, &MainWindow::onActiveProfileChanged);
    connect(engine_, &TurnEngine::engineStateChanged, this, &MainWindow::onEngineStateChanged);

    loadConfig();
    rebuildProfileList();

    if (!engine_->init()) {
        lblStatus_->setText("Initialization failed. Check permissions.");
    }
}

QString MainWindow::getConfigFilePath() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/surfturnbinds";
    QDir().mkpath(path);
    return path + "/config.ini";
}

void MainWindow::loadConfig() {
    QString path = getConfigFilePath();
    QSettings settings(path, QSettings::IniFormat);

    int leftKey = settings.value("General/left_key", KEY_H).toInt();
    int rightKey = settings.value("General/right_key", KEY_L).toInt();
    int cycleKey = settings.value("General/cycle_key", 0).toInt();
    int toggleKey = settings.value("General/toggle_key", 0).toInt();
    double sens = settings.value("General/sensitivity", 1.0).toDouble();
    int activeId = settings.value("General/active_profile", 1).toInt();

    engine_->setLeftKey(leftKey);
    engine_->setRightKey(rightKey);
    engine_->setCycleKey(cycleKey);
    engine_->setToggleActiveKey(toggleKey);
    engine_->setSensitivity(sens);

    spinSensitivity_->blockSignals(true);
    spinSensitivity_->setValue(sens);
    spinSensitivity_->blockSignals(false);

    btnLeftKey_->setText(QString::fromStdString(KeyNames::getName(leftKey)));
    btnRightKey_->setText(QString::fromStdString(KeyNames::getName(rightKey)));
    btnCycleKey_->setText(QString::fromStdString(KeyNames::getName(cycleKey)));
    btnMasterToggleKey_->setText(QString::fromStdString(KeyNames::getName(toggleKey)));

    int pCount = settings.value("Profiles/count", 0).toInt();
    if (pCount > 0) {
        std::vector<YawProfile> list;
        for (int i = 0; i < pCount && i < 5; ++i) {
            QString prefix = QString("Profiles/profile_%1_").arg(i);
            int id = settings.value(prefix + "id", i + 1).toInt();
            double speed = settings.value(prefix + "speed", 210.0).toDouble();
            int key = settings.value(prefix + "key", 0).toInt();
            list.push_back({id, speed, key});
        }
        if (!list.empty()) {
            engine_->setProfiles(list, activeId);
        }
    }

    lblStatus_->setText("Config loaded: " + path);
}

void MainWindow::saveConfig() {
    QString path = getConfigFilePath();
    QSettings settings(path, QSettings::IniFormat);

    settings.setValue("General/left_key", engine_->getLeftKey());
    settings.setValue("General/right_key", engine_->getRightKey());
    settings.setValue("General/cycle_key", engine_->getCycleKey());
    settings.setValue("General/toggle_key", engine_->getToggleActiveKey());
    settings.setValue("General/sensitivity", engine_->getSensitivity());
    settings.setValue("General/active_profile", engine_->getActiveProfileId());

    auto profiles = engine_->getProfiles();
    settings.setValue("Profiles/count", static_cast<int>(profiles.size()));
    for (size_t i = 0; i < profiles.size(); ++i) {
        QString prefix = QString("Profiles/profile_%1_").arg(i);
        settings.setValue(prefix + "id", profiles[i].id);
        settings.setValue(prefix + "speed", profiles[i].yawspeed);
        settings.setValue(prefix + "key", profiles[i].switchKey);
    }
    settings.sync();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveConfig();
    event->accept();
}

void MainWindow::updateWindowFocusState() {
    if (engine_) {
        bool focused = isActiveWindow() || underMouse();
        engine_->setWindowFocused(focused);
    }
}

void MainWindow::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::ActivationChange) {
        updateWindowFocusState();
    }
}

void MainWindow::enterEvent(QEnterEvent* event) {
    QWidget::enterEvent(event);
    updateWindowFocusState();
}

void MainWindow::leaveEvent(QEvent* event) {
    QWidget::leaveEvent(event);
    updateWindowFocusState();
}

void MainWindow::setupUi() {
    setWindowTitle("Surf Turnbinds (Linux)");
    resize(500, 540);
    setMinimumWidth(460);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(14);
    rootLayout->setContentsMargins(18, 18, 18, 18);

    // 1. Header: Status & Master Toggle
    auto* topLayout = new QHBoxLayout();
    auto* lblTitle = new QLabel("SURF TURNBINDS", this);
    lblTitle->setStyleSheet("font-weight: bold; font-size: 15px; color: #38bdf8;");

    btnToggleActive_ = new QPushButton("Active", this);
    btnToggleActive_->setCheckable(true);
    btnToggleActive_->setChecked(true);
    btnToggleActive_->setStyleSheet("font-weight: bold; padding: 6px 14px; background-color: #059669; color: white; border-radius: 4px;");
    connect(btnToggleActive_, &QPushButton::clicked, this, &MainWindow::onToggleActive);

    topLayout->addWidget(lblTitle);
    topLayout->addStretch();
    topLayout->addWidget(btnToggleActive_);
    rootLayout->addLayout(topLayout);

    lblStatus_ = new QLabel("Ready - listening for input", this);
    lblStatus_->setStyleSheet("color: #94a3b8; font-size: 12px;");
    rootLayout->addWidget(lblStatus_);

    // 2. Global Keybinds & Sensitivity Group
    auto* grpGlobal = new QGroupBox("Controls & Keys (Click to set, 'Clear' or Esc to unset)", this);
    auto* grid = new QGridLayout(grpGlobal);
    grid->setSpacing(8);

    // Left Turn
    grid->addWidget(new QLabel("Turn Left Key:", this), 0, 0);
    btnLeftKey_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getLeftKey())), this);
    connect(btnLeftKey_, &QPushButton::clicked, this, &MainWindow::onSelectLeftKeyClicked);
    grid->addWidget(btnLeftKey_, 0, 1);

    btnClearLeftKey_ = new QPushButton("Clear", this);
    btnClearLeftKey_->setFixedWidth(56);
    connect(btnClearLeftKey_, &QPushButton::clicked, this, &MainWindow::onClearLeftKeyClicked);
    grid->addWidget(btnClearLeftKey_, 0, 2);

    // Right Turn
    grid->addWidget(new QLabel("Turn Right Key:", this), 1, 0);
    btnRightKey_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getRightKey())), this);
    connect(btnRightKey_, &QPushButton::clicked, this, &MainWindow::onSelectRightKeyClicked);
    grid->addWidget(btnRightKey_, 1, 1);

    btnClearRightKey_ = new QPushButton("Clear", this);
    btnClearRightKey_->setFixedWidth(56);
    connect(btnClearRightKey_, &QPushButton::clicked, this, &MainWindow::onClearRightKeyClicked);
    grid->addWidget(btnClearRightKey_, 1, 2);

    // Cycle Next Speed Key
    grid->addWidget(new QLabel("Cycle Speeds Key:", this), 2, 0);
    btnCycleKey_ = new QPushButton("None", this);
    connect(btnCycleKey_, &QPushButton::clicked, this, &MainWindow::onSelectCycleKeyClicked);
    grid->addWidget(btnCycleKey_, 2, 1);

    btnClearCycleKey_ = new QPushButton("Clear", this);
    btnClearCycleKey_->setFixedWidth(56);
    connect(btnClearCycleKey_, &QPushButton::clicked, this, &MainWindow::onClearCycleKeyClicked);
    grid->addWidget(btnClearCycleKey_, 2, 2);

    // Master Toggle Key (Full turnbinds ON/OFF)
    grid->addWidget(new QLabel("Master Toggle (On/Off):", this), 3, 0);
    btnMasterToggleKey_ = new QPushButton("None", this);
    connect(btnMasterToggleKey_, &QPushButton::clicked, this, &MainWindow::onSelectMasterToggleKeyClicked);
    grid->addWidget(btnMasterToggleKey_, 3, 1);

    btnClearMasterToggleKey_ = new QPushButton("Clear", this);
    btnClearMasterToggleKey_->setFixedWidth(56);
    connect(btnClearMasterToggleKey_, &QPushButton::clicked, this, &MainWindow::onClearMasterToggleKeyClicked);
    grid->addWidget(btnClearMasterToggleKey_, 3, 2);

    // In-game sensitivity
    grid->addWidget(new QLabel("In-Game Sens (800 DPI):", this), 4, 0);
    spinSensitivity_ = new QDoubleSpinBox(this);
    spinSensitivity_->setRange(0.1, 10.0);
    spinSensitivity_->setSingleStep(0.05);
    spinSensitivity_->setValue(1.0);
    connect(spinSensitivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSensitivityChanged);
    grid->addWidget(spinSensitivity_, 4, 1, 1, 2);

    rootLayout->addWidget(grpGlobal);

    // 3. Yaw Speeds Profile Column
    auto* grpProfiles = new QGroupBox("Yaw Speeds (°/s)", this);
    auto* profilesOuterLayout = new QVBoxLayout(grpProfiles);
    profilesOuterLayout->setSpacing(8);

    auto* profileHeader = new QHBoxLayout();
    profileHeader->addWidget(new QLabel("Configured Speeds (Max 5):", this));
    profileHeader->addStretch();
    btnAddProfile_ = new QPushButton("+ Add Speed", this);
    btnAddProfile_->setStyleSheet("font-weight: bold; background-color: #0284c7; color: white; padding: 4px 10px; border-radius: 4px;");
    connect(btnAddProfile_, &QPushButton::clicked, this, &MainWindow::onAddProfileClicked);
    profileHeader->addWidget(btnAddProfile_);
    profilesOuterLayout->addLayout(profileHeader);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollWidget = new QWidget(scrollArea);
    profilesListLayout_ = new QVBoxLayout(scrollWidget);
    profilesListLayout_->setSpacing(8);
    profilesListLayout_->setContentsMargins(0, 0, 0, 0);
    scrollArea->setWidget(scrollWidget);

    profilesOuterLayout->addWidget(scrollArea);
    rootLayout->addWidget(grpProfiles, 1);
}

void MainWindow::rebuildProfileList() {
    while (QLayoutItem* item = profilesListLayout_->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    profileWidgets_.clear();

    auto profiles = engine_->getProfiles();
    int activeId = engine_->getActiveProfileId();

    btnAddProfile_->setEnabled(profiles.size() < 5);

    for (const auto& p : profiles) {
        auto* row = new QWidget(this);
        row->setObjectName(QString("profileRow_%1").arg(p.id));
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(10, 8, 10, 8);
        rowLayout->setSpacing(8);

        // Active Radio button
        auto* radio = new QRadioButton(row);
        radio->setText("Active");
        radio->setChecked(p.id == activeId);
        connect(radio, &QRadioButton::clicked, [this, id = p.id]() {
            onProfileRadioClicked(id);
        });
        rowLayout->addWidget(radio);

        // SpinBox for °/s
        auto* spin = new QDoubleSpinBox(row);
        spin->setRange(50.0, 1500.0);
        spin->setSingleStep(10.0);
        spin->setSuffix(" °/s");
        spin->setValue(p.yawspeed);
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this, id = p.id](double val) {
            onProfileSpinChanged(id, val);
        });
        rowLayout->addWidget(spin);

        // Slider
        auto* slider = new QSlider(Qt::Horizontal, row);
        slider->setRange(50, 1500);
        slider->setValue(static_cast<int>(p.yawspeed));
        connect(slider, &QSlider::valueChanged, [this, id = p.id](int val) {
            onProfileSliderChanged(id, val);
        });
        rowLayout->addWidget(slider, 1);

        // Keybind Button
        QString keyText = (p.switchKey != 0) ? QString::fromStdString(KeyNames::getName(p.switchKey)) : "None";
        auto* btnKey = new QPushButton("Key: [" + keyText + "]", row);
        btnKey->setToolTip("Click to set hotkey, press Esc to clear");
        connect(btnKey, &QPushButton::clicked, [this, id = p.id]() {
            onProfileKeyClicked(id);
        });
        rowLayout->addWidget(btnKey);

        // Clear Key Button
        auto* btnClearKey = new QPushButton("Clear", row);
        btnClearKey->setFixedWidth(50);
        connect(btnClearKey, &QPushButton::clicked, [this, id = p.id]() {
            onProfileKeyClearClicked(id);
        });
        rowLayout->addWidget(btnClearKey);

        // Delete Row Button
        auto* btnRemove = new QPushButton("✖", row);
        btnRemove->setFixedWidth(28);
        btnRemove->setEnabled(profiles.size() > 1);
        btnRemove->setToolTip("Remove this speed slot");
        connect(btnRemove, &QPushButton::clicked, [this, id = p.id]() {
            onRemoveProfileClicked(id);
        });
        rowLayout->addWidget(btnRemove);

        profilesListLayout_->addWidget(row);
        profileWidgets_[p.id] = {p.id, row, radio, slider, spin, btnKey, btnClearKey, btnRemove};
    }

    profilesListLayout_->addStretch();
    updateActiveRowHighlight(activeId);
}

void MainWindow::updateActiveRowHighlight(int activeId) {
    for (const auto& [id, w] : profileWidgets_) {
        bool isActive = (id == activeId);
        w.radioActive->setChecked(isActive);

        if (isActive) {
            w.container->setStyleSheet(
                "QWidget#" + w.container->objectName() + " {"
                "  background-color: #1e293b;"
                "  border: 2px solid #38bdf8;"
                "  border-radius: 6px;"
                "}"
            );
        } else {
            w.container->setStyleSheet(
                "QWidget#" + w.container->objectName() + " {"
                "  background-color: #0f172a;"
                "  border: 1px solid #334155;"
                "  border-radius: 6px;"
                "}"
            );
        }
    }
}

void MainWindow::onToggleActive() {
    bool active = btnToggleActive_->isChecked();
    btnToggleActive_->setText(active ? "Active" : "Paused");
    btnToggleActive_->setStyleSheet(active ? "font-weight: bold; padding: 6px 14px; background-color: #059669; color: white; border-radius: 4px;"
                                           : "font-weight: bold; padding: 6px 14px; background-color: #475569; color: #cbd5e1; border-radius: 4px;");
    engine_->setActive(active);
}

void MainWindow::onSelectLeftKeyClicked() {
    currentCapturingTarget_ = CAPTURE_LEFT_KEY;
    currentCapturingId_ = 0;
    btnLeftKey_->setText("Press any key (Esc to unset)...");
    engine_->startCapture(CAPTURE_LEFT_KEY);
}

void MainWindow::onClearLeftKeyClicked() {
    engine_->cancelCapture();
    engine_->setLeftKey(0);
    btnLeftKey_->setText("None");
    saveConfig();
}

void MainWindow::onSelectRightKeyClicked() {
    currentCapturingTarget_ = CAPTURE_RIGHT_KEY;
    currentCapturingId_ = 0;
    btnRightKey_->setText("Press any key (Esc to unset)...");
    engine_->startCapture(CAPTURE_RIGHT_KEY);
}

void MainWindow::onClearRightKeyClicked() {
    engine_->cancelCapture();
    engine_->setRightKey(0);
    btnRightKey_->setText("None");
    saveConfig();
}

void MainWindow::onSelectCycleKeyClicked() {
    currentCapturingTarget_ = CAPTURE_CYCLE_KEY;
    currentCapturingId_ = 0;
    btnCycleKey_->setText("Press any key (Esc to unset)...");
    engine_->startCapture(CAPTURE_CYCLE_KEY);
}

void MainWindow::onClearCycleKeyClicked() {
    engine_->cancelCapture();
    engine_->setCycleKey(0);
    btnCycleKey_->setText("None");
    saveConfig();
}

void MainWindow::onSelectMasterToggleKeyClicked() {
    currentCapturingTarget_ = CAPTURE_TOGGLE_ACTIVE_KEY;
    currentCapturingId_ = 0;
    btnMasterToggleKey_->setText("Press any key (Esc to unset)...");
    engine_->startCapture(CAPTURE_TOGGLE_ACTIVE_KEY);
}

void MainWindow::onClearMasterToggleKeyClicked() {
    engine_->cancelCapture();
    engine_->setToggleActiveKey(0);
    btnMasterToggleKey_->setText("None");
    saveConfig();
}

void MainWindow::onAddProfileClicked() {
    double newSpeed = 210.0;
    auto profiles = engine_->getProfiles();
    if (!profiles.empty()) {
        newSpeed = profiles.back().yawspeed + 100.0;
    }
    engine_->addProfile(newSpeed, 0);
    rebuildProfileList();
    saveConfig();
}

void MainWindow::onRemoveProfileClicked(int id) {
    engine_->removeProfile(id);
    rebuildProfileList();
    saveConfig();
}

void MainWindow::onProfileRadioClicked(int id) {
    engine_->setActiveProfile(id);
    saveConfig();
}

void MainWindow::onProfileSliderChanged(int id, int value) {
    auto it = profileWidgets_.find(id);
    if (it != profileWidgets_.end()) {
        it->second.spinSpeed->blockSignals(true);
        it->second.spinSpeed->setValue(value);
        it->second.spinSpeed->blockSignals(false);
    }
    engine_->setProfileSpeed(id, value);
    saveConfig();
}

void MainWindow::onProfileSpinChanged(int id, double value) {
    auto it = profileWidgets_.find(id);
    if (it != profileWidgets_.end()) {
        it->second.slider->blockSignals(true);
        it->second.slider->setValue(static_cast<int>(value));
        it->second.slider->blockSignals(false);
    }
    engine_->setProfileSpeed(id, value);
    saveConfig();
}

void MainWindow::onProfileKeyClicked(int id) {
    currentCapturingTarget_ = CAPTURE_PROFILE_KEY;
    currentCapturingId_ = id;
    auto it = profileWidgets_.find(id);
    if (it != profileWidgets_.end()) {
        it->second.btnKey->setText("Press key (Esc to unset)...");
    }
    engine_->startCapture(CAPTURE_PROFILE_KEY, id);
}

void MainWindow::onProfileKeyClearClicked(int id) {
    engine_->cancelCapture();
    engine_->setProfileKey(id, 0);
    auto it = profileWidgets_.find(id);
    if (it != profileWidgets_.end()) {
        it->second.btnKey->setText("Key: [None]");
    }
    saveConfig();
}

void MainWindow::onSensitivityChanged(double val) {
    engine_->setSensitivity(val);
    saveConfig();
}

void MainWindow::onKeyCaptured(int target, int targetId, int keycode, const QString& keyName) {
    currentCapturingTarget_ = CAPTURE_NONE;
    currentCapturingId_ = 0;

    // 1. If key is 0 (Esc / Backspace pressed to unset)
    if (keycode == 0) {
        if (target == CAPTURE_LEFT_KEY) {
            engine_->setLeftKey(0);
            btnLeftKey_->setText("None");
        } else if (target == CAPTURE_RIGHT_KEY) {
            engine_->setRightKey(0);
            btnRightKey_->setText("None");
        } else if (target == CAPTURE_CYCLE_KEY) {
            engine_->setCycleKey(0);
            btnCycleKey_->setText("None");
        } else if (target == CAPTURE_TOGGLE_ACTIVE_KEY) {
            engine_->setToggleActiveKey(0);
            btnMasterToggleKey_->setText("None");
        } else if (target == CAPTURE_PROFILE_KEY) {
            engine_->setProfileKey(targetId, 0);
            auto it = profileWidgets_.find(targetId);
            if (it != profileWidgets_.end()) {
                it->second.btnKey->setText("Key: [None]");
            }
        }
        saveConfig();
        return;
    }

    // 2. Conflict Detection across all existing binds
    QString conflictAction = "";
    int conflictType = 0;
    int conflictProfileId = 0;

    if (target != CAPTURE_LEFT_KEY && engine_->getLeftKey() == keycode) {
        conflictAction = "Turn Left";
        conflictType = CAPTURE_LEFT_KEY;
    } else if (target != CAPTURE_RIGHT_KEY && engine_->getRightKey() == keycode) {
        conflictAction = "Turn Right";
        conflictType = CAPTURE_RIGHT_KEY;
    } else if (target != CAPTURE_CYCLE_KEY && engine_->getCycleKey() == keycode) {
        conflictAction = "Cycle Speeds";
        conflictType = CAPTURE_CYCLE_KEY;
    } else if (target != CAPTURE_TOGGLE_ACTIVE_KEY && engine_->getToggleActiveKey() == keycode) {
        conflictAction = "Master Toggle (On/Off)";
        conflictType = CAPTURE_TOGGLE_ACTIVE_KEY;
    } else {
        auto profiles = engine_->getProfiles();
        for (const auto& p : profiles) {
            if ((target != CAPTURE_PROFILE_KEY || targetId != p.id) && p.switchKey == keycode) {
                conflictAction = QString("Speed Slot (%1 °/s)").arg(p.yawspeed);
                conflictType = CAPTURE_PROFILE_KEY;
                conflictProfileId = p.id;
                break;
            }
        }
    }

    // 3. Clear the conflicting previous bind
    if (conflictType != 0) {
        if (conflictType == CAPTURE_LEFT_KEY) {
            engine_->setLeftKey(0);
            btnLeftKey_->setText("None");
        } else if (conflictType == CAPTURE_RIGHT_KEY) {
            engine_->setRightKey(0);
            btnRightKey_->setText("None");
        } else if (conflictType == CAPTURE_CYCLE_KEY) {
            engine_->setCycleKey(0);
            btnCycleKey_->setText("None");
        } else if (conflictType == CAPTURE_TOGGLE_ACTIVE_KEY) {
            engine_->setToggleActiveKey(0);
            btnMasterToggleKey_->setText("None");
        } else if (conflictType == CAPTURE_PROFILE_KEY) {
            engine_->setProfileKey(conflictProfileId, 0);
            auto it = profileWidgets_.find(conflictProfileId);
            if (it != profileWidgets_.end()) {
                it->second.btnKey->setText("Key: [None]");
            }
        }
    }

    // 4. Assign the new bind to target
    if (target == CAPTURE_LEFT_KEY) {
        engine_->setLeftKey(keycode);
        btnLeftKey_->setText(keyName);
    } else if (target == CAPTURE_RIGHT_KEY) {
        engine_->setRightKey(keycode);
        btnRightKey_->setText(keyName);
    } else if (target == CAPTURE_CYCLE_KEY) {
        engine_->setCycleKey(keycode);
        btnCycleKey_->setText(keyName);
    } else if (target == CAPTURE_TOGGLE_ACTIVE_KEY) {
        engine_->setToggleActiveKey(keycode);
        btnMasterToggleKey_->setText(keyName);
    } else if (target == CAPTURE_PROFILE_KEY) {
        engine_->setProfileKey(targetId, keycode);
        auto it = profileWidgets_.find(targetId);
        if (it != profileWidgets_.end()) {
            it->second.btnKey->setText("Key: [" + keyName + "]");
        }
    }

    saveConfig();

    // 5. Pop up warning if there was a duplicate bind
    if (!conflictAction.isEmpty()) {
        QMessageBox::warning(
            this,
            "Keybind Conflict",
            QString("'%1' was already bound to \"%2\".\n\n"
                    "The previous bind has been cleared and assigned to this action.")
                .arg(keyName)
                .arg(conflictAction)
        );
    }
}

void MainWindow::onActiveProfileChanged(int profileId) {
    updateActiveRowHighlight(profileId);
    saveConfig();
}

void MainWindow::onEngineStateChanged(bool active, const QString& message) {
    lblStatus_->setText(message);
    btnToggleActive_->blockSignals(true);
    btnToggleActive_->setChecked(active);
    btnToggleActive_->setText(active ? "Active" : "Paused");
    btnToggleActive_->setStyleSheet(active ? "font-weight: bold; padding: 6px 14px; background-color: #059669; color: white; border-radius: 4px;"
                                           : "font-weight: bold; padding: 6px 14px; background-color: #475569; color: #cbd5e1; border-radius: 4px;");
    btnToggleActive_->blockSignals(false);
}

#include "moc_MainWindow.cpp"
