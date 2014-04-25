// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#ifndef GUI_H
#define GUI_H

#include "ui_gui.h"

class QAbstractTableModel;
class QAbstractListModel;

class Gui : public QWidget, private Ui::Gui
{
	Q_OBJECT

public:
	explicit Gui(QWidget *parent = nullptr);

	void setFullModel(QAbstractTableModel* model);
	void setDiffModels(QAbstractTableModel* leftModel, QAbstractTableModel* rightModel);
	void setSessionLists(QAbstractListModel* model);

signals:
	void newFileSelected(const QString& filename, const QString& buildRoot, const QDateTime& timestamp, const QString& comments) const;
	void sessionSelectionChanged(const QString& session_L, const QString& session_R) const;
	void deletionRequested(const QString& session) const;

private slots:
	void requestNewTables() const;
};

#endif // GUI_H
