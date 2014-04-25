// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QStringListModel>
#include <QSqlQueryModel>
#include <QSqlDatabase>
#include <QDateTime>
#include <QMap>

struct RawError
{
	QString repo;
	QString file;
	int line;
	QString message;
};

struct Session
{
	QDateTime timestamp;
	QString comments;
	QList<QSharedPointer<RawError>> errors;
};

class Database : public QObject
{
	Q_OBJECT

public:
	Database(const QString& sqliteFile, QObject* parent = nullptr);
	~Database() { _db.close(); }

	void addSession(const Session& session);

	// Functions to get pointers to the internal data
	QAbstractListModel* sessionListModel() const {return _sessionListModel;}
	QAbstractTableModel* fullModel() const {return _fullModel;}
	QAbstractTableModel* diffModel_L() const {return _diffModel_L;}
	QAbstractTableModel* diffModel_R() const {return _diffModel_R;}

	// Functions to update internal data
	void setFullModel(const QString& session);
	void setDiffModels(const QString& session1, const QString& session2);

public slots:
	void removeSession(const QString& session);

private:
	static QString simplifyEntry(const QDateTime& timestamp, const QString& comments = QString());
	int insert(const QString& table, QList<QPair<QString, QVariant>> fields);
	void refreshSessionList();

	QSqlDatabase _db;

	// Caches for database keys
	QMap<QString, int> _sessionMap;
	QMap<QString, int> _repoMap;
	QMap<QString, int> _fileMap;
	QMap<QString, int> _msgMap;
	QMap<quint32, int> _errorMap;

	QStringListModel* _sessionListModel;
	QSqlQueryModel* _fullModel;
	QSqlQueryModel* _diffModel_L;
	QSqlQueryModel* _diffModel_R;
};

#endif // DATABASE_H
