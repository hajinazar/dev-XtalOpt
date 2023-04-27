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

#include <xtalopt/ui/tab_mo.h>

#include <xtalopt/ui/dialog.h>
#include <xtalopt/xtalopt.h>

#include <QDebug>
#include <QSettings>

#include <QFileDialog>
#include <QMessageBox>

#include <QInputDialog>
#include <QHeaderView>

using namespace std;

namespace XtalOpt {

TabMo::TabMo(GlobalSearch::AbstractDialog* parent, XtalOpt* p)
  : AbstractTab(parent, p)
{
  ui.setupUi(m_tab_widget);

  // Setup feature table header sizes
  ui.table_features->setColumnWidth(0, 150);
  ui.table_features->setColumnWidth(1, 350);
  ui.table_features->setColumnWidth(2, 200);
  ui.table_features->horizontalHeader()->setSectionResizeMode(3,QHeaderView::Stretch);

  // Before we make any connections, let's read the settings
  readSettings();

  // Update active button if features are being entered; this is to avoid doing anything but adding!
  connect(ui.combo_type, SIGNAL(activated(int)), this, SLOT(setActiveButtonAdd()));
  connect(ui.line_output, SIGNAL(returnPressed()), this, SLOT(setActiveButtonAdd()));
  connect(ui.line_path, SIGNAL(returnPressed()), this, SLOT(setActiveButtonAdd()));
  connect(ui.table_features, SIGNAL(cellClicked(int, int)), this, SLOT(setActiveButtonRemove()));

  // Update fields with opt type selection
  connect(ui.combo_type, SIGNAL(currentIndexChanged(const QString&)), this,
          SLOT(updateFieldsWithOptSelection(const QString&)));
  // Features
  connect(ui.push_addFeatures, SIGNAL(clicked()), this, SLOT(addFeatures()));
  connect(ui.push_removeFeatures, SIGNAL(clicked()), this, SLOT(removeFeatures()));
  connect(ui.cb_redo_features, SIGNAL(toggled(bool)), this,
          SLOT(updateOptimizationInfo()));

  initialize();
}

TabMo::~TabMo()
{
}

void TabMo::writeSettings(const QString& filename)
{
}

void TabMo::readSettings(const QString& filename)
{
  updateGUI();
}

void TabMo::setActiveButtonAdd()
{
  ui.push_addFeatures->setDefault(true);
}

void TabMo::setActiveButtonRemove()
{
  ui.push_removeFeatures->setDefault(true);
}

void TabMo::updateFieldsWithOptSelection(QString value_type)
{
  if (value_type == "Hardness:AFLOW-ML")
  {
    ui.line_path->setText("N/A");
    ui.line_output->setText("N/A");
    ui.line_path->setDisabled(true);
    ui.line_output->setDisabled(true);
  }
  else
  {
    ui.line_path->setDisabled(false);
    ui.line_output->setDisabled(false);
  }
}

void TabMo::updateGUI()
{
  m_updateGuiInProgress = true;
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Features
  bool wasBlocked = ui.cb_redo_features->blockSignals(true);
  ui.cb_redo_features->setChecked(xtalopt->m_featuresReDo);
  ui.cb_redo_features->blockSignals(wasBlocked);

  // Initiate the features table
  updateFeaturesTable();

  m_updateGuiInProgress = false;
}

void TabMo::lockGUI()
{
  ui.combo_type->setDisabled(true);
  ui.sb_weight->setDisabled(true);
  ui.line_path->setDisabled(true);
  ui.line_output->setDisabled(true);
  ui.push_addFeatures->setDisabled(true);
  ui.push_removeFeatures->setDisabled(true);
  ui.table_features->setDisabled(true);
  //ui.cb_redo_features->setDisabled(true);
}

bool TabMo::updateOptimizationInfo()
{
  if (m_updateGuiInProgress)
    return true;

  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Features/hardness; the processFeature... is called here
  //   at least once to properly initialize weights for hardness and number of features.
  xtalopt->m_featuresReDo = ui.cb_redo_features->isChecked();
  bool ret = xtalopt->processFeaturesInfo();
  updateFeaturesTable();

  return ret;
}

void TabMo::addFeatures()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // A copy: used for restoring table if anything goes wrong!
  QStringList tmpfeatureslst;
  for(int i = 0; i < xtalopt->featureListSize(); i++)
   tmpfeatureslst.push_back(xtalopt->featureListGet(i));

  QString value_type = ui.combo_type->currentText();
  QString value_path = ui.line_path->text();
  QString value_outf = ui.line_output->text();
  QString value_wegt = QString::number(ui.sb_weight->value());

  if (value_path.split(" ", QString::SkipEmptyParts).size() != 1
      || value_outf.split(" ", QString::SkipEmptyParts).size() != 1)
  {
    errorPromptWindow("Invalid user-defined script/output file name!");
    return;
  }

  QString ftxt = value_type + " " + value_path + " " + value_outf + " " + value_wegt;

  xtalopt->featureListAdd(ftxt);

  // If anything goes wrong with feature/weights; restore last ok info!
  if (!updateOptimizationInfo())
  {
    errorPromptWindow("Total weight can't exceed 1.0!");
    xtalopt->featureListClear();
    for (int i = 0; i < tmpfeatureslst.size(); i++)
      xtalopt->featureListAdd(tmpfeatureslst.at(i));
    updateOptimizationInfo();
  }

  // Clean up the entry fields in "Add Feature" after adding a feature
  //ui.line_path->setText("/path_to/script");
  //ui.line_output->setText("/path_to/output_file");
  ui.line_output->clear();
  ui.line_path->clear();
  ui.sb_weight->setValue(0.0);
  ui.combo_type->setCurrentIndex(0);
}

void TabMo::removeFeatures()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // A copy: used for restoring table if anything goes wrong!
  QStringList tmpfeatureslst;
  for(int i = 0; i < xtalopt->featureListSize(); i++)
    tmpfeatureslst.push_back(xtalopt->featureListGet(i));

  int row = ui.table_features->currentRow();
  int tot = ui.table_features->rowCount();

  if (tot == 0 || row < 0 || row > tot)
    return;

  xtalopt->featureListRemove(row);

  // If anything goes wrong with feature/weights; restore last ok info!
  if (!updateOptimizationInfo())
  {
    errorPromptWindow("Total weight can't exceed 1.0!");
    xtalopt->featureListClear();
    for (int i = 0; i < tmpfeatureslst.size(); i++)
      xtalopt->featureListAdd(tmpfeatureslst.at(i));
    updateOptimizationInfo();
  }
}

void TabMo::updateFeaturesTable()
{
  XtalOpt* xtalopt = qobject_cast<XtalOpt*>(m_opt);

  // Adjust table size:
  int numRows = xtalopt->featureListSize();

  ui.table_features->setRowCount(numRows);

  for (int i = 0; i < numRows; i++) {
    QStringList fline = xtalopt->featureListGet(i).split(" ", QString::SkipEmptyParts);
    QTableWidgetItem* value_type = new QTableWidgetItem(fline[0]);
    QTableWidgetItem* value_path = new QTableWidgetItem(fline[1]);
    QTableWidgetItem* value_outf = new QTableWidgetItem(fline[2]);
    QTableWidgetItem* value_wegt = new QTableWidgetItem(fline[3]);

    ui.table_features->setItem(i, FC_TYPE, value_type);
    ui.table_features->setItem(i, FC_PATH, value_path);
    ui.table_features->setItem(i, FC_OUTPUT, value_outf);
    ui.table_features->setItem(i, FC_WEIGHT, value_wegt);
  }
}

void TabMo::errorPromptWindow(const QString& instr)
{
  QMessageBox msgBox;
  msgBox.setText(instr);
  msgBox.exec();
}
}
