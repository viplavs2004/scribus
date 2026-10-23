/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#ifndef AGENTCONTROLLER_H
#define AGENTCONTROLLER_H

#include <functional>
#include <memory>

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "scribusapi.h"

/**
 * @brief Talks to an LLM on behalf of the agent sidebar and lets it run Python in Scribus.
 *
 * The controller wraps an LLMQore client and the conversation history. It lives on the
 * GUI thread: network I/O is asynchronous, and the model's tool calls run on this thread
 * as well, which is what the Scribus scripting API requires.
 *
 * The model has one tool, run_python. Before it runs, the user has to approve it
 * (approvalRequested / resolveApproval) unless auto-approval is switched on.
 *
 * This header deliberately does not include any LLMQore header (LLMQore needs C++20,
 * the rest of Scribus builds as C++17); everything LLMQore-specific is behind Private.
 */
class SCRIBUS_API AgentController : public QObject
{
	Q_OBJECT

public:
	enum class Provider
	{
		Ollama,
		OpenAICompatible,
		Claude,
		Gemini,
		LlamaCpp
	};

	struct Settings
	{
		Provider provider { Provider::Ollama };
		QString baseUrl;
		QString apiKey;
		QString model;
	};

	/**
	 * Runs a Python script inside Scribus. Called on the GUI thread.
	 * The result has the keys ok (bool), output (QString) and error (QString).
	 */
	using ScriptRunner = std::function<QVariantMap(const QString& code)>;

	explicit AgentController(QObject* parent = nullptr);
	~AgentController() override;

	/** @brief Default URL and model for a provider; the API key is left empty. */
	static Settings defaultSettings(Provider provider);
	static QString providerId(Provider provider);
	static Provider providerFromId(const QString& id);

	const Settings& settings() const;
	void setSettings(const Settings& settings);

	void setScriptRunner(ScriptRunner runner);

	/** @brief When true, scripts run without asking the user first. */
	void setAutoApprove(bool autoApprove);
	bool autoApprove() const;

	bool isBusy() const;

public slots:
	/** @brief Adds the prompt (and any image files, such as a photo or scan) to the conversation and asks the model. */
	void sendPrompt(const QString& text, const QStringList& imagePaths = {});
	/** @brief Cancels the running request, if any. */
	void cancel();
	/** @brief Forgets the conversation history. */
	void newConversation();
	/** @brief Answers the pending approvalRequested(). */
	void resolveApproval(bool approved);

signals:
	/** @brief A piece of the assistant's answer arrived. */
	void assistantChunk(const QString& chunk);
	/** @brief The model wants to run a script and waits for resolveApproval(). */
	void approvalRequested(const QString& summary, const QString& code);
	/** @brief A script the model asked for has run (successfully or not). */
	void scriptFinished(const QString& summary, bool ok, const QString& resultText);
	/** @brief The turn ended successfully. */
	void turnFinished();
	/** @brief The turn ended with an error (network, authentication, bad model name...). */
	void turnFailed(const QString& error);
	void busyChanged(bool busy);

private:
	struct Private;
	std::unique_ptr<Private> d;

	void rebuildClient();
	void setBusy(bool busy);
};

#endif // AGENTCONTROLLER_H
