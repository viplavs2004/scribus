/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#include "agentpalette.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

#include "ai/agentcontroller.h"
#include "prefscontext.h"
#include "scribus.h"
#include "scribuscore.h"

namespace
{
	/**
	 * Runs a script through the scripter plugin. The plugin is a separate library, so it is
	 * found through the meta-object system: ScripterCore is a child of the main window and
	 * has the slot runAgentScript(). Runs on the GUI thread, as the scripting API requires.
	 */
	QVariantMap runScriptThroughScripter(const QString& code)
	{
		QVariantMap result{ {"ok", false}, {"output", QString()}, {"error", QString()} };

		// The scripter plugin publishes itself as an application property (see ScripterCore).
		QObject* scripter = qApp->property("scribusScripterCore").value<QObject*>();
		if (!scripter)
		{
			result["error"] = QObject::tr("The Python scripter plugin is not loaded.");
			return result;
		}

		QVariantMap scriptResult;
		const bool invoked = QMetaObject::invokeMethod(scripter, "runAgentScript", Qt::DirectConnection,
		                                               Q_RETURN_ARG(QVariantMap, scriptResult), Q_ARG(QString, code));
		if (!invoked)
		{
			result["error"] = QObject::tr("The scripter plugin does not support agent scripts.");
			return result;
		}
		return scriptResult;
	}
}

AgentPalette::AgentPalette(QWidget* parent)
	: DockPanelBase("AgentPalette", parent),
	  m_controller(new AgentController(this))
{
	buildUi();
	loadSettings();
	languageChange();

	m_controller->setScriptRunner(&runScriptThroughScripter);

	connect(m_controller, &AgentController::assistantChunk, this, &AgentPalette::onAssistantChunk);
	connect(m_controller, &AgentController::approvalRequested, this, &AgentPalette::onApprovalRequested);
	connect(m_controller, &AgentController::scriptFinished, this, &AgentPalette::onScriptFinished);
	connect(m_controller, &AgentController::turnFinished, this, &AgentPalette::onTurnFinished);
	connect(m_controller, &AgentController::turnFailed, this, &AgentPalette::onTurnFailed);
	connect(m_controller, &AgentController::busyChanged, this, &AgentPalette::onBusyChanged);
}

void AgentPalette::buildUi()
{
	auto* root = new QWidget(this);
	auto* layout = new QVBoxLayout(root);
	layout->setContentsMargins(4, 4, 4, 4);

	m_transcript = new QTextBrowser(root);
	m_transcript->setOpenExternalLinks(false);
	layout->addWidget(m_transcript, 1);

	// Shown while the agent waits for permission to run a script.
	m_approvalBar = new QWidget(root);
	auto* approvalLayout = new QVBoxLayout(m_approvalBar);
	approvalLayout->setContentsMargins(0, 0, 0, 0);
	m_approvalSummary = new QLabel(m_approvalBar);
	m_approvalSummary->setWordWrap(true);
	m_approvalCode = new QPlainTextEdit(m_approvalBar);
	m_approvalCode->setReadOnly(true);
	m_approvalCode->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_approvalCode->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_approvalCode->setMaximumHeight(130);
	m_approvalAlways = new QCheckBox(m_approvalBar);
	auto* approvalButtons = new QWidget(m_approvalBar);
	auto* approvalButtonLayout = new QHBoxLayout(approvalButtons);
	approvalButtonLayout->setContentsMargins(0, 0, 0, 0);
	m_rejectButton = new QPushButton(approvalButtons);
	m_approveButton = new QPushButton(approvalButtons);
	approvalButtonLayout->addStretch(1);
	approvalButtonLayout->addWidget(m_rejectButton);
	approvalButtonLayout->addWidget(m_approveButton);
	approvalLayout->addWidget(m_approvalSummary);
	approvalLayout->addWidget(m_approvalCode);
	approvalLayout->addWidget(m_approvalAlways);
	approvalLayout->addWidget(approvalButtons);
	m_approvalBar->setVisible(false);
	layout->addWidget(m_approvalBar);

	m_settingsToggle = new QToolButton(root);
	m_settingsToggle->setCheckable(true);
	m_settingsToggle->setToolButtonStyle(Qt::ToolButtonTextOnly);
	m_settingsToggle->setAutoRaise(true);
	layout->addWidget(m_settingsToggle);

	m_settingsPanel = new QWidget(root);
	auto* form = new QFormLayout(m_settingsPanel);
	form->setContentsMargins(0, 0, 0, 0);
	m_providerCombo = new QComboBox(m_settingsPanel);
	m_providerCombo->addItem("Ollama (local)", AgentController::providerId(AgentController::Provider::Ollama));
	m_providerCombo->addItem("OpenAI-compatible", AgentController::providerId(AgentController::Provider::OpenAICompatible));
	m_providerCombo->addItem("Claude", AgentController::providerId(AgentController::Provider::Claude));
	m_providerCombo->addItem("Gemini", AgentController::providerId(AgentController::Provider::Gemini));
	m_providerCombo->addItem("llama.cpp (local)", AgentController::providerId(AgentController::Provider::LlamaCpp));
	m_urlEdit = new QLineEdit(m_settingsPanel);
	m_modelEdit = new QLineEdit(m_settingsPanel);
	m_keyEdit = new QLineEdit(m_settingsPanel);
	m_keyEdit->setEchoMode(QLineEdit::Password);
	form->addRow(tr("Provider"), m_providerCombo);
	form->addRow(tr("URL"), m_urlEdit);
	form->addRow(tr("Model"), m_modelEdit);
	form->addRow(tr("API key"), m_keyEdit);
	m_settingsPanel->setVisible(false);
	layout->addWidget(m_settingsPanel);

	m_input = new QPlainTextEdit(root);
	m_input->setMaximumHeight(90);
	m_input->installEventFilter(this);
	layout->addWidget(m_input);

	auto* buttons = new QWidget(root);
	auto* buttonLayout = new QHBoxLayout(buttons);
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	m_newChatButton = new QPushButton(buttons);
	m_sendButton = new QPushButton(buttons);
	buttonLayout->addWidget(m_newChatButton);
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(m_sendButton);
	layout->addWidget(buttons);

	setWidget(root);

	connect(m_sendButton, &QPushButton::clicked, this, &AgentPalette::sendClicked);
	connect(m_newChatButton, &QPushButton::clicked, this, &AgentPalette::newChatClicked);
	connect(m_approveButton, &QPushButton::clicked, this, &AgentPalette::approveClicked);
	connect(m_rejectButton, &QPushButton::clicked, this, &AgentPalette::rejectClicked);
	connect(m_settingsToggle, &QToolButton::toggled, this, &AgentPalette::settingsToggled);
	connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AgentPalette::providerChanged);
	connect(m_urlEdit, &QLineEdit::editingFinished, this, &AgentPalette::applySettings);
	connect(m_modelEdit, &QLineEdit::editingFinished, this, &AgentPalette::applySettings);
	connect(m_keyEdit, &QLineEdit::editingFinished, this, &AgentPalette::applySettings);
}

void AgentPalette::languageChange()
{
	setWindowTitle(tr("AI Agent"));
	m_input->setPlaceholderText(tr("Describe what you want to build or change..."));
	m_newChatButton->setText(tr("New chat"));
	m_sendButton->setText(m_controller && m_controller->isBusy() ? tr("Stop") : tr("Send"));
	m_settingsToggle->setText(m_settingsToggle->isChecked() ? tr("Settings \xE2\x96\xB4") : tr("Settings \xE2\x96\xBE"));
	m_approveButton->setText(tr("Run script"));
	m_rejectButton->setText(tr("Reject"));
	m_approvalAlways->setText(tr("Don't ask again in this chat"));
}

bool AgentPalette::eventFilter(QObject* watched, QEvent* event)
{
	// Enter sends, Shift+Enter inserts a new line.
	if (watched == m_input && event->type() == QEvent::KeyPress)
	{
		auto* key = static_cast<QKeyEvent*>(event);
		if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) && !(key->modifiers() & Qt::ShiftModifier))
		{
			sendClicked();
			return true;
		}
	}
	return DockPanelBase::eventFilter(watched, event);
}

void AgentPalette::loadSettings()
{
	AgentController::Settings s;
	if (m_palettePrefs && m_palettePrefs->contains("provider"))
	{
		s.provider = AgentController::providerFromId(m_palettePrefs->get("provider"));
		const AgentController::Settings defaults = AgentController::defaultSettings(s.provider);
		s.baseUrl = m_palettePrefs->get("url", defaults.baseUrl);
		s.model = m_palettePrefs->get("model", defaults.model);
	}
	else
	{
		s = AgentController::defaultSettings(AgentController::Provider::Ollama);
	}
	// The API key is deliberately not stored in the preferences file (plain text).
	// It has to be entered again in each session until secure storage is added.

	const int index = m_providerCombo->findData(AgentController::providerId(s.provider));
	{
		const QSignalBlocker blocker(m_providerCombo);
		m_providerCombo->setCurrentIndex(index < 0 ? 0 : index);
	}
	m_urlEdit->setText(s.baseUrl);
	m_modelEdit->setText(s.model);
	m_controller->setSettings(s);
}

void AgentPalette::saveSettings()
{
	if (!m_palettePrefs)
		return;
	const AgentController::Settings& s = m_controller->settings();
	m_palettePrefs->set("provider", AgentController::providerId(s.provider));
	m_palettePrefs->set("url", s.baseUrl);
	m_palettePrefs->set("model", s.model);
}

void AgentPalette::providerChanged(int index)
{
	const auto provider = AgentController::providerFromId(m_providerCombo->itemData(index).toString());
	const AgentController::Settings defaults = AgentController::defaultSettings(provider);
	m_urlEdit->setText(defaults.baseUrl);
	m_modelEdit->setText(defaults.model);
	applySettings();
}

void AgentPalette::applySettings()
{
	AgentController::Settings s;
	s.provider = AgentController::providerFromId(m_providerCombo->currentData().toString());
	s.baseUrl = m_urlEdit->text().trimmed();
	s.model = m_modelEdit->text().trimmed();
	s.apiKey = m_keyEdit->text().trimmed();

	const AgentController::Settings& old = m_controller->settings();
	if (s.provider == old.provider && s.baseUrl == old.baseUrl && s.model == old.model && s.apiKey == old.apiKey)
		return;
	m_controller->setSettings(s);
	saveSettings();
}

void AgentPalette::settingsToggled(bool shown)
{
	m_settingsPanel->setVisible(shown);
	languageChange();
}

void AgentPalette::sendClicked()
{
	if (m_controller->isBusy())
	{
		m_controller->cancel();
		return;
	}
	const QString text = m_input->toPlainText().trimmed();
	if (text.isEmpty())
		return;
	applySettings(); // the user may not have left the last settings field yet
	m_input->clear();
	appendUserMessage(text);
	startAssistantMessage();
	m_controller->sendPrompt(text);
}

void AgentPalette::newChatClicked()
{
	m_controller->newConversation();
	m_transcript->clear();
	m_assistantOpen = false;
	showApprovalBar(false);
	m_approvalAlways->setChecked(false);
	m_controller->setAutoApprove(false);
}

void AgentPalette::appendUserMessage(const QString& text)
{
	m_transcript->append(QStringLiteral("<p><b>%1</b><br>%2</p>")
		.arg(tr("You"), text.toHtmlEscaped().replace('\n', "<br>")));
}

void AgentPalette::startAssistantMessage()
{
	m_assistantOpen = true;
	m_transcript->append(QStringLiteral("<p><b>%1</b><br></p>").arg(tr("Agent")));
}

void AgentPalette::onAssistantChunk(const QString& chunk)
{
	if (!m_assistantOpen)
		return;
	QTextCursor cursor(m_transcript->document());
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(chunk);
	m_transcript->ensureCursorVisible();
}

void AgentPalette::onApprovalRequested(const QString& summary, const QString& code)
{
	m_approvalSummary->setText(summary.isEmpty()
		? tr("The agent wants to run a script in Scribus:")
		: tr("The agent wants to run a script: %1").arg(summary));
	m_approvalCode->setPlainText(code);
	showApprovalBar(true);
}

void AgentPalette::approveClicked()
{
	if (m_approvalAlways->isChecked())
		m_controller->setAutoApprove(true);
	showApprovalBar(false);
	m_controller->resolveApproval(true);
}

void AgentPalette::rejectClicked()
{
	showApprovalBar(false);
	m_controller->resolveApproval(false);
}

void AgentPalette::showApprovalBar(bool shown)
{
	m_approvalBar->setVisible(shown);
	if (!shown)
		m_approvalCode->clear();
}

void AgentPalette::onScriptFinished(const QString& summary, bool ok, const QString& resultText)
{
	const QString title = summary.isEmpty() ? tr("Script") : summary;
	m_transcript->append(QStringLiteral("<p><i>%1 %2</i></p><pre>%3</pre>")
		.arg(QString(QChar(ok ? 0x2714 : 0x2718)), title.toHtmlEscaped(), resultText.toHtmlEscaped()));
	// What the model says after the script gets its own block.
	startAssistantMessage();
}

void AgentPalette::onTurnFinished()
{
	m_assistantOpen = false;
}

void AgentPalette::onTurnFailed(const QString& error)
{
	m_assistantOpen = false;
	showApprovalBar(false);
	appendNotice(error);
}

void AgentPalette::appendNotice(const QString& text)
{
	m_transcript->append(QStringLiteral("<p><i>%1</i></p>").arg(text.toHtmlEscaped()));
}

void AgentPalette::onBusyChanged(bool busy)
{
	m_sendButton->setText(busy ? tr("Stop") : tr("Send"));
	m_input->setReadOnly(busy);
}
