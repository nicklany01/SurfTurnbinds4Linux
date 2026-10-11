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
    void onSelectLeftKeyClicked();
    void onClearLeftKeyClicked();
    void onSelectRightKeyClicked();
    void onClearRightKeyClicked();
    void onSelectCycleKeyClicked();
    void onClearCycleKeyClicked();
    void onSelectMasterToggleKeyClicked();
    void onClearMasterToggleKeyClicked();

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

    void loadConfig();
    void saveConfig();
    QString getConfigFilePath() const;

    TurnEngine* engine_{nullptr};

    QPushButton* btnToggleActive_{nullptr};
    QLabel* lblStatus_{nullptr};

    QPushButton* btnLeftKey_{nullptr};
    QPushButton* btnClearLeftKey_{nullptr};

    QPushButton* btnRightKey_{nullptr};
    QPushButton* btnClearRightKey_{nullptr};

    QPushButton* btnCycleKey_{nullptr};
    QPushButton* btnClearCycleKey_{nullptr};

    QPushButton* btnMasterToggleKey_{nullptr};
    QPushButton* btnClearMasterToggleKey_{nullptr};

    QDoubleSpinBox* spinSensitivity_{nullptr};

    QPushButton* btnAddProfile_{nullptr};
    QVBoxLayout* profilesListLayout_{nullptr};
    std::map<int, ProfileRowWidgets> profileWidgets_;

    int currentCapturingTarget_{CAPTURE_NONE};
    int currentCapturingId_{0};
};
