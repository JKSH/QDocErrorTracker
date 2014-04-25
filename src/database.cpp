// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "database.h"
#include "database_p.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

//======================================================================
// DATABASE
//======================================================================
/**********************************************************************\
 * CONSTRUCTOR/DESTRUCTOR
\**********************************************************************/
Database::Database(const QString& sqliteFile, QObject* parent)
	: QObject(parent)
	, _db(QSqlDatabase::addDatabase("QSQLITE"))
	, _sessionListModel(new QStringListModel(this))
	, _fullModel(new DatabaseModel(this))
	, _diffModel_L(new DatabaseModel(this))
	, _diffModel_R(new DatabaseModel(this))
{
	_db.setDatabaseName(sqliteFile);
	if (!_db.open())
	{
		qWarning() << "Failed to open" << sqliteFile;
		return;
	}

	QString createRepos =
			"CREATE TABLE IF NOT EXISTS Repos("
			"id INTEGER PRIMARY KEY,"
			"repo TEXT)";

	QString createFiles =
			"CREATE TABLE IF NOT EXISTS Files("
			"id INTEGER PRIMARY KEY,"
			"repo INTEGER REFERENCES Repos(id),"
			"file TEXT)";

	QString createMessages =
			"CREATE TABLE IF NOT EXISTS Messages("
			"id INTEGER PRIMARY KEY,"
			"message TEXT)";

	QString createErrors =
			"CREATE TABLE IF NOT EXISTS Errors("
			"id INTEGER PRIMARY KEY,"
			"file INTEGER REFERENCES Files(id),"
			"message INTEGER REFERENCES Messages(id),"
			"notes TEXT)";

	QString createSessions =
			"CREATE TABLE IF NOT EXISTS Sessions("
			"id INTEGER PRIMARY KEY,"
			"timestamp TEXT,"
			"comments TEXT)";

	QString createMain =
			"CREATE TABLE IF NOT EXISTS Main("
			"id INTEGER PRIMARY KEY,"
			"session INTEGER REFERENCES Sessions(id),"
			"error INTEGER REFERENCES Errors(id),"
			"line INTEGER)";

	QSqlQuery q;
	if (!q.exec("PRAGMA foreign_keys = ON"))
		qWarning() << "Enabling foreign keys:" << q.lastError().text();

	q.exec(createSessions);
	q.exec(createRepos);
	q.exec(createFiles);
	q.exec(createMessages);
	q.exec(createErrors);
	q.exec(createMain);

	if (!q.exec("SELECT id,timestamp,comments FROM Sessions"))
		qWarning() << "Loading Sessions:" << q.lastError().text();
	while (q.next())
	{
		QString entry = simplifyEntry(q.value("timestamp").toDateTime(),
				q.value("comments").toString());

		_sessionMap[entry] = q.value("id").toInt();
	}
	refreshSessionList();

	if (!q.exec("SELECT id,repo FROM Repos"))
		qWarning() << "Loading Repos:" << q.lastError().text();
	while (q.next())
		_msgMap[q.value("repo").toString()] = q.value("id").toInt();

	if (!q.exec("SELECT id,file,message FROM Errors"))
		qWarning() << "Loading Errors:" << q.lastError().text();
	while (q.next())
	{
		// Merge the File and Message foreign keys to form a composite key
		// ASSUMPTION: The number of files/messages won't hit 65k
		quint32 key = (q.value("file").toUInt() << 16) | q.value("message").toUInt();
		_errorMap[key] = q.value("id").toInt();
	}

	if (!q.exec("SELECT id,message FROM Messages"))
		qWarning() << "Loading Messages:" << q.lastError().text();
	while (q.next())
		_msgMap[q.value("message").toString()] = q.value("id").toInt();

	QString fileQuery =
			"SELECT Files.id,Repos.repo,Files.file FROM Files "
			"JOIN Repos ON Repos.id=Files.repo";
	if (!q.exec(fileQuery))
		qWarning() << "Loading Files:" << q.lastError().text();
	while (q.next())
	{
		QString joinedPath = q.value("repo").toString() + '/' + q.value("file").toString();
		_fileMap[joinedPath] = q.value("id").toInt();
	}
}


/**********************************************************************\
 * PUBLIC
\**********************************************************************/
void
Database::addSession(const Session& session)
{
	typedef QPair<QString, QVariant> Field;
	typedef QList<Field> Fields;

	QString sessionString = simplifyEntry(session.timestamp, session.comments);
	if (_sessionMap.contains(sessionString))
	{
		qWarning() << "Cannot add to database. Session already exists:" << sessionString;
		return;
	}

	// Use 1 transaction to INSERT everything. Painfully slow otherwise.
	QSqlQuery q(_db);
	q.exec("BEGIN");

	int sessionId = insert("Sessions", Fields()
			<< Field("timestamp", session.timestamp)
			<< Field("comments", session.comments));

	for (int i = 0; i < session.errors.count(); ++i)
	{
		const QString& msg = session.errors[i]->message;
		const QString& repo = session.errors[i]->repo;
		const QString& file = session.errors[i]->file;

		if (!_repoMap.contains(repo))
		{
			_repoMap[repo] = insert("Repos", Fields()
					<< Field("repo", repo));
		}
		int repoId = _repoMap[repo];

		if (!_msgMap.contains(msg))
		{
			_msgMap[msg] = insert("Messages", Fields()
					<< Field("message", msg));
		}
		int msgId = _msgMap[msg];

		QString longFilePath = repo + '/' + file;
		if (!_fileMap.contains(longFilePath))
		{
			_fileMap[longFilePath] = insert("Files", Fields()
					<< Field("repo", repoId)
					<< Field("file", file));
		}
		int fileId = _fileMap[longFilePath];

		quint32 errorKey = (fileId << 16) | msgId;
		if (!_errorMap.contains(errorKey))
		{
			_errorMap[errorKey] = insert("Errors", Fields()
					<< Field("file", fileId)
					<< Field("message", msgId));
		}
		int errorId = _errorMap[errorKey];

		insert("Main", Fields()
				<< Field("session", sessionId)
				<< Field("error", errorId)
				<< Field("line", session.errors[i]->line));
	}

	// Finalize transaction
	q.exec("COMMIT");

	_sessionMap[sessionString] = sessionId;
	refreshSessionList();
}

static const QString coreModelSelection =
		"SELECT Main.id,Repos.repo,Files.file,Main.line,Messages.message,Errors.notes FROM Main "
		"JOIN Errors ON Errors.id=Main.error "
		"JOIN Files ON Files.id=Errors.file "
		"JOIN Repos ON Repos.id=Files.repo "
		"JOIN Messages ON Messages.id=Errors.message ";

void
Database::setFullModel(const QString& session)
{
	QSqlQuery q(_db);
	q.prepare(coreModelSelection +
			"WHERE Main.session=?");
	q.addBindValue(_sessionMap[session]);
	q.exec();
	_fullModel->setQuery(q);

	// Must forcibly pull all data. Otherwise, the sorted table might have
	// intermediate rows missing.
	while (_fullModel->canFetchMore())
		_fullModel->fetchMore();
}

void
Database::setDiffModels(const QString& session1, const QString& session2)
{
	QSqlQuery q(_db);
	q.prepare(coreModelSelection +
			"WHERE Main.session=? "
			"AND Main.error NOT IN (SELECT error FROM Main WHERE session=?)");
	q.addBindValue(_sessionMap[session1]);
	q.addBindValue(_sessionMap[session2]);
	q.exec();
	_diffModel_L->setQuery(q);
	while (_diffModel_L->canFetchMore())
		_diffModel_L->fetchMore();

	q.prepare(coreModelSelection +
			"WHERE Main.session=? "
			"AND Main.error NOT IN (SELECT error FROM Main WHERE session=?)");
	q.addBindValue(_sessionMap[session2]);
	q.addBindValue(_sessionMap[session1]);
	q.exec();
	_diffModel_R->setQuery(q);
	while (_diffModel_R->canFetchMore())
		_diffModel_R->fetchMore();
}

/**********************************************************************\
 * PUBLIC SLOTS
\**********************************************************************/
void
Database::removeSession(const QString& session)
{
	QSqlQuery q(_db);
	q.prepare("DELETE FROM Main WHERE session=?");
	q.addBindValue(_sessionMap[session]);
	if (!q.exec())
		qWarning() << "Deleting Session from Main:" << q.lastError().text();

	q.prepare("DELETE FROM Sessions WHERE id=?");
	q.addBindValue(_sessionMap[session]);
	if (!q.exec())
		qWarning() << "Deleting Session:" << q.lastError().text();

	_sessionMap.remove(session);
	refreshSessionList();
}

/**********************************************************************\
 * PRIVATE
\**********************************************************************/
QString
Database::simplifyEntry(const QDateTime& timestamp, const QString& comments)
{
	QString timeString = timestamp.toString("yyyy-MM-dd hh:mm");
	if (comments.isEmpty())
		return timeString;
	else
		return timeString + " - " + comments;
}

int
Database::insert(const QString& table, QList<QPair<QString, QVariant>> fields)
{
	QString columns;
	QString values;
	for (int i = 0; i < fields.size(); ++i)
	{
		columns += fields[i].first + ',';
		values  += "?,";
	}
	columns.chop(1); // Remove trailing ','
	values.chop(1);

	QString query = QString("INSERT INTO %1(%2) VALUES(%3)")
			.arg(table).arg(columns).arg(values);

	QSqlQuery q(_db);
	q.prepare(query);
	for (int i = 0; i < fields.size(); ++i)
		q.addBindValue(fields[i].second);

	if (!q.exec())
	{
		qWarning() << "Inserting into" << table << ':' << q.lastError().text();
		return -1;
	}
	return q.lastInsertId().toInt();
}

void
Database::refreshSessionList()
{
	auto sessions = _sessionMap.keys();
	std::sort(sessions.begin(), sessions.end());
	std::reverse(sessions.begin(), sessions.end());
	_sessionListModel->setStringList(sessions);

	// TODO: Clear table models if session list changed
}

//======================================================================
// DATABASEMODEL
//======================================================================
Qt::ItemFlags
DatabaseModel::flags(const QModelIndex& index) const
{
	// Only notes can be updated manually
	Qt::ItemFlags flags = QSqlQueryModel::flags(index);
	if (headerData(index.column(), Qt::Horizontal) == "notes")
		flags |= Qt::ItemIsEditable;

	return flags;
}

bool
DatabaseModel::setData(const QModelIndex& index, const QVariant& value, int /*role*/)
{
	// Only notes can be updated manually
	if (headerData(index.column(), Qt::Horizontal) != "notes")
		return false;

	// TODO: Grab indices in a more robust manner
	QModelIndex repoIndex = this->index(index.row(), 1);
	QModelIndex fileIndex = this->index(index.row(), 2);
	QModelIndex msgIndex = this->index(index.row(), 4);

	// Update error notes. Errors are considered identical if the same message
	// originates from the same file
	QString updateQuery =
			"UPDATE Errors SET notes=? "
			"WHERE file="
			"    (SELECT id FROM Files WHERE file=? AND repo="
			"        (SELECT id FROM Repos WHERE repo=?)"
			"    ) "
			"AND message="
			"    (SELECT id FROM Messages WHERE message=?)";

	QSqlQuery q;
	q.prepare(updateQuery);
	q.addBindValue(value);
	q.addBindValue(data(fileIndex));
	q.addBindValue(data(repoIndex));
	q.addBindValue(data(msgIndex));
	bool ok = q.exec();

	// Refresh. Block modelAboutToBeReset()/modelReset() signals
	// to maintain the views' sort order and scroll position
	if (ok)
	{
		q = this->query();
		q.exec();

		this->blockSignals(true);
		this->setQuery(q);
		while (canFetchMore())
			fetchMore();
		this->blockSignals(false);
	}

	return ok;
}
