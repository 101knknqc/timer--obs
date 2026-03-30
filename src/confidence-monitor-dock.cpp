// confidence-monitor-dock.cpp
// Append to confidence-monitor.cpp at link time via CMake — or include directly.

#include "confidence-monitor.hpp"
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QScrollArea>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>

// ═══════════════════════════════════════════════════════════════
//  ConfidenceMonitorDock — constructor
// ═══════════════════════════════════════════════════════════════

ConfidenceMonitorDock::ConfidenceMonitorDock(QWidget *parent)
	: QDockWidget("Confidence Monitor", parent)
{
	setObjectName("ConfidenceMonitorDock");
	setFeatures(QDockWidget::DockWidgetMovable |
	            QDockWidget::DockWidgetFloatable |
	            QDockWidget::DockWidgetClosable);

	// ── Root widget ───────────────────────────────────────────
	auto *root = new QWidget(this);
	root->setStyleSheet("background: #0d0d0d;");
	setWidget(root);

	auto *rootLayout = new QVBoxLayout(root);
	rootLayout->setContentsMargins(8, 8, 8, 8);
	rootLayout->setSpacing(0);

	// ── Header ────────────────────────────────────────────────
	auto *header = new QLabel("🎬  Confidence Monitor");
	header->setStyleSheet(
		"color: #fff;"
		"font-size: 14px;"
		"font-weight: 700;"
		"padding: 6px 4px 10px 4px;"
		"border-bottom: 1px solid #2a2a2a;"
	);
	rootLayout->addWidget(header);

	// ── Scroll area for timer rows ────────────────────────────
	auto *scrollArea = new QScrollArea();
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setStyleSheet("background: transparent; border: none;");

	m_container = new QWidget();
	m_container->setStyleSheet("background: transparent;");
	m_listLayout = new QVBoxLayout(m_container);
	m_listLayout->setSpacing(6);
	m_listLayout->setContentsMargins(0, 8, 0, 8);
	m_listLayout->addStretch();

	scrollArea->setWidget(m_container);
	rootLayout->addWidget(scrollArea, 1);

	// ── Add timer button ──────────────────────────────────────
	m_addBtn = new QPushButton("＋  Add Timer");
	m_addBtn->setFixedHeight(36);
	m_addBtn->setStyleSheet(
		"QPushButton {"
		"  background: #1a1a2e;"
		"  color: #7788ff;"
		"  border: 1px solid #2a2a5a;"
		"  border-radius: 5px;"
		"  font-size: 13px;"
		"  font-weight: 600;"
		"  margin-top: 6px;"
		"}"
		"QPushButton:hover { background: #22224a; }"
	);
	rootLayout->addWidget(m_addBtn);
	connect(m_addBtn, &QPushButton::clicked, this, &ConfidenceMonitorDock::onAddTimer);

	// ── Media player for alerts ───────────────────────────────
	m_audioOut = new QAudioOutput(this);
	m_audioOut->setVolume(1.0f);
	m_player   = new QMediaPlayer(this);
	m_player->setAudioOutput(m_audioOut);

	// ── Tick timer (100ms resolution) ────────────────────────
	m_tickTimer = new QTimer(this);
	m_tickTimer->setInterval(100);
	connect(m_tickTimer, &QTimer::timeout, this, &ConfidenceMonitorDock::onTick);
	m_tickTimer->start();

	// ── Restore saved timers ──────────────────────────────────
	loadSettings();

	// Add a default timer if nothing saved
	if (m_timers.isEmpty())
		onAddTimer();
}

ConfidenceMonitorDock::~ConfidenceMonitorDock()
{
	saveSettings();
}

// ─────────────────────────────────────────────────────────────

void ConfidenceMonitorDock::Register()
{
	const auto cb = []() {
		auto *mainWin = static_cast<QMainWindow *>(
			obs_frontend_get_main_window());
		auto *dock = new ConfidenceMonitorDock(mainWin);
		mainWin->addDockWidget(Qt::RightDockWidgetArea, dock);

		auto *action = static_cast<QAction *>(
			obs_frontend_add_dock(dock));
		if (action)
			action->setChecked(true);
	};

	obs_frontend_add_event_callback(
		[](obs_frontend_event event, void *data) {
			if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING)
				reinterpret_cast<void(*)()>(data)();
		},
		reinterpret_cast<void *>(+cb));
}

// ═══════════════════════════════════════════════════════════════
//  Tick — called every 100ms
// ═══════════════════════════════════════════════════════════════

void ConfidenceMonitorDock::onTick()
{
	qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

	for (int i = 0; i < m_timers.size(); ++i) {
		TimerData &d = m_timers[i];

		if (d.state != TimerState::Running) continue;

		qint64 elapsed    = nowMs - d.startWallMs;
		qint64 remaining  = d.remainingMs - elapsed;

		// ── Alert: half-time ──────────────────────────────────
		qint64 halfMs = (qint64)(d.durationSeconds / 2) * 1000LL;
		if (!d.alertHalfFired && remaining <= halfMs) {
			d.alertHalfFired = true;
			playSound(d);
		}

		// ── Alert: 5 minutes ─────────────────────────────────
		if (!d.alert5MinFired && remaining <= 300000LL) {
			d.alert5MinFired = true;
			playSound(d);
		}

		// ── Alert: 1 minute ──────────────────────────────────
		if (!d.alert1MinFired && remaining <= 60000LL) {
			d.alert1MinFired = true;
			playSound(d);
		}

		// ── Alert: zero / expired ─────────────────────────────
		if (!d.alertZeroFired && remaining <= 0) {
			d.alertZeroFired = true;
			playSound(d);
		}

		// ── Refresh the row widget ────────────────────────────
		if (i < m_rows.size())
			m_rows[i]->refresh(nowMs);
	}
}

// ═══════════════════════════════════════════════════════════════
//  Add / Delete
// ═══════════════════════════════════════════════════════════════

void ConfidenceMonitorDock::onAddTimer()
{
	TimerData d;
	d.id              = m_nextId++;
	d.label           = QString("Timer %1").arg(m_timers.size() + 1);
	d.durationSeconds = 25 * 60; // default 25 min
	d.remainingMs     = (qint64)d.durationSeconds * 1000LL;
	addTimerRow(d);
}

void ConfidenceMonitorDock::addTimerRow(TimerData data)
{
	m_timers.append(data);
	TimerData &ref = m_timers.last();

	auto *row = new TimerRowWidget(ref, m_container);

	// Remove the trailing stretch, add row, re-add stretch
	QLayoutItem *stretch = m_listLayout->takeAt(m_listLayout->count() - 1);
	m_listLayout->addWidget(row);
	m_listLayout->addItem(stretch);

	m_rows.append(row);

	connect(row, &TimerRowWidget::requestDelete,
	        this, &ConfidenceMonitorDock::onDeleteTimer);

	connect(row, &TimerRowWidget::requestBrowseSound,
	        this, &ConfidenceMonitorDock::onBrowseSound);

	connect(row, &TimerRowWidget::requestPlayAlert,
	        this, [this](int id) {
		for (auto &d : m_timers)
			if (d.id == id) { playSound(d); return; }
	});
}

void ConfidenceMonitorDock::onDeleteTimer(int id)
{
	for (int i = 0; i < m_timers.size(); ++i) {
		if (m_timers[i].id == id) {
			m_timers.removeAt(i);
			m_listLayout->removeWidget(m_rows[i]);
			m_rows[i]->deleteLater();
			m_rows.removeAt(i);
			break;
		}
	}
	saveSettings();
}

// ═══════════════════════════════════════════════════════════════
//  Sound
// ═══════════════════════════════════════════════════════════════

void ConfidenceMonitorDock::onBrowseSound(int id)
{
	QString path = QFileDialog::getOpenFileName(
		this,
		"Choisir un son d'alerte",
		QStandardPaths::writableLocation(QStandardPaths::HomeLocation),
		"Fichiers audio (*.wav *.mp3 *.ogg *.flac)"
	);
	if (path.isEmpty()) return;

	for (int i = 0; i < m_timers.size(); ++i) {
		if (m_timers[i].id == id) {
			m_timers[i].soundPath       = path;
			m_timers[i].useBuiltinSound = false;
			m_rows[i]->updateSoundLabel(QFileInfo(path).fileName());
			saveSettings();
			return;
		}
	}
}

void ConfidenceMonitorDock::playSound(const TimerData &d)
{
	m_player->stop();

	if (!d.useBuiltinSound && !d.soundPath.isEmpty()
	    && QFileInfo::exists(d.soundPath)) {
		// Custom .wav chosen by user
		m_player->setSource(QUrl::fromLocalFile(d.soundPath));
	} else {
		// Fallback: embedded resource compiled into the .dll
		m_player->setSource(QUrl("qrc:/sounds/alert.wav"));
	}

	m_player->play();
}

// ═══════════════════════════════════════════════════════════════
//  Save / Load
// ═══════════════════════════════════════════════════════════════

void ConfidenceMonitorDock::saveSettings()
{
	QSettings s("ConfidenceMonitor", "Timers");
	s.beginWriteArray("timers");
	for (int i = 0; i < m_timers.size(); ++i) {
		s.setArrayIndex(i);
		const TimerData &d = m_timers[i];
		s.setValue("id",       d.id);
		s.setValue("label",    d.label);
		s.setValue("source",   d.obsSourceName);
		s.setValue("duration", d.durationSeconds);
		s.setValue("sound",       d.soundPath);
		s.setValue("builtinSnd",  d.useBuiltinSound);
	}
	s.endArray();
	s.setValue("nextId", m_nextId);
}

void ConfidenceMonitorDock::loadSettings()
{
	QSettings s("ConfidenceMonitor", "Timers");
	m_nextId   = s.value("nextId", 1).toInt();
	int count  = s.beginReadArray("timers");
	for (int i = 0; i < count; ++i) {
		s.setArrayIndex(i);
		TimerData d;
		d.id              = s.value("id", m_nextId++).toInt();
		d.label           = s.value("label", "Timer").toString();
		d.obsSourceName   = s.value("source", "").toString();
		d.durationSeconds = s.value("duration", 1500).toInt();
		d.soundPath       = s.value("sound", "").toString();
		d.useBuiltinSound = s.value("builtinSnd", true).toBool();
		d.remainingMs     = (qint64)d.durationSeconds * 1000LL;
		addTimerRow(d);
	}
	s.endArray();
}
