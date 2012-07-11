/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center, 
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without 
even the implied warranty of MERCHANTABILITY or FITNESS FOR 
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/


// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>
#include <berryIPreferences.h>
#include <berryIPreferencesService.h>
#include <berryIBerryPreferences.h>

// Qmitk
#include "CommandLineModulesView.h"
#include "CommandLineModulesPreferencesPage.h"

// Qt
#include <QMessageBox>
#include <QScrollArea>
#include <QFile>
#include <QFileDialog>
#include <QFileInfoList>
#include <QDir>
#include <QBuffer>
#include <QtUiTools/QUiLoader>
#include <QByteArray>
#include <QHBoxLayout>
#include <QAction>

// CTK
#include <ctkCmdLineModuleManager.h>
#include <ctkCmdLineModuleInstance.h>
#include <ctkCmdLineModuleDescription.h>
#include <ctkCmdLineModuleInstanceFactoryQtGui.h>
#include <ctkCmdLineModuleXmlValidator.h>
#include <ctkCmdLineModuleProcessFuture.h>

const std::string CommandLineModulesView::VIEW_ID = "org.mitk.gui.qt.cli";

CommandLineModulesView::CommandLineModulesView()
: m_Controls(NULL)
, m_Parent(NULL)
, m_ModuleManager(NULL)
, m_TemporaryDirectoryName("")
{
  m_ModulesDirectoryNames.clear();
  m_MapFilenameToReference.clear();
  m_MapTabToModuleInstance.clear();
  m_ModuleManager = new ctkCmdLineModuleManager(new ctkCmdLineModuleInstanceFactoryQtGui());
}

CommandLineModulesView::~CommandLineModulesView()
{
  if (m_ModuleManager != NULL)
  {
    delete m_ModuleManager;
  }
}

void CommandLineModulesView::SetFocus()
{
  this->m_Controls->m_ComboBox->setFocus();
}

void CommandLineModulesView::CreateQtPartControl( QWidget *parent )
{
  if (!m_Controls)
  {
    // We create CommandLineModulesViewControls, which derives from the Qt generated class.
    m_Controls = new CommandLineModulesViewControls(parent);

    // Loads the preferences like directory settings into member variables.
    this->RetrievePreferenceValues();

    // Connect signals to slots after we have set up GUI.
    connect(this->m_Controls->m_RunButton, SIGNAL(pressed()), this, SLOT(OnRunButtonPressed()));
    connect(this->m_Controls->m_StopButton, SIGNAL(pressed()), this, SLOT(OnStopButtonPressed()));
    connect(this->m_Controls->m_RestoreDefaults, SIGNAL(pressed()), this, SLOT(OnRestoreDefaultsButtonPressed()));
    connect(this->m_Controls->m_ComboBox, SIGNAL(actionChanged(QAction*)), this, SLOT(OnActionChanged(QAction*)));
    connect(&(this->m_FutureWatcher), SIGNAL(finished()), this, SLOT(OnFutureFinished()));
  }
}

void CommandLineModulesView::RetrievePreferenceValues()
{
  berry::IPreferencesService::Pointer prefService
      = berry::Platform::GetServiceRegistry()
      .GetServiceById<berry::IPreferencesService>(berry::IPreferencesService::ID);

  assert( prefService );

  std::string id = "/" + CommandLineModulesView::VIEW_ID;
  berry::IBerryPreferences::Pointer prefs
      = (prefService->GetSystemPreferences()->Node(id))
        .Cast<berry::IBerryPreferences>();

  assert( prefs );

  m_TemporaryDirectoryName = QString::fromStdString(prefs->Get(CommandLineModulesPreferencesPage::TEMPORARY_DIRECTORY_NODE_NAME, ""));
  QString modulesPath = QString::fromStdString(prefs->Get(CommandLineModulesPreferencesPage::MODULES_DIRECTORY_NODE_NAME, ""));
  QStringList paths;
  paths << "/scratch0/NOT_BACKED_UP/clarkson/build/CTK-build/CTK-build/bin";

  // OnPreferencesChanged can be called for each preference in a dialog box, so
  // when you hit "OK", it could be called repeatedly.
  if (paths != this->m_ModulesDirectoryNames)
  {
    m_MapFilenameToReference = this->LoadModuleReferencesFromPaths(paths);
    QMenu *menu = this->CreateMenuFromReferences(m_MapFilenameToReference);
    this->m_Controls->m_ComboBox->setMenu(menu);
    this->m_ModulesDirectoryNames = paths;
  }
}

QHash<QString, ctkCmdLineModuleReference> CommandLineModulesView::LoadModuleReferencesFromPaths(QStringList &paths)
{
  QString path;
  QString executable;
  QFileInfo executableFileInfo;
  QHash<QString, ctkCmdLineModuleReference> result;

  foreach (path, paths)
  {
    if (!path.isNull() && !path.isEmpty() && !path.trimmed().isEmpty())
    {
      QDir directory = QDir(path);
      directory.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Executable);

      QFileInfoList executablesInfoList = directory.entryInfoList();
      foreach (executableFileInfo, executablesInfoList)
      {
        executable = executableFileInfo.absoluteFilePath();
        ctkCmdLineModuleReference ref = m_ModuleManager->registerModule(executable, true);
        if (ref)
        {
          result[executable] = ref;
        }
      }
    }
  }
  return result;
}

QMenu* CommandLineModulesView::CreateMenuFromReferences(QHash<QString, ctkCmdLineModuleReference>& hashMap)
{
  QMenu *menu = new QMenu();
  ctkCmdLineModuleReference ref;

  QList<ctkCmdLineModuleReference> references = hashMap.values();
  foreach (ref, references)
  {
    menu->addAction(ref.description().title());
  }
  return menu;
}

void CommandLineModulesView::OnPreferencesChanged(const berry::IBerryPreferences* /*prefs*/)
{
  // Loads the preferences into member variables.
  this->RetrievePreferenceValues();
}

void CommandLineModulesView::AddModuleTab(const ctkCmdLineModuleReference& moduleRef)
{
  ctkCmdLineModuleInstance* moduleInstance = m_ModuleManager->createModuleInstance(moduleRef);
  if (!moduleInstance) return;

  QObject* guiHandle = moduleInstance->guiHandle();
  QWidget* widget = qobject_cast<QWidget*>(guiHandle);

  int tabIndex = m_Controls->m_TabWidget->addTab(widget, moduleRef.description().title());
  m_Controls->m_TabWidget->setCurrentIndex(tabIndex);
  m_MapTabToModuleInstance[tabIndex] = moduleInstance;

  m_Controls->m_HelpBrowser->clear();
  m_Controls->m_HelpBrowser->setPlainText(moduleRef.description().description());
  m_Controls->m_AboutBrowser->clear();
  m_Controls->m_AboutBrowser->setPlainText(moduleRef.description().acknowledgements());
}

void CommandLineModulesView::OnFutureFinished()
{
  qDebug() << "*** Future finished ***";
  qDebug() << "stdout:" << m_FutureWatcher.future().standardOutput();
  qDebug() << "stderr:" << m_FutureWatcher.future().standardError();
}

void CommandLineModulesView::OnActionChanged(QAction* action)
{
  ctkCmdLineModuleReference ref = this->GetReferenceByIdentifier(action->text());
  if (ref)
  {
    this->AddModuleTab(ref);
  }
}

ctkCmdLineModuleReference CommandLineModulesView::GetReferenceByIdentifier(QString identifier)
{
  ctkCmdLineModuleReference result;

  QList<ctkCmdLineModuleReference> references = m_MapFilenameToReference.values();
  ctkCmdLineModuleReference ref;
  foreach(ref, references)
  {
    if (ref.description().title() == identifier)
    {
      result = ref;
    }
  }
  return result;
}

void CommandLineModulesView::OnRunButtonPressed()
{
  qDebug() << "Creating module command line...";

  ctkCmdLineModuleInstance* moduleInstance = m_MapTabToModuleInstance[m_Controls->m_TabWidget->currentIndex()];
  if (!moduleInstance)
  {
    qWarning() << "Invalid module instance";
    return;
  }

  qDebug() << "Launching module command line...";

  ctkCmdLineModuleProcessFuture future = moduleInstance->run();

  qDebug() << "Launched module command line...";
}

void CommandLineModulesView::OnStopButtonPressed()
{
  qDebug() << "Stopping module command line...";


  qDebug() << "Stopped module command line...";
}

void CommandLineModulesView::OnRestoreDefaultsButtonPressed()
{

}
