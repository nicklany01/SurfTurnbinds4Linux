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

    int leftKey1 = settings.value("General/left_key_1", settings.value("General/left_key", KEY_H)).toInt();
    int leftKey2 = settings.value("General/left_key_2", 0).toInt();

    int rightKey1 = settings.value("General/right_key_1", settings.value("General/right_key", KEY_L)).toInt();
    int rightKey2 = settings.value("General/right_key_2", 0).toInt();

    int cycleKey1 = settings.value("General/cycle_key_1", settings.value("General/cycle_key", 0)).toInt();
    int cycleKey2 = settings.value("General/cycle_key_2", 0).toInt();

    int toggleKey1 = settings.value("General/toggle_key_1", settings.value("General/toggle_key", 0)).toInt();
    int toggleKey2 = settings.value("General/toggle_key_2", 0).toInt();

    double sens = settings.value("General/sensitivity", 1.0).toDouble();
    int activeId = settings.value("General/active_profile", 1).toInt();

    engine_->setLeftKey1(leftKey1);
    engine_->setLeftKey2(leftKey2);
    engine_->setRightKey1(rightKey1);
    engine_->setRightKey2(rightKey2);
    engine_->setCycleKey1(cycleKey1);
    engine_->setCycleKey2(cycleKey2);
    engine_->setToggleActiveKey1(toggleKey1);
    engine_->setToggleActiveKey2(toggleKey2);
    engine_->setSensitivity(sens);

    spinSensitivity_->blockSignals(true);
    spinSensitivity_->setValue(sens);
    spinSensitivity_->blockSignals(false);

    btnLeftKey1_->setText(QString::fromStdString(KeyNames::getName(leftKey1)));
    btnLeftKey2_->setText(QString::fromStdString(KeyNames::getName(leftKey2)));
    btnRightKey1_->setText(QString::fromStdString(KeyNames::getName(rightKey1)));
    btnRightKey2_->setText(QString::fromStdString(KeyNames::getName(rightKey2)));
    btnCycleKey1_->setText(QString::fromStdString(KeyNames::getName(cycleKey1)));
    btnCycleKey2_->setText(QString::fromStdString(KeyNames::getName(cycleKey2)));
    btnMasterToggleKey1_->setText(QString::fromStdString(KeyNames::getName(toggleKey1)));
    btnMasterToggleKey2_->setText(QString::fromStdString(KeyNames::getName(toggleKey2)));

    int pCount = settings.value("Profiles/count", 0).toInt();
    if (pCount > 0) {
        std::vector<YawProfile> list;
        for (int i = 0; i < pCount && i < 5; ++i) {
            QString prefix = QString("Profiles/profile_%1_").arg(i);
            int id = settings.value(prefix + "id", i + 1).toInt();
            double speed = settings.value(prefix + "speed", 210.0).toDouble();
            int key = settings.value(prefix + "key", settings.value(prefix + "key1", 0)).toInt();
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

    settings.setValue("General/left_key", engine_->getLeftKey1());
    settings.setValue("General/left_key_1", engine_->getLeftKey1());
    settings.setValue("General/left_key_2", engine_->getLeftKey2());

    settings.setValue("General/right_key", engine_->getRightKey1());
    settings.setValue("General/right_key_1", engine_->getRightKey1());
    settings.setValue("General/right_key_2", engine_->getRightKey2());

    settings.setValue("General/cycle_key", engine_->getCycleKey1());
    settings.setValue("General/cycle_key_1", engine_->getCycleKey1());
    settings.setValue("General/cycle_key_2", engine_->getCycleKey2());

    settings.setValue("General/toggle_key", engine_->getToggleActiveKey1());
    settings.setValue("General/toggle_key_1", engine_->getToggleActiveKey1());
    settings.setValue("General/toggle_key_2", engine_->getToggleActiveKey2());

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

void MainWindow::resetAllCaptureButtonTexts() {
    btnLeftKey1_->setText(QString::fromStdString(KeyNames::getName(engine_->getLeftKey1())));
    btnLeftKey2_->setText(QString::fromStdString(KeyNames::getName(engine_->getLeftKey2())));
    btnRightKey1_->setText(QString::fromStdString(KeyNames::getName(engine_->getRightKey1())));
    btnRightKey2_->setText(QString::fromStdString(KeyNames::getName(engine_->getRightKey2())));
    btnCycleKey1_->setText(QString::fromStdString(KeyNames::getName(engine_->getCycleKey1())));
    btnCycleKey2_->setText(QString::fromStdString(KeyNames::getName(engine_->getCycleKey2())));
    btnMasterToggleKey1_->setText(QString::fromStdString(KeyNames::getName(engine_->getToggleActiveKey1())));
    btnMasterToggleKey2_->setText(QString::fromStdString(KeyNames::getName(engine_->getToggleActiveKey2())));

    auto profiles = engine_->getProfiles();
    for (const auto& p : profiles) {
        auto it = profileWidgets_.find(p.id);
        if (it != profileWidgets_.end()) {
            QString k = (p.switchKey != 0) ? QString::fromStdString(KeyNames::getName(p.switchKey)) : "None";
            it->second.btnKey->setText("Key: [" + k + "]");
        }
    }
}

void MainWindow::setupUi() {
    setWindowTitle("Surf Turnbinds (Linux)");
    resize(580, 640);
    setMinimumWidth(520);
    setMinimumHeight(480);

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
    btnToggleActive_->setStyleSheet("font-weight: bold; padding: 6px 14px; background-color: #059669; color: white; border-radius: 4px;" );
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
    grid->setContentsMargins(12, 14, 12, 14);

    auto* lblHdrAction = new QLabel("Action", this);
    lblHdrAction->setStyleSheet("font-weight: bold; color: #94a3b8; font-size: 11px;");
    grid->addWidget(lblHdrAction, 0, 0);

    auto* lblHdrKey1 = new QLabel("Key 1 (Primary)", this);
    lblHdrKey1->setStyleSheet("font-weight: bold; color: #94a3b8; font-size: 11px;");
    grid->addWidget(lblHdrKey1, 0, 1, 1, 2);

    auto* lblHdrKey2 = new QLabel("Key 2 (Secondary / Combo)", this);
    lblHdrKey2->setStyleSheet("font-weight: bold; color: #94a3b8; font-size: 11px;");
    grid->addWidget(lblHdrKey2, 0, 3, 1, 2);

    // Left Turn
    grid->addWidget(new QLabel("Turn Left:", this), 1, 0);
    btnLeftKey1_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getLeftKey1())), this);
    connect(btnLeftKey1_, &QPushButton::clicked, this, &MainWindow::onSelectLeftKey1Clicked);
    grid->addWidget(btnLeftKey1_, 1, 1);

    btnClearLeftKey1_ = new QPushButton("Clear", this);
    btnClearLeftKey1_->setFixedWidth(56);
    connect(btnClearLeftKey1_, &QPushButton::clicked, this, &MainWindow::onClearLeftKey1Clicked);
    grid->addWidget(btnClearLeftKey1_, 1, 2);

    btnLeftKey2_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getLeftKey2())), this);
    connect(btnLeftKey2_, &QPushButton::clicked, this, &MainWindow::onSelectLeftKey2Clicked);
    grid->addWidget(btnLeftKey2_, 1, 3);

    btnClearLeftKey2_ = new QPushButton("Clear", this);
    btnClearLeftKey2_->setFixedWidth(56);
    connect(btnClearLeftKey2_, &QPushButton::clicked, this, &MainWindow::onClearLeftKey2Clicked);
    grid->addWidget(btnClearLeftKey2_, 1, 4);

    // Right Turn
    grid->addWidget(new QLabel("Turn Right:", this), 2, 0);
    btnRightKey1_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getRightKey1())), this);
    connect(btnRightKey1_, &QPushButton::clicked, this, &MainWindow::onSelectRightKey1Clicked);
    grid->addWidget(btnRightKey1_, 2, 1);

    btnClearRightKey1_ = new QPushButton("Clear", this);
    btnClearRightKey1_->setFixedWidth(56);
    connect(btnClearRightKey1_, &QPushButton::clicked, this, &MainWindow::onClearRightKey1Clicked);
    grid->addWidget(btnClearRightKey1_, 2, 2);

    btnRightKey2_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getRightKey2())), this);
    connect(btnRightKey2_, &QPushButton::clicked, this, &MainWindow::onSelectRightKey2Clicked);
    grid->addWidget(btnRightKey2_, 2, 3);

    btnClearRightKey2_ = new QPushButton("Clear", this);
    btnClearRightKey2_->setFixedWidth(56);
    connect(btnClearRightKey2_, &QPushButton::clicked, this, &MainWindow::onClearRightKey2Clicked);
    grid->addWidget(btnClearRightKey2_, 2, 4);

    // Cycle Next Speed Key
    grid->addWidget(new QLabel("Cycle Speeds:", this), 3, 0);
    btnCycleKey1_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getCycleKey1())), this);
    connect(btnCycleKey1_, &QPushButton::clicked, this, &MainWindow::onSelectCycleKey1Clicked);
    grid->addWidget(btnCycleKey1_, 3, 1);

    btnClearCycleKey1_ = new QPushButton("Clear", this);
    btnClearCycleKey1_->setFixedWidth(56);
    connect(btnClearCycleKey1_, &QPushButton::clicked, this, &MainWindow::onClearCycleKey1Clicked);
    grid->addWidget(btnClearCycleKey1_, 3, 2);

    btnCycleKey2_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getCycleKey2())), this);
    connect(btnCycleKey2_, &QPushButton::clicked, this, &MainWindow::onSelectCycleKey2Clicked);
    grid->addWidget(btnCycleKey2_, 3, 3);

    btnClearCycleKey2_ = new QPushButton("Clear", this);
    btnClearCycleKey2_->setFixedWidth(56);
    connect(btnClearCycleKey2_, &QPushButton::clicked, this, &MainWindow::onClearCycleKey2Clicked);
    grid->addWidget(btnClearCycleKey2_, 3, 4);

    // Master Toggle Key (Combo: Key 1 + Key 2, e.g. Ctrl + B)
    auto* lblMasterToggle = new QLabel("Master Toggle (Combo):", this);
    lblMasterToggle->setToolTip("Press both keys simultaneously (e.g. Left Ctrl + B) to toggle turnbinds");
    grid->addWidget(lblMasterToggle, 4, 0);

    btnMasterToggleKey1_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getToggleActiveKey1())), this);
    btnMasterToggleKey1_->setToolTip("Combo Key 1 (e.g. Left Ctrl)");
    connect(btnMasterToggleKey1_, &QPushButton::clicked, this, &MainWindow::onSelectMasterToggleKey1Clicked);
    grid->addWidget(btnMasterToggleKey1_, 4, 1);

    btnClearMasterToggleKey1_ = new QPushButton("Clear", this);
    btnClearMasterToggleKey1_->setFixedWidth(56);
    connect(btnClearMasterToggleKey1_, &QPushButton::clicked, this, &MainWindow::onClearMasterToggleKey1Clicked);
    grid->addWidget(btnClearMasterToggleKey1_, 4, 2);

    btnMasterToggleKey2_ = new QPushButton(QString::fromStdString(KeyNames::getName(engine_->getToggleActiveKey2())), this);
    btnMasterToggleKey2_->setToolTip("Combo Key 2 (e.g. B)");
    connect(btnMasterToggleKey2_, &QPushButton::clicked, this, &MainWindow::onSelectMasterToggleKey2Clicked);
    grid->addWidget(btnMasterToggleKey2_, 4, 3);

    btnClearMasterToggleKey2_ = new QPushButton("Clear", this);
    btnClearMasterToggleKey2_->setFixedWidth(56);
    connect(btnClearMasterToggleKey2_, &QPushButton::clicked, this, &MainWindow::onClearMasterToggleKey2Clicked);
    grid->addWidget(btnClearMasterToggleKey2_, 4, 4);

    // In-game sensitivity
    grid->addWidget(new QLabel("In-Game Sens (800 DPI):", this), 5, 0);
    spinSensitivity_ = new QDoubleSpinBox(this);
    spinSensitivity_->setRange(0.1, 10.0);
    spinSensitivity_->setSingleStep(0.05);
    spinSensitivity_->setValue(1.0);
    connect(spinSensitivity_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onSensitivityChanged);
    grid->addWidget(spinSensitivity_, 5, 1, 1, 4);

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
        btnClearKey->setFixedWidth(56);
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

// Left Key 1 & 2
void MainWindow::onSelectLeftKey1Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_LEFT_KEY_1;
    currentCapturingId_ = 0;
    btnLeftKey1_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_LEFT_KEY_1);
}

void MainWindow::onClearLeftKey1Clicked() {
    engine_->cancelCapture();
    engine_->setLeftKey1(0);
    btnLeftKey1_->setText("None");
    saveConfig();
}

void MainWindow::onSelectLeftKey2Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_LEFT_KEY_2;
    currentCapturingId_ = 0;
    btnLeftKey2_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_LEFT_KEY_2);
}

void MainWindow::onClearLeftKey2Clicked() {
    engine_->cancelCapture();
    engine_->setLeftKey2(0);
    btnLeftKey2_->setText("None");
    saveConfig();
}

// Right Key 1 & 2
void MainWindow::onSelectRightKey1Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_RIGHT_KEY_1;
    currentCapturingId_ = 0;
    btnRightKey1_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_RIGHT_KEY_1);
}

void MainWindow::onClearRightKey1Clicked() {
    engine_->cancelCapture();
    engine_->setRightKey1(0);
    btnRightKey1_->setText("None");
    saveConfig();
}

void MainWindow::onSelectRightKey2Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_RIGHT_KEY_2;
    currentCapturingId_ = 0;
    btnRightKey2_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_RIGHT_KEY_2);
}

void MainWindow::onClearRightKey2Clicked() {
    engine_->cancelCapture();
    engine_->setRightKey2(0);
    btnRightKey2_->setText("None");
    saveConfig();
}

// Cycle Key 1 & 2
void MainWindow::onSelectCycleKey1Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_CYCLE_KEY_1;
    currentCapturingId_ = 0;
    btnCycleKey1_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_CYCLE_KEY_1);
}

void MainWindow::onClearCycleKey1Clicked() {
    engine_->cancelCapture();
    engine_->setCycleKey1(0);
    btnCycleKey1_->setText("None");
    saveConfig();
}

void MainWindow::onSelectCycleKey2Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_CYCLE_KEY_2;
    currentCapturingId_ = 0;
    btnCycleKey2_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_CYCLE_KEY_2);
}

void MainWindow::onClearCycleKey2Clicked() {
    engine_->cancelCapture();
    engine_->setCycleKey2(0);
    btnCycleKey2_->setText("None");
    saveConfig();
}

// Master Toggle Key 1 & 2
void MainWindow::onSelectMasterToggleKey1Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_TOGGLE_ACTIVE_KEY_1;
    currentCapturingId_ = 0;
    btnMasterToggleKey1_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_TOGGLE_ACTIVE_KEY_1);
}

void MainWindow::onClearMasterToggleKey1Clicked() {
    engine_->cancelCapture();
    engine_->setToggleActiveKey1(0);
    btnMasterToggleKey1_->setText("None");
    saveConfig();
}

void MainWindow::onSelectMasterToggleKey2Clicked() {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_TOGGLE_ACTIVE_KEY_2;
    currentCapturingId_ = 0;
    btnMasterToggleKey2_->setText("Press key (Esc unsets)...");
    engine_->startCapture(CAPTURE_TOGGLE_ACTIVE_KEY_2);
}

void MainWindow::onClearMasterToggleKey2Clicked() {
    engine_->cancelCapture();
    engine_->setToggleActiveKey2(0);
    btnMasterToggleKey2_->setText("None");
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

// Profile Key (Single key)
void MainWindow::onProfileKeyClicked(int id) {
    resetAllCaptureButtonTexts();
    currentCapturingTarget_ = CAPTURE_PROFILE_KEY;
    currentCapturingId_ = id;
    auto it = profileWidgets_.find(id);
    if (it != profileWidgets_.end()) {
        it->second.btnKey->setText("Press key...");
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
        switch (target) {
            case CAPTURE_LEFT_KEY_1:
                engine_->setLeftKey1(0);
                btnLeftKey1_->setText("None");
                break;
            case CAPTURE_LEFT_KEY_2:
                engine_->setLeftKey2(0);
                btnLeftKey2_->setText("None");
                break;
            case CAPTURE_RIGHT_KEY_1:
                engine_->setRightKey1(0);
                btnRightKey1_->setText("None");
                break;
            case CAPTURE_RIGHT_KEY_2:
                engine_->setRightKey2(0);
                btnRightKey2_->setText("None");
                break;
            case CAPTURE_CYCLE_KEY_1:
                engine_->setCycleKey1(0);
                btnCycleKey1_->setText("None");
                break;
            case CAPTURE_CYCLE_KEY_2:
                engine_->setCycleKey2(0);
                btnCycleKey2_->setText("None");
                break;
            case CAPTURE_TOGGLE_ACTIVE_KEY_1:
                engine_->setToggleActiveKey1(0);
                btnMasterToggleKey1_->setText("None");
                break;
            case CAPTURE_TOGGLE_ACTIVE_KEY_2:
                engine_->setToggleActiveKey2(0);
                btnMasterToggleKey2_->setText("None");
                break;
            case CAPTURE_PROFILE_KEY: {
                engine_->setProfileKey(targetId, 0);
                auto it = profileWidgets_.find(targetId);
                if (it != profileWidgets_.end()) {
                    it->second.btnKey->setText("Key: [None]");
                }
                break;
            }
            default:
                break;
        }
        saveConfig();
        return;
    }

    // 2. Conflict Detection across all existing binds
    QString conflictAction = "";
    int conflictType = 0;
    int conflictProfileId = 0;

    if (target != CAPTURE_LEFT_KEY_1 && engine_->getLeftKey1() == keycode) {
        conflictAction = "Turn Left (Key 1)";
        conflictType = CAPTURE_LEFT_KEY_1;
    } else if (target != CAPTURE_LEFT_KEY_2 && engine_->getLeftKey2() == keycode) {
        conflictAction = "Turn Left (Key 2)";
        conflictType = CAPTURE_LEFT_KEY_2;
    } else if (target != CAPTURE_RIGHT_KEY_1 && engine_->getRightKey1() == keycode) {
        conflictAction = "Turn Right (Key 1)";
        conflictType = CAPTURE_RIGHT_KEY_1;
    } else if (target != CAPTURE_RIGHT_KEY_2 && engine_->getRightKey2() == keycode) {
        conflictAction = "Turn Right (Key 2)";
        conflictType = CAPTURE_RIGHT_KEY_2;
    } else if (target != CAPTURE_CYCLE_KEY_1 && engine_->getCycleKey1() == keycode) {
        conflictAction = "Cycle Speeds (Key 1)";
        conflictType = CAPTURE_CYCLE_KEY_1;
    } else if (target != CAPTURE_CYCLE_KEY_2 && engine_->getCycleKey2() == keycode) {
        conflictAction = "Cycle Speeds (Key 2)";
        conflictType = CAPTURE_CYCLE_KEY_2;
    } else if (target != CAPTURE_TOGGLE_ACTIVE_KEY_1 && engine_->getToggleActiveKey1() == keycode) {
        conflictAction = "Master Toggle (Key 1)";
        conflictType = CAPTURE_TOGGLE_ACTIVE_KEY_1;
    } else if (target != CAPTURE_TOGGLE_ACTIVE_KEY_2 && engine_->getToggleActiveKey2() == keycode) {
        conflictAction = "Master Toggle (Key 2)";
        conflictType = CAPTURE_TOGGLE_ACTIVE_KEY_2;
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

    // 3. Clear conflicting previous bind
    if (conflictType != 0) {
        switch (conflictType) {
            case CAPTURE_LEFT_KEY_1:
                engine_->setLeftKey1(0);
                btnLeftKey1_->setText("None");
                break;
            case CAPTURE_LEFT_KEY_2:
                engine_->setLeftKey2(0);
                btnLeftKey2_->setText("None");
                break;
            case CAPTURE_RIGHT_KEY_1:
                engine_->setRightKey1(0);
                btnRightKey1_->setText("None");
                break;
            case CAPTURE_RIGHT_KEY_2:
                engine_->setRightKey2(0);
                btnRightKey2_->setText("None");
                break;
            case CAPTURE_CYCLE_KEY_1:
                engine_->setCycleKey1(0);
                btnCycleKey1_->setText("None");
                break;
            case CAPTURE_CYCLE_KEY_2:
                engine_->setCycleKey2(0);
                btnCycleKey2_->setText("None");
                break;
            case CAPTURE_TOGGLE_ACTIVE_KEY_1:
                engine_->setToggleActiveKey1(0);
                btnMasterToggleKey1_->setText("None");
                break;
            case CAPTURE_TOGGLE_ACTIVE_KEY_2:
                engine_->setToggleActiveKey2(0);
                btnMasterToggleKey2_->setText("None");
                break;
            case CAPTURE_PROFILE_KEY: {
                engine_->setProfileKey(conflictProfileId, 0);
                auto it = profileWidgets_.find(conflictProfileId);
                if (it != profileWidgets_.end()) {
                    it->second.btnKey->setText("Key: [None]");
                }
                break;
            }
            default:
                break;
        }
    }

    // 4. Assign new bind to target
    switch (target) {
        case CAPTURE_LEFT_KEY_1:
            engine_->setLeftKey1(keycode);
            btnLeftKey1_->setText(keyName);
            break;
        case CAPTURE_LEFT_KEY_2:
            engine_->setLeftKey2(keycode);
            btnLeftKey2_->setText(keyName);
            break;
        case CAPTURE_RIGHT_KEY_1:
            engine_->setRightKey1(keycode);
            btnRightKey1_->setText(keyName);
            break;
        case CAPTURE_RIGHT_KEY_2:
            engine_->setRightKey2(keycode);
            btnRightKey2_->setText(keyName);
            break;
        case CAPTURE_CYCLE_KEY_1:
            engine_->setCycleKey1(keycode);
            btnCycleKey1_->setText(keyName);
            break;
        case CAPTURE_CYCLE_KEY_2:
            engine_->setCycleKey2(keycode);
            btnCycleKey2_->setText(keyName);
            break;
        case CAPTURE_TOGGLE_ACTIVE_KEY_1:
            engine_->setToggleActiveKey1(keycode);
            btnMasterToggleKey1_->setText(keyName);
            break;
        case CAPTURE_TOGGLE_ACTIVE_KEY_2:
            engine_->setToggleActiveKey2(keycode);
            btnMasterToggleKey2_->setText(keyName);
            break;
        case CAPTURE_PROFILE_KEY: {
            engine_->setProfileKey(targetId, keycode);
            auto it = profileWidgets_.find(targetId);
            if (it != profileWidgets_.end()) {
                it->second.btnKey->setText("Key: [" + keyName + "]");
            }
            break;
        }
        default:
            break;
    }

    saveConfig();

    // 5. Alert user if previous bind was overwritten
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
