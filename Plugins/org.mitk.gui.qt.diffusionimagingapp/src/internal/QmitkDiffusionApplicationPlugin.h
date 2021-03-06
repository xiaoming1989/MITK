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


#ifndef QMITKDIFFUSIONAPPLICATIONPLUGIN_H_
#define QMITKDIFFUSIONAPPLICATIONPLUGIN_H_

#include <berryAbstractUICTKPlugin.h>

#include <QString>

#include <berryQCHPluginListener.h>

class QmitkDiffusionApplicationPlugin : public QObject, public berry::AbstractUICTKPlugin
{
  Q_OBJECT
  Q_INTERFACES(ctkPluginActivator)

public:

  QmitkDiffusionApplicationPlugin();
  ~QmitkDiffusionApplicationPlugin();

  static QmitkDiffusionApplicationPlugin* GetDefault();

  ctkPluginContext* GetPluginContext() const;

  void start(ctkPluginContext*);

  QString GetQtHelpCollectionFile() const;

private:

  static QmitkDiffusionApplicationPlugin* inst;

  ctkPluginContext* context;
};

#endif /* QMITKDIFFUSIONAPPLICATIONPLUGIN_H_ */
