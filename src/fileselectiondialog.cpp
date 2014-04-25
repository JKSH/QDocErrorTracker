// Copyright (c) 2014 Sze Howe Koh
// This code is licensed under the MIT license (see LICENSE.txt for details)

#include "fileselectiondialog.h"
#include <QFileInfo>
#include <QFileDialog>
#include <QSettings>

FileSelectionDialog::FileSelectionDialog(QWidget *parent)
	: QDialog(parent)
	, _settings("qdocerrortracker.ini", QSettings::IniFormat)
{
	setupUi(this);

	// Show last-used value
	le_root->setText(_settings.value("BuildRoot").toString());


	// Upon button click, prompt for the log file and grab its "Created"
	// timestamp -- that's a good indication of when the build occurred
	connect(tb_file, &QToolButton::clicked, [=]()
	{
		QString logFilename = QFileDialog::getOpenFileName(nullptr,
				"Select QDoc STDERR log", this->_settings.value("LogPath").toString());

		if (!logFilename.isEmpty())
		{
			le_file->setText(logFilename);

			QFileInfo info(logFilename);
			dateTimeEdit->setDateTime(info.created());
		}
	});


	// Upon button click, ask for the build root. This path will later be stripped
	// from the data, so that builds from different paths/machines can be compared.
	//
	// TODO: Use heuristics to find the build root from the log
	connect(tb_root, &QToolButton::clicked, [=]()
	{
		QString buildRootPath = QFileDialog::getExistingDirectory(nullptr,
				"Select the root directory of your Qt source code", this->_settings.value("BuildRoot").toString());

		if (!buildRootPath.isEmpty())
			le_root->setText(buildRootPath);
	});


	// Upon button click, send the parameters to the outside world
	// to parse the log file
	connect(this, &FileSelectionDialog::accepted, [=]()
	{
		QString buildRoot = le_root->text();
		if (!buildRoot.endsWith('/'))
			buildRoot += '/';

		_settings.setValue("LogPath", le_file->text());
		_settings.setValue("BuildRoot", buildRoot);

		emit this->fileSelected(
				le_file->text(),
				buildRoot,
				dateTimeEdit->dateTime(),
				le_comments->text()
		);
	});
}
