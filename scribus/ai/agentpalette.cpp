/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#include "agentpalette.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QFile>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTimer>
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

	QVariantMap runSandboxThroughScripter(const QString& code, const QString& previewPath)
	{
		QVariantMap result{ {"ok", false}, {"output", QString()}, {"error", QString()}, {"preview", QString()} };
		QObject* scripter = qApp->property("scribusScripterCore").value<QObject*>();
		if (!scripter)
		{
			result["error"] = QObject::tr("The Python scripter plugin is not loaded.");
			return result;
		}
		QVariantMap scriptResult;
		const bool invoked = QMetaObject::invokeMethod(scripter, "runAgentSandbox", Qt::DirectConnection,
		                                               Q_RETURN_ARG(QVariantMap, scriptResult), Q_ARG(QString, code), Q_ARG(QString, previewPath));
		if (!invoked)
		{
			result["error"] = QObject::tr("The scripter plugin does not support scratch tests.");
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
	m_controller->setSandboxRunner(&runSandboxThroughScripter);

	// For automated tests only (UI automation of Qt widgets is unreliable): SCRIBUS_AGENT_MODEL picks the model,
	// SCRIBUS_AGENT_PROMPT_FILE names a UTF-8 file whose text is sent shortly after start-up, and
	// SCRIBUS_AGENT_AUTOAPPROVE=1 approves scripts without asking.
	const QString testModel = qEnvironmentVariable("SCRIBUS_AGENT_MODEL");
	if (!testModel.isEmpty())
	{
		m_modelEdit->setText(testModel);
		applySettings();
	}
	if (qEnvironmentVariable("SCRIBUS_AGENT_AUTOAPPROVE") == QStringLiteral("1"))
		m_controller->setAutoApprove(true);
	const QString testPromptFile = qEnvironmentVariable("SCRIBUS_AGENT_PROMPT_FILE");
	if (!testPromptFile.isEmpty())
	{
		QTimer::singleShot(qEnvironmentVariableIntValue("SCRIBUS_AGENT_PROMPT_DELAY") * 1000 + 12000, this, [this, testPromptFile]() {
			QFile file(testPromptFile);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				return;
			m_input->setPlainText(QString::fromUtf8(file.readAll()).trimmed());
			sendClicked();
		});
	}

	// For automated tests only: SCRIBUS_AGENT_ATTACH names image files to attach at start-up.
	const QString preAttached = qEnvironmentVariable("SCRIBUS_AGENT_ATTACH");
	if (!preAttached.isEmpty())
	{
		m_attachments = preAttached.split(QLatin1Char(';'), Qt::SkipEmptyParts);
		updateAttachments();
	}

	connect(m_controller, &AgentController::assistantChunk, this, &AgentPalette::onAssistantChunk);
	connect(m_controller, &AgentController::approvalRequested, this, &AgentPalette::onApprovalRequested);
	connect(m_controller, &AgentController::scriptFinished, this, &AgentPalette::onScriptFinished);
	connect(m_controller, &AgentController::scriptTested, this, &AgentPalette::onScriptTested);
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
	m_approvalPreview = new QLabel(m_approvalBar);
	m_approvalPreview->setAlignment(Qt::AlignCenter);
	m_approvalPreview->setVisible(false);
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
	approvalLayout->addWidget(m_approvalPreview);
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

	m_attachLabel = new QLabel(root);
	m_attachLabel->setWordWrap(true);
	m_attachLabel->setVisible(false);
	layout->addWidget(m_attachLabel);

	m_input = new QPlainTextEdit(root);
	m_input->setMaximumHeight(90);
	m_input->installEventFilter(this);
	layout->addWidget(m_input);

	auto* buttons = new QWidget(root);
	auto* buttonLayout = new QHBoxLayout(buttons);
	buttonLayout->setContentsMargins(0, 0, 0, 0);
	m_newChatButton = new QPushButton(buttons);
	m_attachButton = new QPushButton(buttons);
	m_sendButton = new QPushButton(buttons);
	buttonLayout->addWidget(m_newChatButton);
	buttonLayout->addWidget(m_attachButton);
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(m_sendButton);
	layout->addWidget(buttons);

	setWidget(root);

	connect(m_sendButton, &QPushButton::clicked, this, &AgentPalette::sendClicked);
	connect(m_newChatButton, &QPushButton::clicked, this, &AgentPalette::newChatClicked);
	connect(m_attachButton, &QPushButton::clicked, this, &AgentPalette::attachClicked);
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
	m_attachButton->setText(tr("Attach image..."));
	m_attachButton->setToolTip(tr("Attach a photo or scan, for example the client's printed notice"));
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
	QString text = m_input->toPlainText().trimmed();
	if (text.isEmpty() && m_attachments.isEmpty())
		return;
	if (text.isEmpty())
		text = tr("Read the attached image.");
	applySettings(); // the user may not have left the last settings field yet
	m_input->clear();
	QStringList shown;
	for (const QString& path : std::as_const(m_attachments))
		shown << QFileInfo(path).fileName();
	appendUserMessage(shown.isEmpty() ? text : text + QStringLiteral(" [") + tr("image") + QStringLiteral(": ") + shown.join(QStringLiteral(", ")) + QLatin1Char(']'));
	startAssistantMessage();
	const QStringList images = m_attachments;
	m_attachments.clear();
	updateAttachments();
	m_controller->sendPrompt(text, images);
}

void AgentPalette::attachClicked()
{
	const QStringList files = QFileDialog::getOpenFileNames(this, tr("Attach image"), QString(),
	                                                       tr("Images (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.webp);;All files (*)"));
	m_attachments << files;
	updateAttachments();
}

void AgentPalette::updateAttachments()
{
	QStringList names;
	for (const QString& path : std::as_const(m_attachments))
		names << QFileInfo(path).fileName();
	m_attachLabel->setText(tr("Image: %1").arg(names.join(QStringLiteral(", "))));
	m_attachLabel->setVisible(!names.isEmpty());
}

void AgentPalette::newChatClicked()
{
	m_testCount = 0;
	m_controller->newConversation();
	m_transcript->clear();
	m_attachments.clear();
	updateAttachments();
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
		? tr("The agent is ready to run the final script on your document:")
		: tr("The agent is ready to run the final script on your document: %1").arg(summary));
	const QPixmap preview(m_controller->lastPreviewPath());
	m_approvalPreview->setVisible(!preview.isNull());
	if (!preview.isNull())
	{
		m_approvalPreview->setPixmap(preview.scaled(320, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		m_approvalPreview->setToolTip(tr("Result of the last test on a scratch copy"));
	}
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

void AgentPalette::onScriptTested(const QString& summary, bool ok, const QString& resultText, const QString& previewPath)
{
	Q_UNUSED(previewPath);
	++m_testCount;
	const QString title = summary.isEmpty() ? tr("Test") : summary;
	QString shown = resultText;
	if (shown.size() > 700)
		shown = shown.left(700) + QStringLiteral("\n...");
	m_transcript->append(QStringLiteral("<p><i>%1 %2 #%3: %4</i></p><pre>%5</pre>")
		.arg(QString(QChar(ok ? 0x2714 : 0x2718)), tr("Scratch test")).arg(m_testCount).arg(title.toHtmlEscaped(), shown.toHtmlEscaped()));
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
