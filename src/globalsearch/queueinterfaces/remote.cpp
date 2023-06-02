/**********************************************************************
  RemoteQueueInterface - Base class for running jobs remotely.

  Copyright (C) 2011 by David C. Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifdef ENABLE_SSH

#include <globalsearch/queueinterfaces/remote.h>

#include <globalsearch/sshconnection.h>
#include <globalsearch/sshmanager.h>
#include <globalsearch/structure.h>

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QProcess>
#include <QString>

namespace GlobalSearch {

RemoteQueueInterface::RemoteQueueInterface(OptBase* parent,
                                           const QString& settingFile)
  : QueueInterface(parent)
{
  m_idString = "AbstractRemote";
}

RemoteQueueInterface::~RemoteQueueInterface()
{
}

bool RemoteQueueInterface::writeFiles(
  Structure* s, const QHash<QString, QString>& fileHash) const
{
  // Create file objects
  QList<QFile*> files;
  QStringList filenames = fileHash.keys();
  for (int i = 0; i < filenames.size(); i++) {
    files.append(new QFile(s->fileName() + "/" + filenames.at(i)));
  }

  // Check that the files can be written to
  for (int i = 0; i < files.size(); i++) {
    if (!files.at(i)->open(QIODevice::WriteOnly | QIODevice::Text)) {
      m_opt->error(tr("Cannot write input file %1 (file writing failure)",
                      "1 is a file path")
                     .arg(files.at(i)->fileName()));
      qDeleteAll(files);
      return false;
    }
  }

  // Set up text streams
  QList<QTextStream*> streams;
  for (int i = 0; i < files.size(); i++) {
    streams.append(new QTextStream(files.at(i)));
  }

  // Write files
  for (int i = 0; i < streams.size(); i++) {
    *(streams.at(i)) << fileHash[filenames.at(i)];
  }

  // Close files
  for (int i = 0; i < files.size(); i++) {
    files.at(i)->close();
  }

  // Clean up
  qDeleteAll(streams);
  qDeleteAll(files);

  // If there are copy files, copy those to the dir as well.
  if (!s->copyFiles().empty()) {
    for (const auto& copyFile : s->copyFiles()) {
      QFile infile(copyFile.c_str());
      QString filename = QFileInfo(infile).fileName();
      QFile outfile(s->fileName() + "/" + filename);
      if (!infile.copy(outfile.fileName())) {
        m_opt->error(tr("Failed to copy file %1 to %2")
                       .arg(infile.fileName())
                       .arg(outfile.fileName()));
        return false;
      }

      // Also append them to filenames so that they will be copied to
      // the remote dir
      filenames.append(filename);
    }
    s->clearCopyFiles();
  }

  // Copy to remote
  if (m_opt->m_localQueue) {
    if (!createLocalRemDirectory(s) || !cleanLocalRemDirectory(s))
      return false;
    for (QStringList::const_iterator it = filenames.constBegin(),
        it_end = filenames.constEnd(); it != it_end; ++it) {
      QString stdout_str, stderr_str;
      QProcess proc;
      QString command = "scp " + s->fileName() + "/" + (*it) + " " + s->getRempath() + "/" + (*it);
      proc.start(command);
      proc.waitForFinished();
      stdout_str = QString(proc.readAllStandardOutput());
      stderr_str = QString(proc.readAllStandardError());
      if (!stderr_str.isEmpty()) {
        m_opt->warning(tr("=== Executing %1 === Output %2 "
              "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
        return false;
      }
    }
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning(tr("Cannot connect to ssh server."));
      return false;
    }
    if (!createRemoteDirectory(s, ssh) || !cleanRemoteDirectory(s, ssh)) {
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    for (QStringList::const_iterator it = filenames.constBegin(),
        it_end = filenames.constEnd(); it != it_end; ++it) {
      if (!ssh->copyFileToServer(s->fileName() + "/" + (*it),
            s->getRempath() + "/" + (*it))) {
        m_opt->warning(tr("Error copying \"%1\" to remote server (structure %2)")
            .arg(*it)
            .arg(s->getIDString()));
        m_opt->ssh()->unlockConnection(ssh);
        return false;
      }
    } 
    m_opt->ssh()->unlockConnection(ssh);
  }

  return true;
}

bool RemoteQueueInterface::prepareForStructureUpdate(Structure* s) const
{

  // This delay is needed to make sure all output files are fully written;
  //   for local-remote runs sometimes quickly copying files causes problems!
  if (m_opt->m_localQueue) {
    return copyLocalRemFilesToLocalCache(s);
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning(tr("Cannot connect to ssh server"));
      return false;
    }
    if (!copyRemoteFilesToLocalCache(s, ssh)) {
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }

  return true;
}

bool RemoteQueueInterface::runGenericCommand(const QString& workdir, 
                                             const QString& command)
{
  QString stdout_str;
  QString stderr_str;

  if (m_opt->m_localQueue) {
    QProcess proc;
    proc.setWorkingDirectory(workdir);
    proc.start(command);
    proc.waitForFinished(-1);
    stdout_str = QString(proc.readAllStandardOutput());
    stderr_str = QString(proc.readAllStandardError());
    if (!stderr_str.isEmpty()) {
      m_opt->warning(tr("=== Executing %1 === Output %2 "
            "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
      return false;
    }
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning("Cannot connect to ssh server");
      return false;
    }
    QString runcom;
    int ec;
    runcom = "cd \"" + workdir + "\" && " + command;
    if (!ssh->execute(runcom, stdout_str, stderr_str, ec) || ec !=0) {
      m_opt->warning(tr("Remote command %1 at %2 failed!")
          .arg(command).arg(workdir));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }

  return true;
}

bool RemoteQueueInterface::copyGenericFileFromServer(const QString& rem_file, 
                                                     const QString& loc_file)
{
  if (m_opt->m_localQueue) {
    QString stdout_str, stderr_str;
    QString command = "scp " + rem_file + " " + loc_file;
    QProcess proc;
    proc.start(command);
    proc.waitForFinished();
    stdout_str = QString(proc.readAllStandardOutput());
    stderr_str = QString(proc.readAllStandardError());
    if (!stderr_str.isEmpty()) {
      m_opt->warning(tr("=== Executing %1 === Output %2 "
            "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
      return false;
    }
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning("Cannot connect to ssh server");
      return false;
    }
    if (!ssh->copyFileFromServer(rem_file, loc_file)) {
      m_opt->warning(tr("Failed copying '%1' from remote server to local '%2'")
          .arg(rem_file).arg(loc_file));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }

  return true;
}

bool RemoteQueueInterface::copyGenericFileToServer(const QString& loc_file, 
                                                   const QString& rem_file)
{
  if (m_opt->m_localQueue) {
    QString stdout_str, stderr_str;
    QString command = "scp " + loc_file + " " + rem_file;
    QProcess proc;
    proc.start(command);
    proc.waitForFinished();
    stdout_str = QString(proc.readAllStandardOutput());
    stderr_str = QString(proc.readAllStandardError());
    if (!stderr_str.isEmpty()) {
      m_opt->warning(tr("=== Executing %1 === Output %2 "
            "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
      return false;
    }
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning("Cannot connect to ssh server");
      return false;
    }
    if (!ssh->copyFileToServer(loc_file, rem_file)) {
      m_opt->warning(tr("Failed copying '%1' to remote server '%2'")
          .arg(loc_file).arg(rem_file));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }

  return true;
}

bool RemoteQueueInterface::checkIfGenericFileExists(const QString& spath,
                                                    const QString& sfile)
{
  const QString searchPath = spath;
  QStringList haystack;
  QString needle;

  if (m_opt->m_localQueue) {
    needle = spath + sfile;
    QString stdout_str, stderr_str;
    QProcess proc;
    QString command = "find " + searchPath;
    proc.start(command);
    proc.waitForFinished();
    stdout_str = QString(proc.readAllStandardOutput());
    stderr_str = QString(proc.readAllStandardError());
    if (!stderr_str.isEmpty()) {
      m_opt->warning(tr("=== Executing %1 === Output %2 "
            "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
      return false;
    }
    haystack = stdout_str.split("\n", QString::SkipEmptyParts);
  } else {
    needle = spath + "/" + sfile;
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning(tr("Cannot connect to ssh server"));
      return false;
    }
    if (!ssh->readRemoteDirectoryContents(searchPath, haystack)) {
      m_opt->warning(tr("Error reading directory %1 on %2@%3:%4")
          .arg(searchPath)
          .arg(ssh->getUser())
          .arg(ssh->getHost())
          .arg(ssh->getPort()));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }

  bool exists = false;
  for (QStringList::const_iterator it = haystack.constBegin(),
      it_end = haystack.constEnd();
      it != it_end; ++it) {
    if (it->compare(needle) == 0) {
      // Ouch!
      exists = true;
      break;
    }
  }

  return exists;
}

bool RemoteQueueInterface::checkIfFileExists(Structure* s,
                                             const QString& filename,
                                             bool* exists)
{
  const QString searchPath = s->getRempath();
  QString needle;
  QStringList haystack;

  if (m_opt->m_localQueue) {
    needle = s->getRempath() + filename;
    QProcess proc;
    QString stdout_str;
    QString stderr_str;
    QString command = "find " + searchPath;
    proc.start(command);
    proc.waitForFinished();
    stdout_str = QString(proc.readAllStandardOutput());
    stderr_str = QString(proc.readAllStandardError());
    if (!stderr_str.isEmpty()) {
      m_opt->warning(tr("=== Executing %1 === Output %2 "
            "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
      return false;
    }
    haystack = stdout_str.split("\n", QString::SkipEmptyParts);
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning(tr("Cannot connect to ssh server"));
      return false;
    }
    needle = s->getRempath() + "/" + filename;
    if (!ssh->readRemoteDirectoryContents(searchPath, haystack)) {
      m_opt->warning(tr("Error reading directory %1 on %2@%3:%4")
          .arg(searchPath)
          .arg(ssh->getUser())
          .arg(ssh->getHost())
          .arg(ssh->getPort()));
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }

  *exists = false;
  for (QStringList::const_iterator it = haystack.constBegin(),
                                   it_end = haystack.constEnd();
       it != it_end; ++it) {
    if (it->compare(needle) == 0) {
      // Ouch!
      *exists = true;
      break;
    }
  }

  return true;
}

bool RemoteQueueInterface::fetchFile(Structure* s, const QString& rel_filename,
                                     QString* contents) const
{
  if (m_opt->m_localQueue) {
    QProcess proc;
    QString stdout_str;
    QString stderr_str;
    QString command = "cat " + s->fileName() + "/" + rel_filename ;
    proc.setWorkingDirectory(rel_filename);
    proc.start(command);
    proc.waitForFinished();
    stdout_str = QString(proc.readAllStandardOutput());
    stderr_str = QString(proc.readAllStandardError());
    if (!stderr_str.isEmpty()) {
      m_opt->warning(tr("=== Executing %1 === Output %2 "
            "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
      return false;
    }
  } else {
    SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
    if (ssh == nullptr) {
      m_opt->warning(tr("Cannot connect to ssh server"));
      return false;
    }
    if (!ssh->readRemoteFile(s->fileName() + "/" + rel_filename, *contents)) {
      m_opt->ssh()->unlockConnection(ssh);
      return false;
    }
    m_opt->ssh()->unlockConnection(ssh);
  }
   
  return true;
}

bool RemoteQueueInterface::grepFile(Structure* s, const QString& matchText,
                                    const QString& filename,
                                    QStringList* matches, int* exitcode,
                                    const bool caseSensitive) const
{
  // Since network latency / transfer rates are much slower than
  // reading the file, call grep on the remote server and only
  // transfer back the matches.
  Qt::CaseSensitivity ces = Qt::CaseSensitive;
  QString flags = "";
  if (!caseSensitive) {
    flags = "-i";
    ces = Qt::CaseInsensitive;
  }
  QString stdout_str;
  QString stderr_str;
  int ec;

  if (m_opt->m_localQueue) {
    if (exitcode) {
      *exitcode = 1;
    }
    // Read the file
    QFile infile(s->getRempath() + "/" + filename);
    if (!infile.open(QFile::ReadOnly | QFile::Text)) {
      return false;
    }
    QTextStream in (&infile);
    QString line;
    do {
      line = in.readLine();
      if (line.contains(matchText, ces)) {
        if (matches) {
          *matches << line;
        }
        if (exitcode) {
          *exitcode = 0;
        }
      }
    } while (!line.isNull());
    return true;
  }

  // If its not a local-remote run; we start from here!
  SSHConnection* ssh = m_opt->ssh()->getFreeConnection();
  if (ssh == nullptr) {
    m_opt->warning(tr("Cannot connect to ssh server"));
    return false;
  }
  if (!ssh->execute(QString("grep %1 '%2' %3/%4")
        .arg(flags)
        .arg(matchText)
        .arg(s->getRempath())
        .arg(filename),
        stdout_str, stderr_str, ec)) {
    m_opt->ssh()->unlockConnection(ssh);
    return false;
  }
  if (exitcode) {
    *exitcode = ec;
  }
  m_opt->ssh()->unlockConnection(ssh);
  if (matches) {
    *matches = stdout_str.split('\n', QString::SkipEmptyParts);
  }
  return true;
}

bool RemoteQueueInterface::createRemoteDirectory(Structure* structure,
                                                 SSHConnection* ssh) const
{
  if (m_opt->m_localQueue)
    return false;

  QProcess proc;
  QString command = "mkdir -p \"" + structure->getRempath() + "\"";
  QString stdout_str, stderr_str;

  int ec;
  if (!ssh->execute(command, stdout_str, stderr_str, ec) || ec != 0) {
    m_opt->warning(tr("Error executing %1: %2").arg(command).arg(stderr_str));
    return false;
  }

  return true;
}

bool RemoteQueueInterface::cleanRemoteDirectory(Structure* structure,
                                                SSHConnection* ssh) const
{
  if (m_opt->m_localQueue)
    return false;

  // 2nd arg keeps the directory, only removes directory contents.
  if (!ssh->removeRemoteDirectory(structure->getRempath(), true)) {
    m_opt->warning(
        tr("Error clearing remote directory %1").arg(structure->getRempath()));
    return false;
  }

  return true;
}

bool RemoteQueueInterface::logErrorDirectory(Structure* structure,
                                             SSHConnection* ssh) const
{
  QString path = this->m_opt->filePath;

// Make the directory and copy the files into it
#ifdef WIN32
  path += "\\errorDirs\\";
#else
  path += "/errorDirs/";
#endif
  path += (QString::number(structure->getGeneration()) + "x" +
           QString::number(structure->getIDNumber()));
  if (!ssh->copyDirectoryFromServer(structure->getRempath(), path)) {
    m_opt->error("Cannot copy from remote directory for Structure " +
                 structure->getIDString());
    return false;
  }
  return true;
}

bool RemoteQueueInterface::copyRemoteFilesToLocalCache(Structure* structure,
                                                       SSHConnection* ssh) const
{
  if (m_opt->m_localQueue)
    return false;

  if (!ssh->copyDirectoryFromServer(structure->getRempath(),
        structure->fileName())) {
    m_opt->error("Cannot copy from remote directory for Structure " +
        structure->getIDString());
    return false;
  }
  return true;
}

bool RemoteQueueInterface::createLocalRemDirectory(Structure* structure) const
{
  QDir dir;
  return dir.mkpath(structure->getRempath());
}

bool RemoteQueueInterface::cleanLocalRemDirectory(Structure* structure) const
{
  // This is written only for the case of local-remote runs: localQueue
  //   to be called from the main routine: cleanRemoteDirectory
  // Since this is, basically, a local path, we can just remove
  //   the directory content with native Qt functions.
  QDir dir(structure->getRempath());
  dir.setNameFilters(QStringList() << "*");
  dir.setFilter(QDir::Files);
  foreach(QString dirFile, dir.entryList()) {
    dir.remove(dirFile);
  }

  return true;
}

bool RemoteQueueInterface::copyLocalRemFilesToLocalCache(Structure* structure) const
{
  QString stdout_str, stderr_str;
  QProcess proc;
  QString command = "scp -r " + structure->getRempath() + " " + structure->fileName() + ".."; 
  proc.start(command);
  proc.waitForFinished();
  stdout_str = QString(proc.readAllStandardOutput());
  stderr_str = QString(proc.readAllStandardError());
  if (!stderr_str.isEmpty()) {
    m_opt->warning(tr("=== Executing %1 === Output %2 "
          "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
    return false;
  }
  return true;
}

bool RemoteQueueInterface::logLocalRemErrorDirectory(Structure* structure) const
{
  QString path = this->m_opt->filePath;

// Make the directory and copy the files into it
#ifdef WIN32
  path += "\\errorDirs\\";
#else
  path += "/errorDirs/";
#endif
  path += (QString::number(structure->getGeneration()) + "x" +
           QString::number(structure->getIDNumber()));

  QDir dir;
  if (!dir.mkpath(path)) {
    m_opt->warning("Error: could not create error directory " + path);
  }

  QString stdout_str, stderr_str;
  QProcess proc;
  QString command = "scp -r " + structure->getRempath() + " " + path;

  proc.start(command);
  proc.waitForFinished();
  stdout_str = QString(proc.readAllStandardOutput());
  stderr_str = QString(proc.readAllStandardError());
  if (!stderr_str.isEmpty()) {
    m_opt->warning(tr("=== Executing %1 === Output %2 "
          "=== Error %3").arg(command).arg(stdout_str).arg(stderr_str));
    return false;
  }

  return true;
}
}

#endif // ENABLE_SSH
