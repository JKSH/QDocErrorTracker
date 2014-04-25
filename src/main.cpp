// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include <QApplication>
#include <QTextEdit>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include "database.h"
#include "gui.h"

static void
popupWarning(QtMsgType type, const QMessageLogContext& /*context*/, const QString& msg)
{
	QMessageBox::warning(nullptr, "Warning", msg);
	if (type == QtFatalMsg)
		abort();
}

static QList<QSharedPointer<RawError>>
parseFile(QFile& logFile, const QString& buildRoot)
{
	QStringList mainStuff;
	QStringList otherStuff;

	bool bodyStarted = false;
	while (!logFile.atEnd())
	{
		QString line = logFile.readLine();
		line.remove('\n');
		if (line.isEmpty())
			continue;

		if (!bodyStarted)
		{
			if (line.startsWith(buildRoot))
			{
				bodyStarted = true;
				mainStuff << line.remove(buildRoot);
			}
			else
				otherStuff << line;
		}
		else
		{
			if (line.startsWith(buildRoot))
				mainStuff << line.remove(buildRoot);
			else if (line.startsWith("    "))
				mainStuff.last().append('\n' + line);
			else
				otherStuff << line;
		}
	}

	QList<QSharedPointer<RawError>> entries;
	for (const QString& rawLine : mainStuff)
	{
		// Expect a format like
		// "C:/Qt/git/5.x.y/qtxmlpatterns/examples/xmlpatterns/xquery/doc/src/globalVariables.qdoc:28: warning:   EXAMPLE PATH DOES NOT EXIST: xmlpatterns/xquery/globalVariables"

		RawError* entry = new RawError;
		QStringList tokens = rawLine.split(": ");
		if (tokens.count() < 3)
		{
			QMessageBox::warning(nullptr, "Cannot parse line", rawLine);
			delete entry;
			continue;
		}

		QStringList fileInfo = tokens[0].split(':');
		if (fileInfo.count() < 2)
			entry->line = -1;
		else
			entry->line = fileInfo[1].toInt();

		entry->repo = fileInfo[0].split('/').first();
		entry->file = fileInfo[0].remove(entry->repo + '/');

		// Handle cases where the error message contains ": "
		entry->message = tokens[1];
		for (int i = 2; i < tokens.count(); ++i)
			entry->message += ": " + tokens[i];

		entries << QSharedPointer<RawError>(entry);
	}

	// QC: Show lines that aren't recorded in the database
	QTextEdit* leftOvers = new QTextEdit;
	for (const QString& line : otherStuff)
		leftOvers->append(line);

	leftOvers->setWindowTitle("Unrecorded Lines");
	leftOvers->setAttribute(Qt::WA_DeleteOnClose);
	leftOvers->setReadOnly(true);
	leftOvers->resize(600, 480);
	leftOvers->show();

	return entries;
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(popupWarning);
	QApplication a(argc, argv);

	// cd into the folder which contains the database file and the settings file
	QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
	QDir dir;
	dir.mkpath(dataPath);
	QDir::setCurrent(dataPath);

	Gui gui;
	Database db("data.db");

	// Upon user selection, parse the log file and add entries to the database
	QObject::connect(&gui, &Gui::newFileSelected, [&](const QString& logFilename,
			const QString& buildRoot, const QDateTime& timestamp, const QString& comments)
	{
		QFile logFile(logFilename);
		if (!logFile.open(QFile::ReadOnly|QFile::Text))
		{
			QMessageBox::warning(nullptr, "Error", "Can't open " + logFilename);
			return;
		}

		auto entries = parseFile(logFile, buildRoot);
		if (entries.size() == 0)
		{
			QMessageBox::warning(nullptr, "Warning",
					"No entries found. Please check that you have selected "
					"the correct log file and its corresponding build root.");

			return;
		}

		db.addSession({timestamp, comments, entries});
	});

	// When the user clicks on either of the session lists, fetch the corresponding
	// session from the database and update the table(s).
	QObject::connect(&gui, &Gui::sessionSelectionChanged, [&](const QString& s1, const QString& s2)
	{
		if (!s1.isEmpty())
		{
			db.setFullModel(s1);

			// No diff if only one session has been selected.
			if (!s2.isEmpty())
				db.setDiffModels(s1, s2);
		}
	});

	QObject::connect(&gui, &Gui::deletionRequested,
			&db, &Database::removeSession);

	// Populate + show GUI
	gui.setSessionLists(db.sessionListModel());
	gui.setFullModel(db.fullModel());
	gui.setDiffModels(db.diffModel_L(), db.diffModel_R());
	gui.show();

	return a.exec();
}
