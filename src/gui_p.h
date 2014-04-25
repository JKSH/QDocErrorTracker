// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#ifndef GUI_P_H
#define GUI_P_H

#include <QListView>
#include <QTableView>

class SessionListView : public QListView
{
	Q_OBJECT

public:
	explicit SessionListView(QWidget* parent = nullptr) : QListView(parent) {}

signals:
	void sessionChanged() const;
	void deletionRequested(const QString& session) const;

protected slots:
	void selectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
	{ emit sessionChanged(); }

protected:
	void keyPressEvent(QKeyEvent* event);
};

class SpreadsheetView : public QTableView
{
	Q_OBJECT

public:
	SpreadsheetView(QWidget* parent = nullptr) : QTableView(parent) {}

protected:
	void keyPressEvent(QKeyEvent *event);

private:
	void copySelectedText() const;
};

#endif // GUI_P_H
