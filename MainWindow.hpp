#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <map>
#include "TurnEngine.hpp"

class QEnterEvent;

struct ProfileRowWidgets {
    int id;
    QWidget* container;
    QRadioButton* radioActive;
    QSlider* slider;
    QDoubleSpinBox* spinSpeed;
    QPushButton* btnKey;
    QPushButton* btnClearKey;
    QPushButton* btnRemove;
};

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onToggleActive();

    void onSelectLeftKey1Clicked();
    void onClearLeftKey1Clicked();
    void onSelectLeftKey2Clicked();
    void onClearLeftKey2Clicked();

    void onSelectRightKey1Clicked();
    void onClearRightKey1Clicked();
    void onSelectRightKey2Clicked();
    void onClearRightKey2Clicked();

    void onSelectCycleKey1Clicked();
    void onClearCycleKey1Clicked();
    void onSelectCycleKey2Clicked();
    void onClearCycleKey2Clicked();

    void onSelectMasterToggleKey1Clicked();
    void onClearMasterToggleKey1Clicked();
    void onSelectMasterToggleKey2Clicked();
    void onClearMasterToggleKey2Clicked();

    void onAddProfileClicked();
    void onRemoveProfileClicked(int id);
    void onProfileRadioClicked(int id);
    void onProfileSliderChanged(int id, int value);
    void onProfileSpinChanged(int id, double value);
    void onProfileKeyClicked(int id);
    void onProfileKeyClearClicked(int id);

    void onSensitivityChanged(double val);
    void onKeyCaptured(int target, int targetId, int keycode, const QString& keyName);
    void onActiveProfileChanged(int profileId);
    void onEngineStateChanged(bool active, const QString& message);

private:
    void updateWindowFocusState();
    void setupUi();
    void rebuildProfileList();
    void updateActiveRowHighlight(int activeId);
    void resetAllCaptureButtonTexts();

    void loadConfig();
    void saveConfig();
    QString getConfigFilePath() const;

    TurnEngine* engine_{nullptr};

    QPushButton* btnToggleActive_{nullptr};
    QLabel* lblStatus_{nullptr};

    QPushButton* btnLeftKey1_{nullptr};
    QPushButton* btnClearLeftKey1_{nullptr};
    QPushButton* btnLeftKey2_{nullptr};
    QPushButton* btnClearLeftKey2_{nullptr};

    QPushButton* btnRightKey1_{nullptr};
    QPushButton* btnClearRightKey1_{nullptr};
    QPushButton* btnRightKey2_{nullptr};
    QPushButton* btnClearRightKey2_{nullptr};

    QPushButton* btnCycleKey1_{nullptr};
    QPushButton* btnClearCycleKey1_{nullptr};
    QPushButton* btnCycleKey2_{nullptr};
    QPushButton* btnClearCycleKey2_{nullptr};

    QPushButton* btnMasterToggleKey1_{nullptr};
    QPushButton* btnClearMasterToggleKey1_{nullptr};
    QPushButton* btnMasterToggleKey2_{nullptr};
    QPushButton* btnClearMasterToggleKey2_{nullptr};

    QDoubleSpinBox* spinSensitivity_{nullptr};

    QPushButton* btnAddProfile_{nullptr};
    QVBoxLayout* profilesListLayout_{nullptr};
    std::map<int, ProfileRowWidgets> profileWidgets_;

    int currentCapturingTarget_{CAPTURE_NONE};
    int currentCapturingId_{0};
};
