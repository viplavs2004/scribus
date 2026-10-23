/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#ifndef AGENTPALETTE_H
#define AGENTPALETTE_H

#include "scribusapi.h"
#include "ui/docks/dock_panelbase.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;
class QToolButton;
class QWidget;

class AgentController;

/**
 * @brief Sidebar where the user types prompts for the AI agent.
 *
 * The palette handles the user interface: chat, settings, and the approve/reject bar that
 * is shown before the agent runs a script. All communication with the model is done by
 * AgentController; the palette supplies the function that runs Python through the scripter.
 */
class SCRIBUS_API AgentPalette : public DockPanelBase
{
	Q_OBJECT

public:
	explicit AgentPalette(QWidget* parent = nullptr);
	~AgentPalette() override = default;

	AgentController* controller() const { return m_controller; }

	void languageChange();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
	void sendClicked();
	void newChatClicked();
	void attachClicked();
	void settingsToggled(bool shown);
	void providerChanged(int index);
	void applySettings();
	void onAssistantChunk(const QString& chunk);
	void onApprovalRequested(const QString& summary, const QString& code);
	void onScriptFinished(const QString& summary, bool ok, const QString& resultText);
	void approveClicked();
	void rejectClicked();
	void onTurnFinished();
	void onTurnFailed(const QString& error);
	void onBusyChanged(bool busy);

private:
	void buildUi();
	void loadSettings();
	void saveSettings();
	void appendUserMessage(const QString& text);
	void startAssistantMessage();
	void appendNotice(const QString& text);
	void showApprovalBar(bool shown);
	void updateAttachments();

	AgentController* m_controller {nullptr};

	QTextBrowser* m_transcript {nullptr};
	QPlainTextEdit* m_input {nullptr};
	QPushButton* m_sendButton {nullptr};
	QPushButton* m_newChatButton {nullptr};
	QPushButton* m_attachButton {nullptr};
	QLabel* m_attachLabel {nullptr};
	QStringList m_attachments;
	QToolButton* m_settingsToggle {nullptr};
	QWidget* m_settingsPanel {nullptr};
	QComboBox* m_providerCombo {nullptr};
	QLineEdit* m_urlEdit {nullptr};
	QLineEdit* m_modelEdit {nullptr};
	QLineEdit* m_keyEdit {nullptr};

	QWidget* m_approvalBar {nullptr};
	QLabel* m_approvalSummary {nullptr};
	QPlainTextEdit* m_approvalCode {nullptr};
	QCheckBox* m_approvalAlways {nullptr};
	QPushButton* m_approveButton {nullptr};
	QPushButton* m_rejectButton {nullptr};

	bool m_assistantOpen {false};
	bool m_newBlockBeforeText {false};
};

#endif // AGENTPALETTE_H
