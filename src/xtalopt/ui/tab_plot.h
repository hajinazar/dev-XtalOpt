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
    Featurei_T,
    Feature1_T,
    Feature2_T,
    Feature3_T,
    Feature4_T,
    Feature5_T,
    Feature6_T,
    Feature7_T,
    Feature8_T,
    Feature9_T,
    Feature10_T,
    Feature11_T,
    Feature12_T,
    Feature13_T,
    Feature14_T,
    Feature15_T,
    Feature16_T,
    Feature17_T,
    Feature18_T,
    Feature19_T,
    Feature20_T,
    Featuref_T
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
    Formula_Units_L,
    Featurei_L,
    Feature1_L,
    Feature2_L,
    Feature3_L,
    Feature4_L,
    Feature5_L,
    Feature6_L,
    Feature7_L,
    Feature8_L,
    Feature9_L,
    Feature10_L,
    Feature11_L,
    Feature12_L,
    Feature13_L,
    Feature14_L,
    Feature15_L,
    Feature16_L,
    Feature17_L,
    Feature18_L,
    Feature19_L,
    Feature20_L,
    Featuref_L
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
