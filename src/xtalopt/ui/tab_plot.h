/**********************************************************************
  XtalOpt - Tools for advanced crystal optimization

  Copyright (C) 2009-2011 by David Lonie

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 ***********************************************************************/

#ifndef TAB_PLOT_H
#define TAB_PLOT_H

#include <atomic>

#include <globalsearch/ui/abstracttab.h>

#include "ui_tab_plot.h"

class QReadWriteLock;

namespace GlobalSearch {
class AbstractDialog;
class Structure;
}

namespace XtalOpt {
class XtalOpt;
class Xtal;

class TabPlot : public GlobalSearch::AbstractTab
{
  Q_OBJECT

public:
  explicit TabPlot(GlobalSearch::AbstractDialog* parent, XtalOpt* p);
  virtual ~TabPlot() override;

  enum PlotAxes
  {
    Structure_T = 0,
    Generation_T,
    Enthalpy_T,
    Enthalpy_per_FU_T,
    Energy_T,
    Hardness_T,
    PV_T,
    A_T,
    B_T,
    C_T,
    Alpha_T,
    Beta_T,
    Gamma_A,
    Volume_T,
    Volume_per_FU_T,
    Formula_Units_T,
    // The feature entries: up to 50 entries will appear in the menu only; if multi-objective run.
    // Note: (1) Always put Feature* entries right after the last "fixed" entry (previous ones)!
    //       (2) Don't ever add any entries between below Feature* entries!
    Featurei_T,
    Featuref_T = Featurei_T + 50
    //
  };

  enum PlotType
  {
    Trend_PT = 0,
    DistHist_PT
  };

  enum LabelTypes
  {
    Number_L = 0,
    Symbol_L,
    Enthalpy_L,
    Enthalpy_Per_FU_L,
    Energy_L,
    Hardness_L,
    PV_L,
    Volume_L,
    Generation_L,
    Structure_L,
    StructureID_L,
    Formula_Units_L,
    // The feature entries: up to 50 entries will appear in the menu; only if multi-objective run.
    // Note: (1) Always put Feature* entries right after the last "fixed" entry (previous ones)!
    //       (2) Don't ever add any entries between below Feature* entries!
    Featurei_L,
    Featuref_L = Featurei_L + 50
    //
  };

public slots:
  void selectXtal(QwtPlotMarker* pm);
  void readSettings(const QString& filename = "") override;
  void writeSettings(const QString& filename = "") override;
  void updateGUI() override;
  void disconnectGUI() override;
  void enablePlotUpdate() { m_enablePlotUpdate = true; };
  void disablePlotUpdate() { m_enablePlotUpdate = false; };
  void refreshPlot();
  void updatePlot();
  void plotTrends();
  void plotDistHist();
  void selectMoleculeFromIndex(int index);
  void highlightXtal(GlobalSearch::Structure* s);
  void updatePlotFormulaUnits();

private:
  QwtPlotMarker* addXtalToPlot(Xtal* xtal, double x, double y);
  void plotTrace(double x1, double y1, double x2, double y2);

  std::atomic_bool m_enablePlotUpdate;

  Ui::Tab_Plot ui;
  QReadWriteLock* m_plot_mutex;
  QMap<QwtPlotMarker*, Xtal*> m_marker_xtal_map;
  QList<uint> m_formulaUnitsList;
};
}

#endif
