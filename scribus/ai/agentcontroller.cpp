/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/
#include "agentcontroller.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QFile>
#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QPromise>
#include <QTextStream>
#include <QTimer>

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
		"HOW YOU WORK (follow this every time you build or change something):\n"
		"1. You have two tools. test_script runs a Python script on a THROW-AWAY COPY of the user's document (or in scratch documents the script "
		"creates). It never changes the user's work and needs no permission. It returns the script's output, a verification report and, with some "
		"providers, a preview image. run_python runs a script on the user's REAL document; the user must approve it.\n"
		"2. Look first: use test_script to inspect what the document contains before you change it.\n"
		"3. Develop in the scratch copy. Write the script, call test_script, and READ the result carefully: the printed output, every line of the "
		"verification report, every PROBLEMS FOUND entry and the preview. Compare it with what the user asked for (text, sizes, fonts, positions, "
		"counts, nothing cut off, nothing under 5 pt). If anything is wrong or missing, say briefly what is wrong, fix the script and call "
		"test_script again. Repeat until the result is correct and complete. Do not stop at the first run that merely did not crash, and do not "
		"ask the user to run anything while it is still wrong. If after about eight attempts you cannot get it right, stop and tell the user "
		"exactly what is not working.\n"
		"4. Only when a test shows the correct result with no problems, call run_python ONCE with exactly the script that passed (same code), with a "
		"one-sentence summary. The user is shown the final script and the last preview and decides whether it runs on their real document.\n"
		"5. After run_python, say in one or two sentences what you did. If the user rejects the script, ask what to change, then test again.\n"
		"Never call run_python for exploring or experimenting, and never run an untested script on the real document.\n"
		"Do not call export_ad_pdf, saveDoc or any other function that writes a file inside test_script; put those only in the final run_python script, "
		"so the user approves them. (A test cannot check the PDF itself; check the document it exports.)\n"
		"\n"
		"Rules for scripts (for both tools; they run inside Scribus and can use the scribus module, from scribus import *):\n"
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
		"A bare file name goes to the user's Documents folder; if the user names a place such as the Desktop, pass a full path, for example os.path.join(os.path.expanduser('~'), 'Desktop', 'name.pdf'). The function prints the full path of the file: always tell the user that exact full path. Use PDF/X unless the user asks for a plain PDF.\n"
		"- create_column_notice(paragraphs, width_cm=3.5, height_cm=10.5, english_font='Times New Roman') makes a bordered notice for a narrow "
		"column: court, tribunal, consumer commission or revenue-court notices, summons and similar legal notices with no logo. paragraphs is a "
		"list of (text, align) or (text, align, size_factor) with align 'center', 'left', 'right' or 'justify'. Court name and case-number lines "
		"are centred with size_factor about 1.2; party names and the body are 'justify'; labels such as COMPLAINANTS and the signature lines are "
		"'right'. Copy the text exactly as in the client's document, one paragraph per printed paragraph. The size and the condensing are fitted "
		"for you (report as for create_notice_ad). Bold is not applied. Use export_ad_pdf for the PDF.\n"
		"When the user attaches a photo or scan of a printed notice: first read it and write out the complete transcription in your reply, "
		"keeping the paragraph structure, in Unicode Devanagari for Hindi, copying every spelling, number, date and punctuation mark exactly as "
		"printed (do not correct or modernise anything), and mark any word you cannot read with [?]. Name the kind of notice and the template you "
		"would use. Then STOP and ask the user to check the transcription and confirm; it is a legal text and the operator must approve it. "
		"Only after the user confirms (or sends corrections) build the ad.\n"
		"If the user wants an ad that none of these functions covers (a display ad with images, for example), say so instead of improvising a layout.\n"
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

			auto promise = std::make_shared<QPromise<LLMQore::ToolResult>>();
			promise->start();
			QFuture<LLMQore::ToolResult> future = promise->future();
			QTimer::singleShot(0, this, [this, promise, code, summary]() {
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
				promise->addResult(ok ? LLMQore::ToolResult::text(text) : LLMQore::ToolResult::error(text));
				promise->finish();
			});
			return future;
		}

	private:
		Runner m_runner;
		Reporter m_reporter;
	};

	/**
	 * Tries a script on a scratch copy of the document and reports what it produced: the script's output,
	 * automatic checks (text overflow, tiny text, objects off the page) and a preview image. ReadOnly, so the
	 * user is not asked: nothing in the real documents changes. The model uses it until the result is right,
	 * then runs the final script with run_python.
	 */
	class TestScriptTool : public LLMQore::BaseTool
	{
	public:
		using Runner = std::function<QVariantMap(const QString&, const QString&)>;
		using Reporter = std::function<void(const QString& summary, bool ok, const QString& text, const QString& preview)>;

		TestScriptTool(Runner runner, Reporter reporter, bool attachImage, QObject* parent)
			: LLMQore::BaseTool(parent),
			  m_runner(std::move(runner)),
			  m_reporter(std::move(reporter)),
			  m_attachImage(attachImage)
		{
		}

		QString id() const override { return QStringLiteral("test_script"); }
		QString displayName() const override { return QStringLiteral("Test a script on a scratch copy"); }
		LLMQore::ToolSafety safety() const override { return LLMQore::ToolSafety::ReadOnly; }
		QString description() const override
		{
			return QStringLiteral(
				"Runs a Python script (same scribus module and helper functions as run_python) on a THROW-AWAY COPY of the open "
				"document, or in scratch documents the script creates, and discards everything afterwards. The user's documents are never "
				"changed and the user is not asked. Returns what the script printed, a verification report (every object with position, size, "
				"fonts and sizes, text overflow, text under 5 pt, objects off the page) and, where supported, a preview image of page 1. "
				"Use it to inspect the document, and to develop a change: write the script, test it, read the report, fix the script, test again, "
				"until the result is right. Only then call run_python with the final script.");
		}
		QJsonObject parametersSchema() const override
		{
			return QJsonObject{
				{"type", "object"},
				{"properties", QJsonObject{
					{"code", QJsonObject{
						{"type", "string"},
						{"description", "The Python source code to test."}}},
					{"summary", QJsonObject{
						{"type", "string"},
						{"description", "Optional. One short sentence, in the user's language, saying what this attempt tries."}}}}},
				{"required", QJsonArray{"code"}}};
		}

		QFuture<LLMQore::ToolResult> executeAsync(const QJsonObject& input) override
		{
			const QString code = input.value("code").toString();
			const QString summary = input.value("summary").toString();
			if (code.trimmed().isEmpty())
				return QtFuture::makeReadyValueFuture(LLMQore::ToolResult::error(QStringLiteral("The code parameter is empty.")));

			// Run from the event loop, not inside the library's tool-call callback: the test opens and closes windows and
			// spins nested event loops, which must not happen while the library is still dispatching this call.
			auto promise = std::make_shared<QPromise<LLMQore::ToolResult>>();
			promise->start();
			QFuture<LLMQore::ToolResult> future = promise->future();
			QTimer::singleShot(0, this, [this, promise, code, summary]() {
				static int counter = 0;
				const QString previewPath = QDir::tempPath() + QStringLiteral("/scribus-agent-preview-%1-%2.png")
					.arg(QCoreApplication::applicationPid()).arg(++counter);
				QFile::remove(previewPath);
				const QVariantMap result = m_runner(code, previewPath);
				const bool ok = result.value("ok").toBool();
				const QString output = result.value("output").toString();
				const QString error = result.value("error").toString();
				const QString preview = result.value("preview").toString();

				QString text;
				if (ok)
				{
					text = QStringLiteral("TEST OK (scratch copy, nothing in the real document changed)\n") + output;
				}
				else
				{
					text = QStringLiteral("TEST FAILED (scratch copy, nothing in the real document changed)\n") + error;
					if (!output.isEmpty())
						text += QStringLiteral("\nOutput before the error:\n") + output;
				}
				text = clipped(text);
				m_reporter(summary, ok, text, preview);

				LLMQore::ToolResult toolResult = ok ? LLMQore::ToolResult::text(text) : LLMQore::ToolResult::error(text);
				if (m_attachImage && !preview.isEmpty())
				{
					QFile file(preview);
					if (file.open(QIODevice::ReadOnly))
						toolResult.content.append(LLMQore::ImageContent::fromBytes(file.readAll(), QStringLiteral("image/png")));
				}
				promise->addResult(toolResult);
				promise->finish();
			});
			return future;
		}

	private:
		Runner m_runner;
		Reporter m_reporter;
		bool m_attachImage;
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
	SandboxRunner sandboxRunner;
	QString lastPreview;
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

void AgentController::setSandboxRunner(SandboxRunner runner)
{
	d->sandboxRunner = std::move(runner);
}

QString AgentController::lastPreviewPath() const
{
	return d->lastPreview;
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

	// Scratch-copy testing (ReadOnly: no approval). Tool results carry the preview image only for providers that accept it.
	const bool imagesInToolResults = s.provider == Provider::Claude || s.provider == Provider::OpenAICompatible || s.provider == Provider::Gemini;
	client->tools()->addTool(new TestScriptTool(
		[this](const QString& code, const QString& previewPath) -> QVariantMap {
			if (!d->sandboxRunner)
			{
				return QVariantMap{
					{"ok", false},
					{"output", QString()},
					{"error", QStringLiteral("Scripting is not available in this build.")}};
			}
			d->turnHadOutput = true;
			return d->sandboxRunner(code, previewPath);
		},
		[this](const QString& summary, bool ok, const QString& text, const QString& preview) {
			agentLog(QStringLiteral("test %1: %2").arg(ok ? QStringLiteral("OK") : QStringLiteral("ERROR"), text));
			if (!preview.isEmpty())
				d->lastPreview = preview;
			emit scriptTested(summary, ok, text, preview);
		},
		imagesInToolResults, client));
	client->setMaxToolContinuations(30);

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

void AgentController::sendPrompt(const QString& text, const QStringList& imagePaths)
{
	if (d->busy || !d->client || (text.trimmed().isEmpty() && imagePaths.isEmpty()))
		return;
	if (imagePaths.isEmpty())
	{
		d->conversation.addUser(text);
	}
	else
	{
		QList<LLMQore::TurnContent> content;
		content.append(LLMQore::TextContent{text});
		for (const QString& path : imagePaths)
		{
			// Phone photos are several MB; models read text fine at about 2000 px, so shrink them before sending.
			QImage image(path);
			if (image.isNull())
				continue;
			constexpr int maxSide = 2000;
			if (image.width() > maxSide || image.height() > maxSide)
				image = image.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			QByteArray bytes;
			QBuffer buffer(&bytes);
			buffer.open(QIODevice::WriteOnly);
			image.convertToFormat(QImage::Format_RGB32).save(&buffer, "JPEG", 90);
			content.append(LLMQore::ImageContent::fromBytes(bytes, QStringLiteral("image/jpeg")));
		}
		d->conversation.addUser(content);
	}
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
