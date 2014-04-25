// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#ifndef FILESELECTIONDIALOG_H
#define FILESELECTIONDIALOG_H

#include "ui_fileselectiondialog.h"
#include <QSettings>

class FileSelectionDialog : public QDialog, private Ui::FileSelectionDialog
{
	Q_OBJECT

signals:
	void fileSelected(const QString& filename, const QString& buildRoot, const QDateTime& timestamp, const QString& comments) const;

public:
	explicit FileSelectionDialog(QWidget *parent = nullptr);

private:
	QSettings _settings;
};

#endif // FILESELECTIONDIALOG_H
