// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "gui.h"
#include "fileselectiondialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QKeyEvent>
#include <QClipboard>

//======================================================================
// GUI
//======================================================================
/**********************************************************************\
 * CONSTRUCTOR/DESTRUCTOR
\**********************************************************************/
Gui::Gui(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
	tabWidget->setCurrentIndex(0);
	listView_R->setEnabled(false);

	// Create dialog to capture log file details.
	// No need to keep a pointer to it after parenting and connecting signals.
	auto fileSelectionDialog = new FileSelectionDialog(this);
	connect(pb_newSession, &QPushButton::clicked,
			fileSelectionDialog, &FileSelectionDialog::exec);
	connect(fileSelectionDialog, &FileSelectionDialog::fileSelected,
			this, &Gui::newFileSelected);

	// Enable the 2nd list in "Diff" mode only
	connect(tabWidget, &QTabWidget::currentChanged, [=]()
	{
		listView_R->setEnabled(tabWidget->currentIndex() == 1);
	});

	connect(listView_L, &SessionListView::sessionChanged,
			this, &Gui::requestNewTables);
	connect(listView_R, &SessionListView::sessionChanged,
			this, &Gui::requestNewTables);

	connect(listView_L, &SessionListView::deletionRequested,
			this, &Gui::deletionRequested);
	connect(listView_R, &SessionListView::deletionRequested,
			this, &Gui::deletionRequested);

}


/**********************************************************************\
 * PRIVATE SLOTS
\**********************************************************************/
// Relay the request for the new session(s) to the outside world
void
Gui::requestNewTables() const
{
	auto index_L = listView_L->currentIndex();
	auto index_R = listView_R->currentIndex();

	QString session_L = listView_L->model()->data(index_L).toString();
	QString session_R = listView_L->model()->data(index_R).toString();

	emit sessionSelectionChanged(session_L, session_R);
}


/**********************************************************************\
 * PUBLIC
\**********************************************************************/
void
Gui::setFullModel(QAbstractTableModel* model)
{
	auto oldProxy = tv_full->model();
	auto newProxy = new QSortFilterProxyModel(this);

	newProxy->setSourceModel(model);
	tv_full->setModel(newProxy);

	if (oldProxy)
		oldProxy->deleteLater();
}

void
Gui::setDiffModels(QAbstractTableModel* leftModel, QAbstractTableModel* rightModel)
{
	auto oldProxy_L = tv_diff_L->model();
	auto oldProxy_R = tv_diff_R->model();
	auto newProxy_L = new QSortFilterProxyModel(this);
	auto newProxy_R = new QSortFilterProxyModel(this);

	newProxy_L->setSourceModel(leftModel);
	newProxy_R->setSourceModel(rightModel);
	tv_diff_L->setModel(newProxy_L);
	tv_diff_R->setModel(newProxy_R);

	if (oldProxy_L)
		oldProxy_L->deleteLater();
	if (oldProxy_R)
		oldProxy_R->deleteLater();
}

void
Gui::setSessionLists(QAbstractListModel* model)
{
	listView_L->setModel(model);
	listView_R->setModel(model);

	// Show the errors in the latest session (but not the diff)
	listView_L->setCurrentIndex(listView_L->model()->index(0, 0));
}


//======================================================================
// SESSIONLISTVIEW
//======================================================================
void
SessionListView::keyPressEvent(QKeyEvent* event)
{
	QString session = model()->data(currentIndex()).toString();
	if (event->matches(QKeySequence::Delete) && !session.isEmpty())
	{
		QString question = "Delete the session \""
				+ session
				+ "\"? This action cannot be undone.";
		auto selection = QMessageBox::question(nullptr, "Confirm Deletion", question);
		if (selection == QMessageBox::Yes)
			emit deletionRequested(session);
	}
}


//======================================================================
// SPREADSHEETVIEW
//======================================================================
void
SpreadsheetView::keyPressEvent(QKeyEvent *event)
{
	if (event->matches(QKeySequence::Copy))
		copySelectedText();
	else
		QTableView::keyPressEvent(event);

	// TODO: Implement delete/cut/paste
}

void
SpreadsheetView::copySelectedText() const
{
	QItemSelection sel = selectionModel()->selection();
	if (sel.size() == 0)
		return;

	// Find the bounding rectangle of the selection
	int top    = sel[0].top();
	int bottom = sel[0].bottom();
	int left   = sel[0].left();
	int right  = sel[0].right();
	for (int i = 1; i < sel.size(); ++i)
	{
		top    = std::min(top,    sel[i].top());
		bottom = std::max(bottom, sel[i].bottom());
		left   = std::min(left,   sel[i].left());
		right  = std::max(right,  sel[i].right());
	}

	QModelIndexList cells = sel.indexes();
	int width  = right-left+1;
	int height = bottom-top+1;


	// If selection is rectangular...
	if ( width*height == cells.count() )
	{
		std::sort(cells.begin(), cells.end());

		QString text;
		int i = 0;
		for (int row = 0; row < height; ++row)
		{
			// Add data to flattened string.
			// Some messages contain '\n', so those must be replaced.
			for (int col = 0; col < width; ++col)
				text += cells[i++].data().toString().replace('\n', ' ') + '\t';

			text.chop(1); // Remove last '\t'
			text += '\n';
		}
		// Note: Both Microsoft Excel 2013 and LibreOffice Calc 4 keep the last '\n'

		QGuiApplication::clipboard()->setText(text);
	}
}
