#include "confidence-monitor.hpp"
#include <obs.h>
#include <QMainWindow>
#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════

static const char *STAGE_COLOR_CSS[] = {
	"color: #FFFFFF;",           // White
	"color: #FFE033;",           // Yellow
	"color: #FF8C00;",           // Orange
	"color: #FF2222;",           // Red
};

static const char *STAGE_OBS_COLOR[] = {
	"0xFFFFFFFF",   // White  (AABBGGRR in OBS)
	"0xFF33E0FF",   // Yellow
	"0xFF008CFF",   // Orange
	"0xFF2222FF",   // Red
};

// Convert AABBGGRR hex string to OBS color int
static uint32_t obsColorFromStage(ColorStage s)
{
	switch (s) {
	case ColorStage::White:  return 0xFFFFFFFF;
	case ColorStage::Yellow: return 0xFF33E0FF;
	case ColorStage::Orange: return 0xFF008CFF;
	case ColorStage::Red:    return 0xFF2222FF;
	}
	return 0xFFFFFFFF;
}

static void setObsTextSourceContent(const QString &sourceName,
                                    const QString &text,
                                    uint32_t color)
{
	if (sourceName.isEmpty()) return;

	obs_source_t *source = obs_get_source_by_name(
		sourceName.toUtf8().constData());
	if (!source) return;

	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "text", text.toUtf8().constData());
	obs_data_set_int(settings, "color1", color);
	obs_data_set_int(settings, "color2", color);
	obs_source_update(source, settings);
	obs_data_release(settings);
	obs_source_release(source);
}

// ═══════════════════════════════════════════════════════════════
//  TimerRowWidget
// ═══════════════════════════════════════════════════════════════

TimerRowWidget::TimerRowWidget(TimerData &data, QWidget *parent)
	: QFrame(parent), m_data(data)
{
	setObjectName("TimerRow");
	setFrameShape(QFrame::StyledPanel);
	setStyleSheet(
		"#TimerRow {"
		"  background: #1a1a1a;"
		"  border: 1px solid #333;"
		"  border-radius: 6px;"
		"  margin: 4px 0;"
		"}"
	);

	auto *root = new QVBoxLayout(this);
	root->setSpacing(8);
	root->setContentsMargins(12, 10, 12, 10);

	// ── Row 1: label + big timer display ──────────────────────
	auto *topRow = new QHBoxLayout();

	m_labelEdit = new QLineEdit(m_data.label);
	m_labelEdit->setPlaceholderText("Timer name…");
	m_labelEdit->setFixedWidth(120);
	m_labelEdit->setStyleSheet("color:#aaa; background:#111; border:1px solid #333; border-radius:4px; padding:2px 6px;");

	m_timerLabel = new QLabel("--:--");
	m_timerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_timerLabel->setStyleSheet(
		"font-family: 'Courier New', monospace;"
		"font-size: 42px;"
		"font-weight: 700;"
		"color: #FFFFFF;"
		"letter-spacing: 2px;"
	);

	topRow->addWidget(m_labelEdit);
	topRow->addStretch();
	topRow->addWidget(m_timerLabel);

	// ── Row 2: controls ───────────────────────────────────────
	auto *ctrlRow = new QHBoxLayout();

	m_durationSpin = new QSpinBox();
	m_durationSpin->setRange(1, 600);
	m_durationSpin->setValue(m_data.durationSeconds / 60);
	m_durationSpin->setSuffix(" min");
	m_durationSpin->setFixedWidth(90);
	m_durationSpin->setStyleSheet("color:#ccc; background:#111; border:1px solid #333; border-radius:4px; padding:2px 4px;");

	m_startPauseBtn = new QPushButton("▶  Start");
	m_startPauseBtn->setFixedWidth(90);
	m_startPauseBtn->setStyleSheet(
		"QPushButton { background:#2a7a2a; color:#fff; border:none; border-radius:4px; padding:5px 10px; font-weight:600; }"
		"QPushButton:hover { background:#3a9a3a; }"
	);

	m_resetBtn = new QPushButton("↺  Reset");
	m_resetBtn->setFixedWidth(80);
	m_resetBtn->setStyleSheet(
		"QPushButton { background:#333; color:#ccc; border:none; border-radius:4px; padding:5px 10px; }"
		"QPushButton:hover { background:#444; }"
	);

	m_deleteBtn = new QPushButton("✕");
	m_deleteBtn->setFixedSize(30, 30);
	m_deleteBtn->setStyleSheet(
		"QPushButton { background:#5a1a1a; color:#ff6666; border:none; border-radius:4px; font-weight:700; }"
		"QPushButton:hover { background:#7a2a2a; }"
	);

	ctrlRow->addWidget(m_durationSpin);
	ctrlRow->addWidget(m_startPauseBtn);
	ctrlRow->addWidget(m_resetBtn);
	ctrlRow->addStretch();
	ctrlRow->addWidget(m_deleteBtn);

	// ── Row 3: OBS source + sound ─────────────────────────────
	auto *srcRow = new QHBoxLayout();

	auto *srcLbl = new QLabel("OBS source:");
	srcLbl->setStyleSheet("color:#888; font-size:12px;");

	m_sourceEdit = new QLineEdit(m_data.obsSourceName);
	m_sourceEdit->setPlaceholderText("Text source name in OBS…");
	m_sourceEdit->setStyleSheet("color:#ccc; background:#111; border:1px solid #333; border-radius:4px; padding:2px 6px;");

	auto *sndLbl = new QLabel("Sound:");
	sndLbl->setStyleSheet("color:#888; font-size:12px;");

	m_soundEdit = new QLineEdit(m_data.soundPath);
	m_soundEdit->setPlaceholderText("alert.wav…");
	m_soundEdit->setReadOnly(true);
	m_soundEdit->setStyleSheet("color:#888; background:#111; border:1px solid #333; border-radius:4px; padding:2px 6px;");

	m_browseBtn = new QPushButton("…");
	m_browseBtn->setFixedSize(28, 28);
	m_browseBtn->setStyleSheet(
		"QPushButton { background:#333; color:#ccc; border:none; border-radius:4px; }"
		"QPushButton:hover { background:#444; }"
	);

	srcRow->addWidget(srcLbl);
	srcRow->addWidget(m_sourceEdit, 2);
	srcRow->addSpacing(12);
	srcRow->addWidget(sndLbl);
	srcRow->addWidget(m_soundEdit, 1);
	srcRow->addWidget(m_browseBtn);

	root->addLayout(topRow);
	root->addLayout(ctrlRow);
	root->addLayout(srcRow);

	// ── Flash timer ───────────────────────────────────────────
	m_flashTimer = new QTimer(this);
	m_flashTimer->setInterval(160); // 160ms per flash half-cycle

	// ── Connections ───────────────────────────────────────────
	connect(m_startPauseBtn, &QPushButton::clicked, this, &TimerRowWidget::onStartPause);
	connect(m_resetBtn,      &QPushButton::clicked, this, &TimerRowWidget::onReset);
	connect(m_deleteBtn,     &QPushButton::clicked, this, &TimerRowWidget::onDelete);
	connect(m_browseBtn,     &QPushButton::clicked, this, &TimerRowWidget::onBrowseSound);
	connect(m_sourceEdit,    &QLineEdit::textChanged, this, &TimerRowWidget::onSourceChanged);
	connect(m_durationSpin,  QOverload<int>::of(&QSpinBox::valueChanged), this, &TimerRowWidget::onDurationChanged);
	connect(m_labelEdit,     &QLineEdit::textChanged, this, &TimerRowWidget::onLabelChanged);

	connect(m_flashTimer, &QTimer::timeout, this, [this]() {
		m_data.flashVisible = !m_data.flashVisible;
		m_flashesLeft--;

		if (m_flashesLeft <= 0) {
			m_flashTimer->stop();
			m_data.flashing    = false;
			m_data.flashVisible = true;
			// Commit the new stage
			m_data.currentStage = m_pendingStage;
			QString css = QString(
				"font-family: 'Courier New', monospace;"
				"font-size: 42px;"
				"font-weight: 700;"
				"letter-spacing: 2px;"
				"%1"
			).arg(STAGE_COLOR_CSS[(int)m_data.currentStage]);
			m_timerLabel->setStyleSheet(css);
		} else {
			// Toggle visibility during flash
			m_timerLabel->setVisible(m_data.flashVisible);
		}
	});

	// Init display
	m_data.remainingMs = (qint64)m_data.durationSeconds * 1000LL;
	m_timerLabel->setText(formatTime(m_data.durationSeconds));
}

// ─────────────────────────────────────────────────────────────

QString TimerRowWidget::formatTime(qint64 totalSeconds) const
{
	bool negative = totalSeconds < 0;
	qint64 abs    = negative ? -totalSeconds : totalSeconds;
	int mm        = (int)(abs / 60);
	int ss        = (int)(abs % 60);
	return QString("%1%2:%3")
		.arg(negative ? "-" : "")
		.arg(mm, 2, 10, QChar('0'))
		.arg(ss, 2, 10, QChar('0'));
}

ColorStage TimerRowWidget::computeStage(qint64 remainingMs,
                                        int    durationSeconds) const
{
	if (remainingMs <= 0)
		return ColorStage::Red;

	qint64 remSec  = remainingMs / 1000;
	qint64 halfSec = durationSeconds / 2;

	if (remSec < 60)
		return ColorStage::Red;
	if (remSec < 300)   // < 5 min
		return ColorStage::Orange;
	if (remSec < halfSec)
		return ColorStage::Yellow;

	return ColorStage::White;
}

// Called every 100ms from dock tick
void TimerRowWidget::refresh(qint64 nowMs)
{
	// ── Compute remaining time ────────────────────────────────
	qint64 remainingMs = m_data.remainingMs;

	if (m_data.state == TimerState::Running) {
		qint64 elapsed = nowMs - m_data.startWallMs;
		remainingMs    = m_data.remainingMs - elapsed;
	}

	// ── Format display ────────────────────────────────────────
	qint64 totalSec  = remainingMs / 1000;
	if (remainingMs < 0 && (remainingMs % 1000) != 0)
		totalSec--; // floor toward -inf

	QString timeStr  = formatTime(totalSec);

	if (!m_data.flashing) {
		m_timerLabel->setVisible(true);
		m_timerLabel->setText(timeStr);
	} else {
		// Flash in progress — text already toggled by flashTimer
		m_timerLabel->setText(timeStr);
	}

	// ── Update OBS text source ────────────────────────────────
	if (m_data.state == TimerState::Running || m_data.state == TimerState::Finished) {
		bool obsVisible = !m_data.flashing || m_data.flashVisible;
		uint32_t obsColor = obsColorFromStage(m_data.currentStage);
		setObsTextSourceContent(
			m_data.obsSourceName,
			obsVisible ? timeStr : "",
			obsColor
		);
	}

	if (m_data.state != TimerState::Running) return;

	// ── Check stage transitions ───────────────────────────────
	ColorStage newStage = computeStage(remainingMs, m_data.durationSeconds);

	if (newStage != m_data.currentStage && !m_data.flashing) {
		// Trigger 2 flash cycles (4 half-cycles = 2 full blinks)
		m_pendingStage      = newStage;
		m_data.flashing     = true;
		m_data.flashVisible = true;
		m_flashesLeft       = 4;
		m_flashTimer->start();

		emit requestPlayAlert(m_data.id);
	}

	// ── Update button style live ──────────────────────────────
	if (!m_data.flashing) {
		QString css = QString(
			"font-family: 'Courier New', monospace;"
			"font-size: 42px;"
			"font-weight: 700;"
			"letter-spacing: 2px;"
			"%1"
		).arg(STAGE_COLOR_CSS[(int)m_data.currentStage]);
		m_timerLabel->setStyleSheet(css);
	}
}

// ─────────────────────────────────────────────────────────────
//  Slots
// ─────────────────────────────────────────────────────────────

void TimerRowWidget::onStartPause()
{
	qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

	if (m_data.state == TimerState::Idle ||
	    m_data.state == TimerState::Finished) {
		// Fresh start
		m_data.remainingMs    = (qint64)m_data.durationSeconds * 1000LL;
		m_data.startWallMs    = nowMs;
		m_data.state          = TimerState::Running;
		m_data.currentStage   = ColorStage::White;
		m_data.alertHalfFired = false;
		m_data.alert5MinFired = false;
		m_data.alert1MinFired = false;
		m_data.alertZeroFired = false;
		m_startPauseBtn->setText("⏸  Pause");
		m_startPauseBtn->setStyleSheet(
			"QPushButton { background:#7a5a00; color:#fff; border:none; border-radius:4px; padding:5px 10px; font-weight:600; }"
			"QPushButton:hover { background:#9a7a00; }"
		);

	} else if (m_data.state == TimerState::Running) {
		// Pause — snapshot remaining
		qint64 elapsed     = nowMs - m_data.startWallMs;
		m_data.remainingMs = m_data.remainingMs - elapsed;
		m_data.state       = TimerState::Paused;
		m_startPauseBtn->setText("▶  Resume");
		m_startPauseBtn->setStyleSheet(
			"QPushButton { background:#2a7a2a; color:#fff; border:none; border-radius:4px; padding:5px 10px; font-weight:600; }"
			"QPushButton:hover { background:#3a9a3a; }"
		);

	} else if (m_data.state == TimerState::Paused) {
		// Resume
		m_data.startWallMs = nowMs;
		m_data.state       = TimerState::Running;
		m_startPauseBtn->setText("⏸  Pause");
		m_startPauseBtn->setStyleSheet(
			"QPushButton { background:#7a5a00; color:#fff; border:none; border-radius:4px; padding:5px 10px; font-weight:600; }"
			"QPushButton:hover { background:#9a7a00; }"
		);
	}
}

void TimerRowWidget::onReset()
{
	m_flashTimer->stop();
	m_data.state          = TimerState::Idle;
	m_data.remainingMs    = (qint64)m_data.durationSeconds * 1000LL;
	m_data.currentStage   = ColorStage::White;
	m_data.flashing       = false;
	m_data.flashVisible   = true;
	m_data.alertHalfFired = false;
	m_data.alert5MinFired = false;
	m_data.alert1MinFired = false;
	m_data.alertZeroFired = false;

	m_timerLabel->setVisible(true);
	m_timerLabel->setText(formatTime(m_data.durationSeconds));
	m_timerLabel->setStyleSheet(
		"font-family: 'Courier New', monospace;"
		"font-size: 42px;"
		"font-weight: 700;"
		"letter-spacing: 2px;"
		"color: #FFFFFF;"
	);

	m_startPauseBtn->setText("▶  Start");
	m_startPauseBtn->setStyleSheet(
		"QPushButton { background:#2a7a2a; color:#fff; border:none; border-radius:4px; padding:5px 10px; font-weight:600; }"
		"QPushButton:hover { background:#3a9a3a; }"
	);

	// Clear OBS source
	setObsTextSourceContent(
		m_data.obsSourceName,
		formatTime(m_data.durationSeconds),
		obsColorFromStage(ColorStage::White)
	);
}

void TimerRowWidget::onDelete()
{
	m_flashTimer->stop();
	emit requestDelete(m_data.id);
}

void TimerRowWidget::onBrowseSound()
{
	emit requestBrowseSound(m_data.id);
}

void TimerRowWidget::onSourceChanged(const QString &text)
{
	m_data.obsSourceName = text;
}

void TimerRowWidget::onDurationChanged(int val)
{
	m_data.durationSeconds = val * 60;
	if (m_data.state == TimerState::Idle) {
		m_data.remainingMs = (qint64)m_data.durationSeconds * 1000LL;
		m_timerLabel->setText(formatTime(m_data.durationSeconds));
	}
}

void TimerRowWidget::onLabelChanged(const QString &text)
{
	m_data.label = text;
}

void TimerRowWidget::onSoundChanged(const QString &path)
{
	m_data.soundPath       = path;
	m_data.useBuiltinSound = false;
	m_soundEdit->setText(QFileInfo(path).fileName());
}

void TimerRowWidget::updateSoundLabel(const QString &filename)
{
	m_soundEdit->setText(filename);
}
