/**********************************************************************
  GenerateXrd - Use ObjCryst++ to generate a simulated x-ray diffraction
                pattern

  Copyright (C) 2018 by Patrick Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#include <sstream>

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QString>

#include <globalsearch/iomain.h>
#include <globalsearch/formats/cmlformat.h>
#include <globalsearch/formats/obconvert.h>
#include <globalsearch/structure.h>

#include "generatexrd.h"

namespace GlobalSearch {

bool GenerateXrd::executeGenXrdPattern(const QStringList& args,
                                       const QByteArray& input,
                                       QByteArray& output)
{
  QString program;
  // If the GENXRDPATTERN_EXECUTABLE environment variable is set, then
  // use that
  QByteArray xrdExec = qgetenv("GENXRDPATTERN_EXECUTABLE");
  if (!xrdExec.isEmpty()) {
    program = xrdExec;
  } else {
// Otherwise, search in the current directory, and then ../bin
#ifdef _WIN32
    QString executable = "genXrdPattern.exe";
#else
    QString executable = "genXrdPattern";
#endif
    QString path = QCoreApplication::applicationDirPath();
    if (QFile::exists(path + "/" + executable))
      program = path + "/" + executable;
    else if (QFile::exists(path + "/../bin/" + executable))
      program = path + "/../bin/" + executable;
    else {
      debug("Error: could not find genXrdPattern executable!");
      return false;
    }
  }

  QProcess p;
  p.start(program, args);

  if (!p.waitForStarted()) {
    debug(QString("Error: The genXrdPattern executable at %1 failed to start.").arg(program));
    return false;
  }

  // Give it the input!
  p.write(input.data());

  // Close the write channel
  p.closeWriteChannel();

  if (!p.waitForFinished()) {
    debug(QString("Error: %1 failed to finish.").arg(program));
    output = p.readAll();
    debug(QString("Output is as follows:\n%1").arg(QString(output)));
    return false;
  }

  int exitStatus = p.exitStatus();
  output = p.readAll();

  if (exitStatus == QProcess::CrashExit) {
    debug(QString("Error: %1 crashed!\n"
                  "   Output is as follows:\n%2").arg(program).arg(QString(output)));
    return false;
  }

  if (exitStatus != QProcess::NormalExit) {
    debug(QString("Error: %1 finished abnormally with exit code %2\n"
                  "   Output is as follows:\n%3").arg(program).arg(exitStatus).arg(QString(output)));
    return false;
  }

  // We did it!
  return true;
}

bool GenerateXrd::generateXrdPattern(const Structure& s, XrdData& results,
                                     double wavelength, double peakwidth,
                                     size_t numpoints, double max2theta)
{
  // First, write the structure in CML format
  std::stringstream cml;

  if (!CmlFormat::write(s, cml)) {
    debug(QString("Error in %1: failed to convert structure '%2' to CML format!")
                  .arg(__FUNCTION__).arg(s.getIDString()));
    return false;
  }

  // Then, convert to CIF format
  QByteArray cif;
  if (!OBConvert::convertFormat("cml", "cif", cml.str().c_str(), cif)) {
    debug(QString("Error in %1: failed to convert CML"
                  "format to CIF format with obabel").arg(__FUNCTION__));
    return false;
  }

  // Now, execute genXrdPattern with the given inputs
  QStringList args;
  args << "--read-from-stdin"
       << "--wavelength=" + QString::number(wavelength)
       << "--peakwidth=" + QString::number(peakwidth)
       << "--numpoints=" + QString::number(numpoints)
       << "--max2theta=" + QString::number(max2theta);

  QByteArray output;
  if (!executeGenXrdPattern(args, cif, output)) {
    debug(QString("Error in %1: failed to run external"
                  "genXrdPattern program").arg(__FUNCTION__));
    return false;
  }

  // Store the results
  results.clear();
  bool dataStarted = false;

  QStringList lines = QString(output).split(QRegExp("[\r\n]"),
                                            QString::SkipEmptyParts);
  for (const auto& line : lines) {
    if (!dataStarted && line.contains("#    2Theta/TOF    ICalc")) {
      dataStarted = true;
      continue;
    }

    if (dataStarted) {
      QStringList rowData = line.split(" ", QString::SkipEmptyParts);
      if (rowData.size() != 2) {
        debug(QString("Error in %1: data read from"
                      "genXrdPattern appears to be corrupt! Data is:").arg(__FUNCTION__));
        for (const auto& lineTmp: lines)
          debug(lineTmp);
        return false;
      }
      results.push_back(
        std::make_pair(rowData[0].toDouble(), rowData[1].toDouble()));
    }
  }

  return true;
}

} // end namespace GlobalSearch
