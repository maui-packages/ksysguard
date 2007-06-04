/*
    KSysGuard, the KDE System Guard

	Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>
	Copyright (c) 2006 John Tapsell <john.tapsell@kdemail.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef _KSysGuardProcessList_h_
#define _KSysGuardProcessList_h_

#include <QWidget>
#include <kapplication.h>
#include "ProcessModel.h"
#include "ProcessFilter.h"

class QShowEvent;
class QHideEvent;
class QAction;
class QMenu;
class QLineEdit;
class QTreeView;
class QCheckBox;
namespace Ui {
  class ProcessWidget;
}

extern KApplication* Kapp;

/**
 * This widget implements a process list page. Besides the process
 * list which is implemented as a ProcessList, it contains two
 * comboxes and two buttons.  The combo boxes are used to set the
 * update rate and the process filter.  The buttons are used to force
 * an immediate update and to kill a process.
 */
class KDEUI_EXPORT KSysGuardProcessList : public QWidget
{
	Q_OBJECT

public:
	KSysGuardProcessList(QWidget* parent);
	virtual ~KSysGuardProcessList() { }

	QLineEdit *filterLineEdit() const;
	QTreeView *treeView() const;
	QCheckBox *chkShowTotals() const;

public slots:

	void expandAllChildren(const QModelIndex &parent);
	void currentRowChanged(const QModelIndex &current);
		
	void killProcess();
	void killProcess(const QList< long long> &pids, int sig);

	void reniceProcess();
	void reniceProcess(const QList<long long> &pids, int niceValue);

	void updateList();
private slots:
	void expandInit();
	void showContextMenu(const QPoint &point);
	void showProcessContextMenu(const QPoint &point);
	void showOrHideColumn(QAction *);
	void killFailed();
	void reniceFailed();
	void setSimpleMode(int index);
signals:
	void setFilterMode(int);

private:
	enum { Ps_Info_Command = 1, Ps_Command, Kill_Command, Kill_Supported_Command, Renice_Command, XRes_Info_Command, XRes_Command, XRes_Supported_Command, MemFree_Command, MemUsed_Command };
	/** Does the ksysguardd daemon support killing a process? */
	bool mKillSupported;
	
	/** The column context menu when you right click on a column.*/
	QMenu *mColumnContextMenu;
	/** The context menu when you right click on a process */
	QMenu *mProcessContextMenu;
	
	/** The amount of real physical memory being used in total, in kilobytes */
	long mMemUsed;
	/** The amount of free physical memory, in kilobytes */
	long mMemFree;
	/** The total amount of physical memory in the machine, in kilobytes */
	long mMemTotal;

	/** Whether we are in simple mode.  Must be kept in sync with the cmbFilter and mModel.mSimple
	 */
	bool mSimple;

	/** The process model.  This contains all the data on all the processes running on the system */
	ProcessModel mModel;
	/** The process filter.  The mModel is connected to this, and this filter model connects to the view.  This lets us
	 *  sort the view and filter (by using the combo box or the search line)
	 */
	ProcessFilter mFilterModel;
	/** The logical index in the header of the xres headings inclusive
	 */
	int mXResHeadingStart;
	/** The logical index in the header of the xres headings inclusive
	 */
	int mXResHeadingEnd;
	/** The graphical user interface for this process list widget, auto-generated by Qt Designer */
	Ui::ProcessWidget *mUi;

	QTimer *mUpdateTimer;

	virtual void showEvent(QShowEvent*);
	virtual void hideEvent(QHideEvent*);
};

#endif
