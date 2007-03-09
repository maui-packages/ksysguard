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

#ifndef PROCESSMODEL_H_
#define PROCESSMODEL_H_

#include <kapplication.h>
#include <kuser.h>
#include <QPixmap>
#include <QObject>
#include <QAbstractItemModel>
#include <QStringList>
#include <QList>
#include <QVariant>
#include <QHash>
#include <QSet>
#include <QTime>

#include "process.h"

namespace KSysGuard {
	class Processes;
}

extern KApplication* Kapp;

class ProcessModel : public QAbstractItemModel
{
	Q_OBJECT
		
public:
	ProcessModel(QObject* parent = 0);
	virtual ~ProcessModel() { }

	/* Functions for our Model for QAbstractItemModel*/
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount ( const QModelIndex & parent = QModelIndex() ) const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
	QModelIndex parent ( const QModelIndex & index ) const;
	
	bool hasChildren ( const QModelIndex & parent) const;
	
	/* Functions for setting the model */

	/** Set the untranslated heading names for the incoming data that will be sent in setData.
	 *  The column names we show to the user are based mostly on this information, translated if known, hidden if not necessary etc */
	void setupHeader();

	/** Update data */
	void update();
	
	/** Return a string with the pid of the process and the name of the process.  E.g.  13343: ksyguard
	 */
	QString getStringForProcess(KSysGuard::Process *process) const;
	inline KSysGuard::Process *getProcess(long long pid) { return mPidToProcess[pid]; }
        
	/** Returns whether this user can log in or not.
	 *  @see mUidCanLogin
	 */
	bool canUserLogin(long long uid) const;
	/** In simple mode, everything is flat, with no icons, few if any colors, no xres etc.
	 *  This can be changed at any time.  It is a fairly quick operation.  Basically it resets the model
	 */ 
	void setSimpleMode(bool simple);
	/** In simple mode, everything is flat, with no icons, few if any colors, no xres etc
	 */
	bool isSimpleMode() const { return mSimple;}
	
	/** Set the total amount of physical memory in the machine.  We can used this to determine the percentage of memory an app is using
	 */
	void setTotalMemory(long memTotal) { mMemTotal = memTotal; }

	/** Whether this is showing the processes for the current machine
	 */
	void setIsLocalhost(bool isLocalhost);

public slots:
	void setShowTotals(int totals);
private slots:
	/** Change the data for a process.  This is called from KSysGuard::Processes
	 *  if onlyCpu is set, only the cpu information is updated.  This is for optomization reasons - the cpu percentage
	 *  changes quite often, but if it's the only thing changed then there's no reason to repaint the whole row
	 */
        void processChanged(KSysGuard::Process *process, bool onlyCpu);
        /** Called from KSysGuard::Processes
	 *  This indicates we are about to insert a process in the model.  Emit the appropriate signals
	 */
	void beginInsertRow( KSysGuard::Process *parent);
        /** Called from KSysGuard::Processes
	 *  We have finished inserting a process
	 */
        void endInsertRow();
        /** Called from KSysGuard::Processes
	 *  This indicates we are about to remove a process in the model.  Emit the appropriate signals
	 */
	void beginRemoveRow( KSysGuard::Process *process);
        /** Called from KSysGuard::Processes
	 *  We have finished removing a process
	 */
        void endRemoveRow();
private:
	/** Connects to the host */
	void setupProcesses();
        /** A mapping of running,stopped,etc  to a friendly description like 'Stopped, either by a job control signal or because it is being traced.'*/
	QString getStatusDescription(KSysGuard::Process::ProcessStatus status) const;
	/** This returns a QModelIndex for the given process.  It has to look up the parent for this pid, find the offset this 
	 *  pid is from the parent, and return that.  It's not that slow, but does involve a couple of hash table lookups.
	 */
	QModelIndex getQModelIndex ( KSysGuard::Process *process, int column) const;
	
	/** Return a qt markup tooltip string for a local user.  It will have their full name etc.
	 *  This will be slow the first time, as it practically indirectly reads the whole of /etc/passwd
	 *  But the second time will be as fast as hash lookup as we cache the result
	 */
	inline QString getTooltipForUser(long long uid, long long gid) const;

	/** Return a username (as a QString) for a local user if it can, otherwise just their uid (as a long long).
	 *  This may have been given from the result of "ps" (but not necessarily).  If it's not found, then it
	 *  needs to find out the username from the uid. This will be slow the first time, as it practically indirectly reads the whole of /etc/passwd
	 *  But the second time will be as fast as hash lookup as we cache the result
	 */
	inline QVariant getUsernameForUser(long long uid) const;

	/** @see setIsLocalhost */
	bool mIsLocalhost;

	/** A caching hash for tooltips for a user.
	 *  @see getTooltipForUser */
	mutable QHash<long long,QString> mUserTooltips;

	/** A caching hash for (QString) username for a user uid, or just their uid if it can't be found (as a long long)
	 *  @see getUsernameForUser */
	mutable QHash<long long, QVariant> mUserUsername;

	/** A mapping of a user id to whether this user can log in.  We have to guess based on the shell. All are set to true to non localhost.
	 *  It is set to:
	 *    0 if the user cannot login
	 *    1 is the user can login
	 *  The reason for using an int and not a bool is so that we can do  mUidCanLogin.value(uid,-1)  and thus we get a tristate for whether
	 *  they are logged in, not logged in, or not known yet.
	 *  */
	mutable QHash<long long, int> mUidCanLogin; 


	/** The headings in the model.  The order here is the order that they are shown in.  If you change this, make sure you also change the 
	 *  setup header function
	 */
	enum { HeadingName=0, HeadingUser, HeadingCPUUsage, HeadingVmSize, HeadingMemory, HeadingSharedMemory, HeadingCommand };	
	/** A translated list of headings (column titles) in the order we want to display them. Used in headerData() */
	QStringList mHeadings;
	

	bool mShowChildTotals; ///If set to true, a parent will return the CPU usage of all its children recursively

	bool mSimple; ///In simple mode, the model returns everything as flat, with no icons, no xres etc.  This is set by changing cmbFilter
	QTime mTime; ///Time that we last updated the processes.  This is used to turn process user/sys time into user/sys %
	long mElapsedTimeCentiSeconds; ///Time elapsed since we last update the processes. Calculated from mTime

	long mMemTotal; /// the total amount of physical memory in the machine.  We can used this to determine the percentage of memory an app is using

	/** For a given process id, it returns a Process structure.
	 *  When a process is made, it will be stored here.  If the process is removed for here, it must be deleted to avoid leaking.
	 *  @see class KSysGuard::Process
	 */
	QHash<long, KSysGuard::Process *> mPidToProcess; ///This is filled in from KSysGuard::Processes::getInstance()->getProcesses();

	KSysGuard::Processes *mProcesses;  ///The processes instance
};

#endif
