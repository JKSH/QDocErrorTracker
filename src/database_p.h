// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#ifndef DATABASE_P_H
#define DATABASE_P_H

#include <QSqlQueryModel>

class DatabaseModel : public QSqlQueryModel
{
	Q_OBJECT
public:
	explicit DatabaseModel(QObject* parent = nullptr) : QSqlQueryModel(parent) {}

	Qt::ItemFlags flags(const QModelIndex& index) const;
	bool setData(const QModelIndex& index, const QVariant& value, int role);
};

#endif // DATABASE_P_H
