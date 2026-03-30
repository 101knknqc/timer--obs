#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QDockWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QFrame>
#include <QSpinBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFileDialog>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QElapsedTimer>
#include <QScrollArea>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSettings>

// ─────────────────────────────────────────────────────────────
//  Timer state
// ─────────────────────────────────────────────────────────────

enum class TimerState {
	Idle,
	Running,
	Paused,
	Finished
};

enum class ColorStage {
	White,   // full time
	Yellow,  // past half
	Orange,  // < 5 min
	Red,     // < 1 min  (also used for negative/overrun)
};

struct TimerData {
	int            id;
	QString        label;
	QString        obsSourceName;   // OBS text source to update
	int            durationSeconds; // total configured duration

	// runtime
	TimerState     state           = TimerState::Idle;
	qint64         remainingMs     = 0; // ms left when last paused/started
	qint64         startWallMs     = 0; // wall clock ms when last resumed
	ColorStage     currentStage    = ColorStage::White;

	// alerts (track which have fired this run)
	bool           alertHalfFired  = false;
	bool           alert5MinFired  = false;
	bool           alert1MinFired  = false;
	bool           alertZeroFired  = false;

	// flash state
	bool           flashing        = false;
	int            flashCount      = 0;
	bool           flashVisible    = true;

	// sound file — empty = use built-in embedded alert.wav
	QString        soundPath;
	bool           useBuiltinSound = true; // false once user picks a custom file
};

// ─────────────────────────────────────────────────────────────
//  Per-timer row widget
// ─────────────────────────────────────────────────────────────

class TimerRowWidget : public QFrame {
	Q_OBJECT
public:
	explicit TimerRowWidget(TimerData &data, QWidget *parent = nullptr);

	void refresh(qint64 nowMs); // called every tick
	void applyObsColor(ColorStage stage, bool visible);
	void updateSoundLabel(const QString &filename);

signals:
	void requestDelete(int id);
	void requestBrowseSound(int id);
	void requestPlayAlert(int id);   // fired on every colour-stage change + alerts

private slots:
	void onStartPause();
	void onReset();
	void onDelete();
	void onBrowseSound();
	void onSourceChanged(const QString &text);
	void onDurationChanged(int val);
	void onLabelChanged(const QString &text);
	void onSoundChanged(const QString &path);

private:
	QString formatTime(qint64 totalSeconds) const;
	ColorStage computeStage(qint64 remainingMs, int durationSeconds) const;

	TimerData      &m_data;

	QLabel         *m_timerLabel;
	QPushButton    *m_startPauseBtn;
	QPushButton    *m_resetBtn;
	QPushButton    *m_deleteBtn;

	QSpinBox       *m_durationSpin;
	QLineEdit      *m_labelEdit;
	QLineEdit      *m_sourceEdit;
	QLineEdit      *m_soundEdit;
	QPushButton    *m_browseBtn;

	// for flash animation
	QTimer         *m_flashTimer;
	int             m_flashesLeft = 0;
	ColorStage      m_pendingStage = ColorStage::White;
};

// ─────────────────────────────────────────────────────────────
//  Main dock
// ─────────────────────────────────────────────────────────────

class ConfidenceMonitorDock : public QDockWidget {
	Q_OBJECT
public:
	explicit ConfidenceMonitorDock(QWidget *parent = nullptr);
	~ConfidenceMonitorDock() override;

	static void Register();

private slots:
	void onAddTimer();
	void onTick();
	void onDeleteTimer(int id);
	void onBrowseSound(int id);

private:
	void           addTimerRow(TimerData data = {});
	void           saveSettings();
	void           loadSettings();
	void           playSound(const TimerData &d);
	void           rebuildLayout();

	QWidget        *m_container;
	QVBoxLayout    *m_listLayout;
	QPushButton    *m_addBtn;
	QTimer         *m_tickTimer;

	QVector<TimerData>       m_timers;
	QVector<TimerRowWidget*> m_rows;

	QMediaPlayer   *m_player;
	QAudioOutput   *m_audioOut;

	int             m_nextId = 1;
};
