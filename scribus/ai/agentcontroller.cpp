/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#include "agentcontroller.h"

#include <QDateTime>
#include <QFile>
#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QPromise>
#include <QTextStream>

#include <LLMQore/BaseTool.hpp>
#include <LLMQore/Clients>
#include <LLMQore/Tools>

namespace
{
	const char* kSystemPrompt =
		"You are an assistant built into Scribus 1.7, a desktop publishing application, "
		"working directly on the document the user has open.\n"
		"Always reply in the language the user writes in (English, Hindi or Hinglish) and keep replies short.\n"
		"\n"
		"To look at or change anything, call the run_python tool with a Python script. "
		"The script runs inside Scribus and can use the scribus module (from scribus import *).\n"
		"Rules for scripts:\n"
		"- Units are points unless you call setUnit(). An A4 page is 595 x 842 pt.\n"
		"- There may be no document open: check haveDoc() and create one with newDocument(...) if needed.\n"
		"- Each run is one undo step for the user, so do one complete, logical change per call.\n"
		"- print() what you find or create. If a script fails, read the traceback, fix the script and try again.\n"
		"- Do not call closeDoc() or saveDoc() unless the user asks.\n"
		"- For Hindi (Devanagari) text, set a font that has Devanagari glyphs on the text frame with setFont(). "
		"Font names in getFontNames() include the style, for example 'Nirmala UI Regular' or 'Nirmala UI Bold', so match by substring, never by exact name: "
		"[f for f in getFontNames() if 'Nirmala' in f or 'Mangal' in f or 'Devanagari' in f]. "
		"Never fall back to a font without Devanagari such as Arial; the text would show as empty boxes. "
		"If a script result contains WARNING about missing glyphs, fix the font and run again.\n"
		"- Kruti Dev is a legacy Hindi font. Use it only when the user asks for it. It cannot show Unicode text, so never call setFont() with it yourself. "
		"Instead call use_kruti_dev(frame_name): it converts that frame's Hindi text and sets the font (English words keep their font). "
		"use_unicode_font(frame_name) turns it back into Unicode text with a Unicode font. "
		"Both functions are already available in every script, and both raise an error you should report if a character cannot be converted.\n"
		"\n"
		"Use only these scribus functions. Do not invent others. Every function that takes a frame name works on the selected frame if the name is left out.\n"
		"- haveDoc() -> bool\n"
		"- newDocument(size, margins, orientation, firstPageNumber, unit, pagesType, firstPageOrder, numPages) -> bool; margins is (left, right, top, bottom)\n"
		"- newPage(-1) adds a page at the end; gotoPage(n) selects page n (1-based); pageDimension() -> (width, height)\n"
		"- createText(x, y, width, height) -> frame name; createRect(x, y, width, height) -> name; createImage(x, y, width, height) -> name; createLine(x1, y1, x2, y2) -> name\n"
		"- setText(\"text\", name) (the text comes first, then the frame name); getText(name)\n"
		"- setFont(\"font\", name); setFontSize(points, name); setTextAlignment(ALIGN_LEFT / ALIGN_CENTERED / ALIGN_RIGHT / ALIGN_BLOCK, name)\n"
		"- setLineSpacingMode(1, name) turns on automatic line spacing. Call it after setFontSize on any frame whose text can wrap, or lines of large text overlap\n"
		"- setFillColor(\"color\", name); setLineColor(\"color\", name); defineColor(\"name\", c, m, y, k) with values 0-255; getColorNames()\n"
		"- loadImage(\"file path\", name); setScaleImageToFrame(True, True, name)\n"
		"- getAllObjects() -> list of names; getPosition(name) -> (x, y); getSize(name) -> (w, h); moveObjectAbs(x, y, name); sizeObject(w, h, name); deleteObject(name)\n"
		"- getFontNames() -> list of fonts; use these exact names with setFont()\n"
		"Example, a new A4 page with a centered heading:\n"
		"from scribus import *\n"
		"if not haveDoc():\n"
		"    newDocument(PAPER_A4, (36, 36, 36, 36), PORTRAIT, 1, UNIT_POINTS, NOFACINGPAGES, FIRSTPAGERIGHT, 1)\n"
		"t = createText(36, 36, 523, 60)\n"
		"setText(\"Welcome to our sale\", t)\n"
		"setFontSize(32, t)\n"
		"setLineSpacingMode(1, t)\n"
		"setTextAlignment(ALIGN_CENTERED, t)\n"
		"print(t)\n"
		"\n"
		"Newspaper notice ads (these helper functions are already available in every script):\n"
		"- list_notice_templates() lists the templates. Right now: greater_noida_notice.\n"
		"- greater_noida_mutation_body(flat, floor, tower, area, project, plot, sector, owner, address, registration_date, applicant) "
		"returns the standard Greater Noida Authority mutation notice text (Hindi and English mixed). Fill it only with values you read from the "
		"client's document; never invent a value, and ask the user when a value is missing or unreadable.\n"
		"- create_notice_ad(template, body, width_cm=None, height_cm=None) creates a new document with the finished ad (border, logo, heading, "
		"justified body, signature) and fits the text. Pass the body in Unicode, Hindi and English mixed as it is; the function converts the Hindi "
		"to Kruti Dev and sets English in Times New Roman. It returns a report: print it, and tell the user the body size, how much the letters were "
		"condensed, and every entry of report['warnings']. Do not set font sizes or fonts yourself for these ads.\n"
		"- export_ad_pdf(file_name, standard='x1a') writes the PDF. standard is 'x1a', 'x3' or 'x4' for PDF/X (preferred by the press) or 'plain'. "
		"A bare file name goes to the user's output folder. Use PDF/X unless the user asks for a plain PDF.\n"
		"If the user wants an ad for which there is no template, say so instead of improvising a layout.\n"
		"After a script succeeds, say in one or two sentences what you did.";


	/** Appends a line to the file named by SCRIBUS_AGENT_LOG, if set. For diagnosing agent problems. */
	void agentLog(const QString& line)
	{
		static const QString path = qEnvironmentVariable("SCRIBUS_AGENT_LOG");
		if (path.isEmpty())
			return;
		QFile file(path);
		if (!file.open(QIODevice::Append | QIODevice::Text))
			return;
		QTextStream out(&file);
		out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " " << line << Qt::endl;
	}

	// Longest tool result sent back to the model; keeps a runaway print() from filling the context.
	constexpr int kMaxToolResultChars = 8000;

	QString clipped(const QString& text)
	{
		if (text.size() <= kMaxToolResultChars)
			return text;
		return text.left(kMaxToolResultChars) + QStringLiteral("\n... (output truncated)");
	}
}

namespace
{
	/**
	 * The one tool the model has: run a Python script inside Scribus.
	 * It is Mutating (the default), so LLMQore asks the execution gate before running it.
	 * executeAsync() is called on the client's thread, which is the GUI thread.
	 */
	class RunPythonTool : public LLMQore::BaseTool
	{
	public:
		using Runner = std::function<QVariantMap(const QString&)>;
		using Reporter = std::function<void(const QString& summary, bool ok, const QString& text)>;

		RunPythonTool(Runner runner, Reporter reporter, QObject* parent)
			: LLMQore::BaseTool(parent),
			  m_runner(std::move(runner)),
			  m_reporter(std::move(reporter))
		{
		}

		QString id() const override { return QStringLiteral("run_python"); }
		QString displayName() const override { return QStringLiteral("Run Python in Scribus"); }
		QString description() const override
		{
			return QStringLiteral(
				"Runs a Python script inside Scribus with the scribus scripting module available. "
				"Use it to inspect the document and to create or change pages, frames, text, images and styles. "
				"Returns everything the script printed, or the Python traceback if it failed.");
		}
		QJsonObject parametersSchema() const override
		{
			return QJsonObject{
				{"type", "object"},
				{"properties", QJsonObject{
					{"code", QJsonObject{
						{"type", "string"},
						{"description", "The Python source code to run."}}},
					{"summary", QJsonObject{
						{"type", "string"},
						{"description", "Optional. One short sentence, in the user's language, saying what the script does. Shown to the user when asking for approval."}}}}},
				{"required", QJsonArray{"code"}}};
		}

		QFuture<LLMQore::ToolResult> executeAsync(const QJsonObject& input) override
		{
			const QString code = input.value("code").toString();
			const QString summary = input.value("summary").toString();
			if (code.trimmed().isEmpty())
				return QtFuture::makeReadyValueFuture(LLMQore::ToolResult::error(QStringLiteral("The code parameter is empty.")));

			const QVariantMap result = m_runner(code);
			const bool ok = result.value("ok").toBool();
			const QString output = result.value("output").toString();
			const QString error = result.value("error").toString();

			QString text;
			if (ok)
			{
				text = output.isEmpty() ? QStringLiteral("OK (the script printed nothing)") : QStringLiteral("OK\n") + output;
			}
			else
			{
				text = QStringLiteral("ERROR\n") + error;
				if (!output.isEmpty())
					text += QStringLiteral("\nOutput before the error:\n") + output;
			}
			text = clipped(text);
			m_reporter(summary, ok, text);
			return QtFuture::makeReadyValueFuture(ok ? LLMQore::ToolResult::text(text) : LLMQore::ToolResult::error(text));
		}

	private:
		Runner m_runner;
		Reporter m_reporter;
	};
}

struct AgentController::Private
{
	Settings settings;
	QPointer<LLMQore::BaseClient> client;
	LLMQore::Conversation conversation;
	LLMQore::RequestID requestId;
	bool busy { false };

	bool turnHadOutput { false }; // text or a tool call arrived during the current turn

	ScriptRunner scriptRunner;
	bool autoApprove { false };
	std::shared_ptr<QPromise<bool>> approval; // pending approval, answered by resolveApproval()
};

AgentController::AgentController(QObject* parent)
	: QObject(parent),
	  d(std::make_unique<Private>())
{
	d->settings = defaultSettings(Provider::Ollama);
	d->conversation.setSystem(QString::fromUtf8(kSystemPrompt));
	rebuildClient();
}

AgentController::~AgentController() = default;

AgentController::Settings AgentController::defaultSettings(Provider provider)
{
	Settings s;
	s.provider = provider;
	switch (provider)
	{
	case Provider::Ollama:
		s.baseUrl = QStringLiteral("http://localhost:11434");
		s.model = QStringLiteral("qwen2.5:7b-instruct");
		break;
	case Provider::OpenAICompatible:
		s.baseUrl = QStringLiteral("https://api.openai.com");
		s.model = QStringLiteral("gpt-4o-mini");
		break;
	case Provider::Claude:
		s.baseUrl = QStringLiteral("https://api.anthropic.com");
		s.model = QStringLiteral("claude-sonnet-4-5");
		break;
	case Provider::Gemini:
		s.baseUrl = QStringLiteral("https://generativelanguage.googleapis.com");
		s.model = QStringLiteral("gemini-2.5-flash");
		break;
	case Provider::LlamaCpp:
		s.baseUrl = QStringLiteral("http://localhost:8080");
		break;
	}
	return s;
}

QString AgentController::providerId(Provider provider)
{
	switch (provider)
	{
	case Provider::Ollama:           return QStringLiteral("ollama");
	case Provider::OpenAICompatible: return QStringLiteral("openai");
	case Provider::Claude:           return QStringLiteral("claude");
	case Provider::Gemini:           return QStringLiteral("gemini");
	case Provider::LlamaCpp:         return QStringLiteral("llamacpp");
	}
	return QStringLiteral("ollama");
}

AgentController::Provider AgentController::providerFromId(const QString& id)
{
	for (Provider p : { Provider::Ollama, Provider::OpenAICompatible, Provider::Claude, Provider::Gemini, Provider::LlamaCpp })
	{
		if (providerId(p) == id)
			return p;
	}
	return Provider::Ollama;
}

const AgentController::Settings& AgentController::settings() const
{
	return d->settings;
}

void AgentController::setSettings(const Settings& settings)
{
	d->settings = settings;
	rebuildClient();
}

void AgentController::setScriptRunner(ScriptRunner runner)
{
	d->scriptRunner = std::move(runner);
}

void AgentController::setAutoApprove(bool autoApprove)
{
	d->autoApprove = autoApprove;
}

bool AgentController::autoApprove() const
{
	return d->autoApprove;
}

bool AgentController::isBusy() const
{
	return d->busy;
}

void AgentController::rebuildClient()
{
	if (d->client)
	{
		if (d->busy)
			d->client->cancelRequest(d->requestId);
		d->requestId.clear();
		d->client->disconnect(this);
		d->client->deleteLater();
	}
	resolveApproval(false);
	setBusy(false);

	const Settings& s = d->settings;
	LLMQore::BaseClient* client = nullptr;
	switch (s.provider)
	{
	case Provider::Ollama:
		client = new LLMQore::OllamaClient(s.baseUrl, s.apiKey, s.model, this);
		break;
	case Provider::OpenAICompatible:
		client = new LLMQore::OpenAIClient(s.baseUrl, s.apiKey, s.model, this);
		break;
	case Provider::Claude:
		client = new LLMQore::ClaudeClient(s.baseUrl, s.apiKey, s.model, this);
		break;
	case Provider::Gemini:
		client = new LLMQore::GoogleAIClient(s.baseUrl, s.apiKey, s.model, this);
		break;
	case Provider::LlamaCpp:
		client = new LLMQore::LlamaCppClient(s.baseUrl, s.apiKey, s.model, this);
		break;
	}
	d->client = client;

	// The model's only tool. The runner is looked up at call time so it can be set after the client exists.
	client->tools()->addTool(new RunPythonTool(
		[this](const QString& code) -> QVariantMap {
			if (!d->scriptRunner)
			{
				return QVariantMap{
					{"ok", false},
					{"output", QString()},
					{"error", QStringLiteral("Scripting is not available in this build.")}};
			}
			return d->scriptRunner(code);
		},
		[this](const QString& summary, bool ok, const QString& text) {
			agentLog(QStringLiteral("script %1: %2").arg(ok ? QStringLiteral("OK") : QStringLiteral("ERROR"), text));
			emit scriptFinished(summary, ok, text);
		},
		client));

	// Ask the user before any Mutating tool runs. The future is settled by resolveApproval().
	client->tools()->setExecutionGate(
		[this](const QString&, const QString&, const QString&, const QJsonObject& input) -> QFuture<bool> {
			d->turnHadOutput = true;
			agentLog(QStringLiteral("gate: tool call, autoApprove=%1").arg(d->autoApprove));
			if (d->autoApprove)
				return QtFuture::makeReadyValueFuture(true);
			// A second request while one is pending would be a model bug; decline the older one.
			resolveApproval(false);
			d->approval = std::make_shared<QPromise<bool>>();
			d->approval->start();
			QFuture<bool> future = d->approval->future();
			agentLog(QStringLiteral("gate: waiting for approval. code:\n%1").arg(input.value("code").toString()));
			emit approvalRequested(input.value("summary").toString(), input.value("code").toString());
			return future;
		});

	connect(client, &LLMQore::BaseClient::chunkReceived, this,
		[this](const LLMQore::RequestID& id, const QString& chunk) {
			agentLog(QStringLiteral("chunk: id=%1 current=%2 len=%3").arg(id, d->requestId).arg(chunk.size()));
			if (id != d->requestId)
				return;
			if (!chunk.isEmpty())
				d->turnHadOutput = true;
			emit assistantChunk(chunk);
		});
	connect(client, &LLMQore::BaseClient::requestFinalized, this,
		[this](const LLMQore::RequestID& id, const LLMQore::CompletionInfo& info) {
			if (id == d->requestId)
				d->conversation = info.conversation;
		});
	connect(client, &LLMQore::BaseClient::requestCompleted, this,
		[this](const LLMQore::RequestID& id, const QString&) {
			agentLog(QStringLiteral("completed: id=%1 current=%2").arg(id, d->requestId));
			if (id != d->requestId)
				return;
			setBusy(false);
			if (!d->turnHadOutput)
			{
				// Some models occasionally answer with nothing at all. Say so instead of leaving a blank reply.
				emit turnFailed(tr("The model returned an empty reply. Try again, or choose a different model in Settings."));
				return;
			}
			emit turnFinished();
		});
	connect(client, &LLMQore::BaseClient::requestFailed, this,
		[this](const LLMQore::RequestID& id, const QString& error) {
			agentLog(QStringLiteral("failed: id=%1 current=%2 error=%3").arg(id, d->requestId, error));
			if (id != d->requestId)
				return;
			setBusy(false);
			emit turnFailed(error);
		});
}

void AgentController::sendPrompt(const QString& text)
{
	if (d->busy || !d->client || text.trimmed().isEmpty())
		return;
	d->conversation.addUser(text);
	d->turnHadOutput = false;
	setBusy(true);
	d->requestId = d->client->ask(d->conversation);
	agentLog(QStringLiteral("ask: id=%1 model=%2").arg(d->requestId, d->settings.model));
}

void AgentController::cancel()
{
	if (!d->busy || !d->client)
		return;
	d->client->cancelRequest(d->requestId);
	d->requestId.clear(); // ignore anything the client still emits for the cancelled request
	resolveApproval(false); // do not leave a tool waiting for an answer nobody will give
	setBusy(false);
	emit turnFailed(tr("Cancelled"));
}

void AgentController::newConversation()
{
	cancel();
	d->conversation.clear();
	d->conversation.setSystem(QString::fromUtf8(kSystemPrompt));
}

void AgentController::resolveApproval(bool approved)
{
	if (!d->approval)
		return;
	std::shared_ptr<QPromise<bool>> promise = std::move(d->approval);
	d->approval.reset();
	promise->addResult(approved);
	promise->finish();
}

void AgentController::setBusy(bool busy)
{
	if (d->busy == busy)
		return;
	d->busy = busy;
	emit busyChanged(busy);
}
