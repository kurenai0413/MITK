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

#include "QmitkMeasurementView.h"

#include <QtGui>

#include <mitkVtkLayerController.h>
#include <mitkWeakPointer.h>
#include <mitkPlanarCircle.h>
#include <mitkPlanarPolygon.h>
#include <mitkPlanarAngle.h>
#include <mitkPlanarRectangle.h>
#include <mitkPlanarLine.h>
#include <mitkPlanarCross.h>
#include <mitkPlanarFourPointAngle.h>
#include <mitkPlanarFigureInteractor.h>
#include <mitkPlaneGeometry.h>
#include <mitkGlobalInteraction.h>
#include <mitkILinkedRenderWindowPart.h>
#include <mitkNodePredicateDataType.h>

struct QmitkPlanarFigureData
{
  QmitkPlanarFigureData()
    : m_Figure(0), m_EndPlacementObserverTag(0), m_SelectObserverTag(0), m_StartInteractionObserverTag(0), m_EndInteractionObserverTag(0)
  {
  }

  mitk::PlanarFigure* m_Figure;
  unsigned int m_EndPlacementObserverTag;
  unsigned int m_SelectObserverTag;
  unsigned int m_StartInteractionObserverTag;
  unsigned int m_EndInteractionObserverTag;
};

struct QmitkMeasurementViewData
{
  QmitkMeasurementViewData()
    : m_LineCounter(0), m_PathCounter(0), m_AngleCounter(0),
      m_FourPointAngleCounter(0), m_EllipseCounter(0),
      m_RectangleCounter(0), m_PolygonCounter(0)
  {
  }

  // internal vars
  unsigned int m_LineCounter;
  unsigned int m_PathCounter;
  unsigned int m_AngleCounter;
  unsigned int m_FourPointAngleCounter;
  unsigned int m_EllipseCounter;
  unsigned int m_RectangleCounter;
  unsigned int m_PolygonCounter;
  QList<mitk::DataNode::Pointer> m_CurrentSelection;
  std::map<mitk::DataNode*, QmitkPlanarFigureData> m_DataNodeToPlanarFigureData;
  mitk::WeakPointer<mitk::DataNode> m_SelectedImageNode;

  // WIDGETS
  QWidget* m_Parent;
  QLabel* m_SelectedImageLabel;
  QAction* m_DrawLine;
  QAction* m_DrawPath;
  QAction* m_DrawAngle;
  QAction* m_DrawFourPointAngle;
  QAction* m_DrawEllipse;
  QAction* m_DrawRectangle;
  QAction* m_DrawPolygon;
  QToolBar* m_DrawActionsToolBar;
  QActionGroup* m_DrawActionsGroup;
  QTextBrowser* m_SelectedPlanarFiguresText;
  QPushButton* m_CopyToClipboard;
  QGridLayout* m_Layout;
};

const std::string QmitkMeasurementView::VIEW_ID = "org.mitk.views.measurement";

QmitkMeasurementView::QmitkMeasurementView()
: d( new QmitkMeasurementViewData )
{
}
QmitkMeasurementView::~QmitkMeasurementView()
{
  this->RemoveAllInteractors();
  delete d;
}
void QmitkMeasurementView::CreateQtPartControl(QWidget* parent)
{
  d->m_Parent = parent;

  // image label
  QLabel* selectedImageLabel = new QLabel("Reference Image: ");
  d->m_SelectedImageLabel = new QLabel;
  d->m_SelectedImageLabel->setStyleSheet("font-weight: bold;");

  d->m_DrawActionsToolBar = new QToolBar;
  d->m_DrawActionsGroup = new QActionGroup(this);
  d->m_DrawActionsGroup->setExclusive(true);

  //# add actions
  QAction* currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/line.png"), "Draw Line");
  d->m_DrawLine = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);

  currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/path.png"), "Draw Path");
  d->m_DrawPath = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);
  currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/angle.png"), "Draw Angle");
  d->m_DrawAngle = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);

  currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/four-point-angle.png"), "Draw Four Point Angle");
  d->m_DrawFourPointAngle = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);

  currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/circle.png"), "Draw Circle");
  d->m_DrawEllipse = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);

  currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/rectangle.png"), "Draw Rectangle");
  d->m_DrawRectangle = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);

  currentAction = d->m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/polygon.png"), "Draw Polygon");
  d->m_DrawPolygon = currentAction;
  d->m_DrawActionsToolBar->addAction(currentAction);
  d->m_DrawActionsGroup->addAction(currentAction);

  // planar figure details text
  d->m_SelectedPlanarFiguresText = new QTextBrowser;

  // copy to clipboard button
  d->m_CopyToClipboard = new QPushButton("Copy to Clipboard");

  d->m_Layout = new QGridLayout;
  d->m_Layout->addWidget(selectedImageLabel, 0, 0, 1, 1);
  d->m_Layout->addWidget(d->m_SelectedImageLabel, 0, 1, 1, 1);
  d->m_Layout->addWidget(d->m_DrawActionsToolBar, 1, 0, 1, 2);
  d->m_Layout->addWidget(d->m_SelectedPlanarFiguresText, 2, 0, 1, 2);
  d->m_Layout->addWidget(d->m_CopyToClipboard, 3, 0, 1, 2);

  d->m_Parent->setLayout(d->m_Layout);

  // create connections
  this->CreateConnections();

  // readd interactors and observers
  this->AddAllInteractors();
}
void QmitkMeasurementView::CreateConnections()
{
  QObject::connect( d->m_DrawLine, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawLineTriggered(bool) ) );
  QObject::connect( d->m_DrawPath, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawPathTriggered(bool) ) );
  QObject::connect( d->m_DrawAngle, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawAngleTriggered(bool) ) );
  QObject::connect( d->m_DrawFourPointAngle, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawFourPointAngleTriggered(bool) ) );
  QObject::connect( d->m_DrawEllipse, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawEllipseTriggered(bool) ) );
  QObject::connect( d->m_DrawRectangle, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawRectangleTriggered(bool) ) );
  QObject::connect( d->m_DrawPolygon, SIGNAL( triggered(bool) )
  , this, SLOT( ActionDrawPolygonTriggered(bool) ) );
  QObject::connect( d->m_CopyToClipboard, SIGNAL( clicked(bool) )
  , this, SLOT( CopyToClipboard(bool) ) );
}

void QmitkMeasurementView::NodeAdded( const mitk::DataNode* node )
{
  // add observer for selection in renderwindow
  mitk::PlanarFigure* figure = dynamic_cast<mitk::PlanarFigure*>(node->GetData());
  if( figure )
  {
    MITK_DEBUG << "figure added. will add interactor if needed.";
    mitk::PlanarFigureInteractor::Pointer figureInteractor
        = dynamic_cast<mitk::PlanarFigureInteractor*>(node->GetInteractor());

    mitk::DataNode* nonConstNode = const_cast<mitk::DataNode*>( node );
    if(figureInteractor.IsNull())
    {
      figureInteractor = mitk::PlanarFigureInteractor::New("PlanarFigureInteractor", nonConstNode);
    }
    else
    {
      // just to be sure that the interactor is not added twice
      mitk::GlobalInteraction::GetInstance()->RemoveInteractor(figureInteractor);
    }

    MITK_DEBUG << "adding interactor to globalinteraction";
    mitk::GlobalInteraction::GetInstance()->AddInteractor(figureInteractor);

    MITK_DEBUG << "will now add observers for planarfigure";
    QmitkPlanarFigureData data;
    data.m_Figure = figure;

    // add observer for event when figure has been placed
    typedef itk::SimpleMemberCommand< QmitkMeasurementView > SimpleCommandType;
    SimpleCommandType::Pointer initializationCommand = SimpleCommandType::New();
    initializationCommand->SetCallbackFunction( this, &QmitkMeasurementView::PlanarFigureInitialized );
    data.m_EndPlacementObserverTag = figure->AddObserver( mitk::EndPlacementPlanarFigureEvent(), initializationCommand );

    // add observer for event when figure is picked (selected)
    typedef itk::MemberCommand< QmitkMeasurementView > MemberCommandType;
    MemberCommandType::Pointer selectCommand = MemberCommandType::New();
    selectCommand->SetCallbackFunction( this, &QmitkMeasurementView::PlanarFigureSelected );
    data.m_SelectObserverTag = figure->AddObserver( mitk::SelectPlanarFigureEvent(), selectCommand );

    // add observer for event when interaction with figure starts
    SimpleCommandType::Pointer startInteractionCommand = SimpleCommandType::New();
    startInteractionCommand->SetCallbackFunction( this, &QmitkMeasurementView::DisableCrosshairNavigation);
    data.m_StartInteractionObserverTag = figure->AddObserver( mitk::StartInteractionPlanarFigureEvent(), startInteractionCommand );

    // add observer for event when interaction with figure starts
    SimpleCommandType::Pointer endInteractionCommand = SimpleCommandType::New();
    endInteractionCommand->SetCallbackFunction( this, &QmitkMeasurementView::EnableCrosshairNavigation);
    data.m_EndInteractionObserverTag = figure->AddObserver( mitk::EndInteractionPlanarFigureEvent(), endInteractionCommand );

    // adding to the map of tracked planarfigures
    d->m_DataNodeToPlanarFigureData[nonConstNode] = data;
  }
  this->CheckForTopMostVisibleImage();
}

void QmitkMeasurementView::NodeChanged(const mitk::DataNode* node)
{
  // DETERMINE IF WE HAVE TO RENEW OUR DETAILS TEXT (ANY NODE CHANGED IN OUR SELECTION?)
  bool renewText = false;
  for( int i=0; i < d->m_CurrentSelection.size(); ++i )
  {
    if( node == d->m_CurrentSelection.at(i) )
    {
      renewText = true;
      break;
    }
  }

  if(renewText)
  {
    MITK_DEBUG << "Selected nodes changed. Refreshing text.";
    this->UpdateMeasurementText();
  }

  this->CheckForTopMostVisibleImage();
}

void QmitkMeasurementView::CheckForTopMostVisibleImage()
{
  d->m_SelectedImageNode = this->DetectTopMostVisibleImage().GetPointer();

  if( d->m_SelectedImageNode.IsNotNull() )
  {
    MITK_DEBUG << "Reference image found";
    d->m_SelectedImageLabel->setText( QString::fromStdString( d->m_SelectedImageNode->GetName() ) );
    d->m_DrawActionsToolBar->setEnabled(true);
    MITK_DEBUG << "Updating Measurement text";
  }
  else
  {
    MITK_DEBUG << "No reference image available. Will disable actions for creating new planarfigures";
    d->m_SelectedImageLabel->setText( "No visible image available." );
    d->m_DrawActionsToolBar->setEnabled(false);
  }
}

void QmitkMeasurementView::NodeRemoved(const mitk::DataNode* node)
{
  MITK_DEBUG << "node removed from data storage";

  mitk::DataNode* nonConstNode = const_cast<mitk::DataNode*>(node);
  std::map<mitk::DataNode*, QmitkPlanarFigureData>::iterator it =
      d->m_DataNodeToPlanarFigureData.find(nonConstNode);

  if( it != d->m_DataNodeToPlanarFigureData.end() )
  {
    QmitkPlanarFigureData& data = it->second;

    MITK_DEBUG << "removing figure interactor to globalinteraction";
    mitk::Interactor::Pointer oldInteractor = node->GetInteractor();
    if(oldInteractor.IsNotNull())
      mitk::GlobalInteraction::GetInstance()->RemoveInteractor(oldInteractor);

    // remove observers
    data.m_Figure->RemoveObserver( data.m_EndPlacementObserverTag );
    data.m_Figure->RemoveObserver( data.m_SelectObserverTag );
    data.m_Figure->RemoveObserver( data.m_StartInteractionObserverTag );
    data.m_Figure->RemoveObserver( data.m_EndInteractionObserverTag );

    MITK_DEBUG << "removing from the list of tracked planar figures";
    d->m_DataNodeToPlanarFigureData.erase( it );
  }

  this->CheckForTopMostVisibleImage();
}


void QmitkMeasurementView::PlanarFigureSelected( itk::Object* object, const itk::EventObject& )
{
  MITK_DEBUG << "planar figure " << object << " selected";
  std::map<mitk::DataNode*, QmitkPlanarFigureData>::iterator it =
      d->m_DataNodeToPlanarFigureData.begin();

  d->m_CurrentSelection.clear();
  while( it != d->m_DataNodeToPlanarFigureData.end())
  {
    mitk::DataNode* node = it->first;
    QmitkPlanarFigureData& data = it->second;

    if( data.m_Figure == object )
    {
      node->SetSelected(true);
      d->m_CurrentSelection.push_back( node );
    }
    else
    {
      node->SetSelected(false);
    }

    ++it;
  }
  this->UpdateMeasurementText();
}

void QmitkMeasurementView::PlanarFigureInitialized()
{
  MITK_DEBUG << "planar figure initialized";
}

void QmitkMeasurementView::SetFocus()
{
  d->m_SelectedImageLabel->setFocus();
}

void QmitkMeasurementView::OnSelectionChanged(berry::IWorkbenchPart::Pointer part,
                                              const QList<mitk::DataNode::Pointer> &nodes)
{
  MITK_DEBUG << "Determine the top most visible image";
  MITK_DEBUG << "The PlanarFigure interactor will take the currently visible PlaneGeometry from the slice navigation controller";

  this->CheckForTopMostVisibleImage();

  MITK_DEBUG << "refreshing selection and detailed text";
  d->m_CurrentSelection = nodes;
  this->UpdateMeasurementText();
}

void QmitkMeasurementView::ActionDrawLineTriggered(bool checked)
{
  mitk::PlanarLine::Pointer figure = mitk::PlanarLine::New();
  QString qString = QString("Line%1").arg(++d->m_LineCounter);
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarLine initialized...";
}

void QmitkMeasurementView::ActionDrawPathTriggered(bool checked)
{
  mitk::PlanarPolygon::Pointer figure = mitk::PlanarPolygon::New();
  figure->ClosedOff();
  QString qString = QString("Path%1").arg(++d->m_PathCounter);
  mitk::DataNode::Pointer node = this->AddFigureToDataStorage(figure, qString);
  mitk::BoolProperty::Pointer closedProperty = mitk::BoolProperty::New( false );
  node->SetProperty("ClosedPlanarPolygon", closedProperty);

  MITK_DEBUG << "PlanarPath initialized...";
}

void QmitkMeasurementView::ActionDrawAngleTriggered(bool checked)
{
  mitk::PlanarAngle::Pointer figure = mitk::PlanarAngle::New();
  QString qString = QString("Angle%1").arg(++d->m_AngleCounter);
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarAngle initialized...";
}

void QmitkMeasurementView::ActionDrawFourPointAngleTriggered(bool checked)
{
  mitk::PlanarFourPointAngle::Pointer figure =
    mitk::PlanarFourPointAngle::New();
  QString qString = QString("Four Point Angle%1").arg(++d->m_FourPointAngleCounter);
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarFourPointAngle initialized...";
}

void QmitkMeasurementView::ActionDrawEllipseTriggered(bool checked)
{
  mitk::PlanarCircle::Pointer figure = mitk::PlanarCircle::New();
  QString qString = QString("Circle%1").arg(++d->m_EllipseCounter);
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarCircle initialized...";
}

void QmitkMeasurementView::ActionDrawRectangleTriggered(bool checked)
{
  mitk::PlanarRectangle::Pointer figure = mitk::PlanarRectangle::New();
  QString qString = QString("Rectangle%1").arg(++d->m_RectangleCounter);
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarRectangle initialized...";
}

void QmitkMeasurementView::ActionDrawPolygonTriggered(bool checked)
{
  mitk::PlanarPolygon::Pointer figure = mitk::PlanarPolygon::New();
  figure->ClosedOn();
  QString qString = QString("Polygon%1").arg(++d->m_PolygonCounter);
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarPolygon initialized...";
}

void QmitkMeasurementView::CopyToClipboard( bool checked )
{
  MITK_DEBUG << "Copying current Text to clipboard...";
  QString clipboardText = d->m_SelectedPlanarFiguresText->toPlainText();
  QApplication::clipboard()->setText(clipboardText, QClipboard::Clipboard);
}

mitk::DataNode::Pointer QmitkMeasurementView::AddFigureToDataStorage(
  mitk::PlanarFigure* figure, const QString& name)
{
  // add as
  MITK_DEBUG << "Adding new figure to datastorage...";
  if( d->m_SelectedImageNode.IsNull() )
  {
    MITK_ERROR << "No reference image available";
    return 0;
  }

  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetName(name.toStdString());
  newNode->SetData(figure);
  // set as selected
  // newNode->SetSelected( true );
  this->GetDataStorage()->Add(newNode, d->m_SelectedImageNode);
  return newNode;
}

void QmitkMeasurementView::UpdateMeasurementText()
{
  d->m_SelectedPlanarFiguresText->clear();

  QString infoText;
  QString plainInfoText;
  unsigned int j = 1;
  mitk::PlanarFigure* _PlanarFigure = 0;
  mitk::PlanarAngle* planarAngle = 0;
  mitk::PlanarFourPointAngle* planarFourPointAngle = 0;
  mitk::DataNode::Pointer node = 0;

  for (unsigned int i=0; i<d->m_CurrentSelection.size(); ++i, ++j)
  {
    plainInfoText.clear();
    node = d->m_CurrentSelection.at(i);
    _PlanarFigure = dynamic_cast<mitk::PlanarFigure*> (node->GetData());

    if( !_PlanarFigure )
      continue;

    if(j>1)
      infoText.append("<br />");

    infoText.append(QString("<b>%1</b><hr />").arg(QString::fromStdString(
      node->GetName())));
    plainInfoText.append(QString("%1").arg(QString::fromStdString(
      node->GetName())));

    planarAngle = dynamic_cast<mitk::PlanarAngle*> (_PlanarFigure);
    if(!planarAngle)
    {
      planarFourPointAngle = dynamic_cast<mitk::PlanarFourPointAngle*> (_PlanarFigure);
    }

    double featureQuantity = 0.0;
    for (unsigned int k = 0; k < _PlanarFigure->GetNumberOfFeatures(); ++k)
    {
      if ( !_PlanarFigure->IsFeatureActive( k ) ) continue;

      featureQuantity = _PlanarFigure->GetQuantity(k);
      if ((planarAngle && k == planarAngle->FEATURE_ID_ANGLE)
        || (planarFourPointAngle && k == planarFourPointAngle->FEATURE_ID_ANGLE))
        featureQuantity = featureQuantity * 180 / vnl_math::pi;

      infoText.append(
        QString("<i>%1</i>: %2 %3") .arg(QString(
        _PlanarFigure->GetFeatureName(k))) .arg(featureQuantity, 0, 'f',
        2) .arg(QString(_PlanarFigure->GetFeatureUnit(k))));

      plainInfoText.append(
        QString("\n%1: %2 %3") .arg(QString(_PlanarFigure->GetFeatureName(k))) .arg(
        featureQuantity, 0, 'f', 2) .arg(QString(
        _PlanarFigure->GetFeatureUnit(k))));

      if(k+1 != _PlanarFigure->GetNumberOfFeatures())
        infoText.append("<br />");
    }

    if (j != d->m_CurrentSelection.size())
      infoText.append("<br />");
  }

  d->m_SelectedPlanarFiguresText->setHtml(infoText);
}

void QmitkMeasurementView::AddAllInteractors()
{
  MITK_DEBUG << "Adding interactors to all planar figures";

  mitk::DataStorage::SetOfObjects::ConstPointer _NodeSet = this->GetDataStorage()->GetAll();
  const mitk::DataNode* node = 0;

  for(mitk::DataStorage::SetOfObjects::ConstIterator it=_NodeSet->Begin(); it!=_NodeSet->End()
    ; it++)
  {
    node = const_cast<mitk::DataNode*>(it->Value().GetPointer());
    this->NodeAdded( node );
  }
}

void QmitkMeasurementView::RemoveAllInteractors()
{
  MITK_DEBUG << "Removing interactors and observers from all planar figures";

  mitk::DataStorage::SetOfObjects::ConstPointer _NodeSet = this->GetDataStorage()->GetAll();
  const mitk::DataNode* node = 0;

  for(mitk::DataStorage::SetOfObjects::ConstIterator it=_NodeSet->Begin(); it!=_NodeSet->End()
    ; it++)
  {
    node = const_cast<mitk::DataNode*>(it->Value().GetPointer());
    this->NodeRemoved( node );
  }
}

mitk::DataNode::Pointer QmitkMeasurementView::DetectTopMostVisibleImage()
{
  // get all images from the data storage
  mitk::DataStorage::SetOfObjects::ConstPointer Images
      = this->GetDataStorage()->GetSubset( mitk::NodePredicateDataType::New("Image") );

  mitk::DataNode::Pointer currentNode;
  int maxLayer = itk::NumericTraits<int>::min();

  // iterate over selection
  for (mitk::DataStorage::SetOfObjects::ConstIterator sofIt = Images->Begin(); sofIt != Images->End(); ++sofIt)
  {
    mitk::DataNode::Pointer node = sofIt->Value();
    if ( node.IsNull() )
      continue;
    if (node->IsVisible(NULL) == false)
      continue;
    int layer = 0;
    node->GetIntProperty("layer", layer);
    if ( layer < maxLayer )
      continue;

    currentNode = node;
  }

  return currentNode;
}

void QmitkMeasurementView::EnableCrosshairNavigation()
{
  // enable the crosshair navigation
  if (mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart()))
  {
    linkedRenderWindow->EnableLinkedNavigation(true);
  }
}

void QmitkMeasurementView::DisableCrosshairNavigation()
{
  // disable the crosshair navigation during the drawing
  if (mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart()))
  {
    linkedRenderWindow->EnableLinkedNavigation(false);
  }
}

/*
#include <berryIEditorPart.h>
#include <berryIWorkbenchPage.h>
#include <berryPlatform.h>

#include "mitkGlobalInteraction.h"
#include "mitkPointSet.h"
#include "mitkProperties.h"
#include "mitkStringProperty.h"
#include "mitkIDataStorageService.h"
#include "mitkDataNodeObject.h"
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateData.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateAnd.h>
#include <mitkDataNodeSelection.h>
#include <mitkILinkedRenderWindowPart.h>
#include <mitkIRenderingManager.h>

#include "mitkPlanarCircle.h"
#include "mitkPlanarPolygon.h"
#include "mitkPlanarAngle.h"
#include "mitkPlanarRectangle.h"
#include "mitkPlanarLine.h"
#include "mitkPlanarCross.h"
#include "mitkPlanarFourPointAngle.h"
#include "mitkPlanarFigureInteractor.h"
#include "mitkPlaneGeometry.h"
#include "QmitkPlanarFiguresTableModel.h"

#include <QmitkRenderWindow.h>

#include <QApplication>
#include <QGridLayout>
#include <QMainWindow>
#include <QToolBar>
#include <QLabel>
#include <QTableView>
#include <QClipboard>
#include <QTextBrowser>

#include <QmitkDataStorageTableModel.h>
#include "mitkNodePredicateDataType.h"
#include "mitkPlanarFigure.h"

#include <vtkTextProperty.h>
#include <vtkRenderer.h>
#include <vtkCornerAnnotation.h>
#include <mitkVtkLayerController.h>

const std::string QmitkMeasurementView::VIEW_ID =
"org.mitk.views.measurement";

QmitkMeasurementView::QmitkMeasurementView() :
m_Parent(0), m_Layout(0), m_DrawActionsToolBar(0),
m_DrawActionsGroup(0), m_MeasurementInfoRenderer(0),
m_MeasurementInfoAnnotation(0), m_SelectedPlanarFigures(0),
m_SelectedImageNode(),
m_LineCounter(0), m_PathCounter(0),
m_AngleCounter(0), m_FourPointAngleCounter(0), m_EllipseCounter(0),
  m_RectangleCounter(0), m_PolygonCounter(0), m_Visible(false),
  m_CurrentFigureNodeInitialized(false), m_Activated(false),
  m_LastRenderWindow(0)
{

}

QmitkMeasurementView::~QmitkMeasurementView()
{
  this->GetDataStorage()->AddNodeEvent -= mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeAddedInDataStorage );

  m_SelectedPlanarFigures->NodeChanged.RemoveListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeChanged ) );

  m_SelectedPlanarFigures->NodeRemoved.RemoveListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeRemoved ) );

  m_SelectedPlanarFigures->PropertyChanged.RemoveListener( mitk::MessageDelegate2<QmitkMeasurementView
    , const mitk::DataNode*, const mitk::BaseProperty*>( this, &QmitkMeasurementView::PropertyChanged ) );

  m_SelectedImageNode->NodeChanged.RemoveListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeChanged ) );

  m_SelectedImageNode->NodeRemoved.RemoveListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeRemoved ) );

  m_SelectedImageNode->PropertyChanged.RemoveListener( mitk::MessageDelegate2<QmitkMeasurementView
    , const mitk::DataNode*, const mitk::BaseProperty*>( this, &QmitkMeasurementView::PropertyChanged ) );

  this->RemoveEndPlacementObserverTag();

  if(this->m_LastRenderWindow != NULL)
  {
    this->SetMeasurementInfoToRenderWindow("",m_LastRenderWindow);
    mitk::VtkLayerController::GetInstance(m_LastRenderWindow->GetRenderWindow())->RemoveRenderer(
      m_MeasurementInfoRenderer);
  }
  this->m_MeasurementInfoRenderer->Delete();
}

void QmitkMeasurementView::CreateQtPartControl(QWidget* parent)
{
  m_Parent = parent;
  m_MeasurementInfoRenderer = vtkRenderer::New();
  m_MeasurementInfoAnnotation = vtkCornerAnnotation::New();
  vtkTextProperty *textProp = vtkTextProperty::New();

  m_MeasurementInfoAnnotation->SetMaximumFontSize(12);
  textProp->SetColor(1.0, 1.0, 1.0);
  m_MeasurementInfoAnnotation->SetTextProperty(textProp);

  m_MeasurementInfoRenderer->AddActor(m_MeasurementInfoAnnotation);
  m_DrawActionsToolBar = new QToolBar;
  m_DrawActionsGroup = new QActionGroup(this);
  m_DrawActionsGroup->setExclusive(true);

  //# add actions
  QAction* currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/line.png"), "Draw Line");
  m_DrawLine = currentAction;
  m_DrawLine->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawLineTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/path.png"), "Draw Path");
  m_DrawPath = currentAction;
  m_DrawPath->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawPathTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/angle.png"), "Draw Angle");
  m_DrawAngle = currentAction;
  m_DrawAngle->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawAngleTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/four-point-angle.png"), "Draw Four Point Angle");
  m_DrawFourPointAngle = currentAction;
  m_DrawFourPointAngle->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawFourPointAngleTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/circle.png"), "Draw Circle");
  m_DrawEllipse = currentAction;
  m_DrawEllipse->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawEllipseTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/rectangle.png"), "Draw Rectangle");
  m_DrawRectangle = currentAction;
  m_DrawRectangle->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawRectangleTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/measurement/polygon.png"), "Draw Polygon");
  m_DrawPolygon = currentAction;
  m_DrawPolygon->setCheckable(true);
  m_DrawActionsToolBar->addAction(currentAction);
  m_DrawActionsGroup->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ActionDrawPolygonTriggered(bool) ) );

  currentAction = m_DrawActionsToolBar->addAction(QIcon(
    ":/Qmitk/Images_48.png"), "Reproduce potential bug");
  m_DrawActionsToolBar->addAction(currentAction);
  QObject::connect( currentAction, SIGNAL( triggered(bool) )
    , this, SLOT( ReproducePotentialBug(bool) ) );


  QLabel* selectedImageLabel = new QLabel("Selected Image: ");
  m_SelectedImage = new QLabel;
  m_SelectedImage->setStyleSheet("font-weight: bold;");
  m_SelectedPlanarFiguresText = new QTextBrowser;

  m_CopyToClipboard = new QPushButton("Copy to Clipboard");
  QObject::connect( m_CopyToClipboard, SIGNAL( clicked(bool) )
    , this, SLOT( CopyToClipboard(bool) ) );

  m_Layout = new QGridLayout;
  m_Layout->addWidget(selectedImageLabel, 0, 0, 1, 1);
  m_Layout->addWidget(m_SelectedImage, 0, 1, 1, 1);
  m_Layout->addWidget(m_DrawActionsToolBar, 1, 0, 1, 2);
  m_Layout->addWidget(m_SelectedPlanarFiguresText, 2, 0, 1, 2);
  m_Layout->addWidget(m_CopyToClipboard, 3, 0, 1, 2);
  m_Layout->setRowStretch(0, 1);
  m_Layout->setRowStretch(1, 1);
  m_Layout->setRowStretch(2, 10);
  m_Layout->setRowStretch(3, 1);
  m_Layout->setContentsMargins(2, 2, 2, 2);

  parent->setLayout(m_Layout);


  m_SelectedPlanarFigures = mitk::DataStorageSelection::New(this->GetDataStorage(), false);

  m_SelectedPlanarFigures->NodeChanged.AddListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeChanged ) );

  m_SelectedPlanarFigures->NodeRemoved.AddListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeRemoved ) );

  m_SelectedPlanarFigures->PropertyChanged.AddListener( mitk::MessageDelegate2<QmitkMeasurementView
    , const mitk::DataNode*, const mitk::BaseProperty*>( this, &QmitkMeasurementView::PropertyChanged ) );

  m_SelectedImageNode = mitk::DataStorageSelection::New(this->GetDataStorage(), false);

  m_SelectedImageNode->PropertyChanged.AddListener( mitk::MessageDelegate2<QmitkMeasurementView
    , const mitk::DataNode*, const mitk::BaseProperty*>( this, &QmitkMeasurementView::PropertyChanged ) );

  m_SelectedImageNode->NodeChanged.AddListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeChanged ) );

  m_SelectedImageNode->NodeRemoved.AddListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeRemoved ) );

  this->GetDataStorage()->AddNodeEvent.AddListener( mitk::MessageDelegate1<QmitkMeasurementView
    , const mitk::DataNode*>( this, &QmitkMeasurementView::NodeAddedInDataStorage ) );

}

void QmitkMeasurementView::OnSelectionChanged(berry::IWorkbenchPart::Pointer,
                                              const QList<mitk::DataNode::Pointer> &nodes)
{
  if ( nodes.isEmpty() ) return;

  m_SelectedImageNode->RemoveAllNodes();

  mitk::BaseData* _BaseData;
  mitk::PlanarFigure* _PlanarFigure;
  mitk::Image* selectedImage;
  m_SelectedPlanarFigures->RemoveAllNodes();

  foreach (mitk::DataNode::Pointer _DataNode, nodes)
  {
    _PlanarFigure = 0;

    if (!_DataNode)
      continue;

    _BaseData = _DataNode->GetData();

    if (!_BaseData)
      continue;

    // planar figure selected
    if ((_PlanarFigure = dynamic_cast<mitk::PlanarFigure *> (_BaseData)))
    {
      // add to the selected planar figures
      m_SelectedPlanarFigures->AddNode(_DataNode);
      // take parent image as the selected image
      mitk::DataStorage::SetOfObjects::ConstPointer parents =
        this->GetDataStorage()->GetSources(_DataNode);
      if (parents->size() > 0)
      {
        mitk::DataNode::Pointer parent = parents->front();
        if ((selectedImage = dynamic_cast<mitk::Image *> (parent->GetData())))
        {
          *m_SelectedImageNode = parent;
        }
      }

    }
    else if ((selectedImage = dynamic_cast<mitk::Image *> (_BaseData)))
    {
      *m_SelectedImageNode = _DataNode;
      //mitk::RenderingManager::GetInstance()->InitializeViews(
      //selectedImage->GetTimeSlicedGeometry(), mitk::RenderingManager::REQUEST_UPDATE_ALL, true );
    }
  } // end for

  this->PlanarFigureSelectionChanged();
}

void QmitkMeasurementView::PlanarFigureSelectionChanged()
{
  if ( !this->m_Activated ) return;

  if (m_SelectedImageNode->GetNode().IsNotNull())
  {
    mitk::Image* selectedImage = dynamic_cast<mitk::Image*>(m_SelectedImageNode->GetNode()->GetData());
    if(selectedImage && selectedImage->GetDimension() > 3)
    {
      m_SelectedImageNode->RemoveAllNodes();
      m_SelectedImage->setText( "4D images are not supported." );
      m_DrawActionsToolBar->setEnabled(false);
    }
    else
    {
      m_SelectedImage->setText(QString::fromStdString(
        m_SelectedImageNode->GetNode()->GetName()));
      m_DrawActionsToolBar->setEnabled(true);
    }
  }
  else
  {
    m_SelectedImage->setText(
      "None. Please select an image.");

    m_DrawActionsToolBar->setEnabled(false);
  }

  if (m_SelectedPlanarFigures->GetSize() == 0 && this->GetRenderWindowPart() != 0)
  {
    foreach (QmitkRenderWindow* renderWindow, this->GetRenderWindowPart()->GetRenderWindows().values())
    {
      this->SetMeasurementInfoToRenderWindow("", renderWindow);
    }
  }

  unsigned int j = 1;
  mitk::PlanarFigure* _PlanarFigure = 0;
  mitk::PlanarAngle* planarAngle = 0;
  mitk::PlanarFourPointAngle* planarFourPointAngle = 0;
  mitk::DataNode::Pointer node = 0;
  m_SelectedPlanarFiguresText->clear();
  QString infoText;
  QString plainInfoText;
  std::vector<mitk::DataNode*> nodes = m_SelectedPlanarFigures->GetNodes();

  for (std::vector<mitk::DataNode*>::iterator it = nodes.begin(); it
    != nodes.end(); ++it, ++j)
  {
    plainInfoText.clear();
    node = *it;
    if(j>1)
      infoText.append("<br />");

    infoText.append(QString("<b>%1</b><hr />").arg(QString::fromStdString(
      node->GetName())));
    plainInfoText.append(QString("%1").arg(QString::fromStdString(
      node->GetName())));

    _PlanarFigure = dynamic_cast<mitk::PlanarFigure*> (node->GetData());

    planarAngle = dynamic_cast<mitk::PlanarAngle*> (_PlanarFigure);
    if(!planarAngle)
    {
      planarFourPointAngle = dynamic_cast<mitk::PlanarFourPointAngle*> (_PlanarFigure);
    }

    if(!_PlanarFigure)
      continue;

    double featureQuantity = 0.0;
    for (unsigned int i = 0; i < _PlanarFigure->GetNumberOfFeatures(); ++i)
    {
      if ( !_PlanarFigure->IsFeatureActive( i ) ) continue;

      featureQuantity = _PlanarFigure->GetQuantity(i);
      if ((planarAngle && i == planarAngle->FEATURE_ID_ANGLE)
        || (planarFourPointAngle && i == planarFourPointAngle->FEATURE_ID_ANGLE))
        featureQuantity = featureQuantity * 180 / vnl_math::pi;

      infoText.append(
        QString("<i>%1</i>: %2 %3") .arg(QString(
        _PlanarFigure->GetFeatureName(i))) .arg(featureQuantity, 0, 'f',
        2) .arg(QString(_PlanarFigure->GetFeatureUnit(i))));

      plainInfoText.append(
        QString("\n%1: %2 %3") .arg(QString(_PlanarFigure->GetFeatureName(i))) .arg(
        featureQuantity, 0, 'f', 2) .arg(QString(
        _PlanarFigure->GetFeatureUnit(i))));

      if(i+1 != _PlanarFigure->GetNumberOfFeatures())
        infoText.append("<br />");
    }

    if (j != nodes.size())
      infoText.append("<br />");
  }

  m_SelectedPlanarFiguresText->setHtml(infoText);

  // for the last selected planar figure ...
  if (_PlanarFigure)
  {
    const mitk::PlaneGeometry
      * _PlaneGeometry =
      dynamic_cast<const mitk::PlaneGeometry*> (_PlanarFigure->GetGeometry2D());

    QmitkRenderWindow* selectedRenderWindow = 0;
    bool PlanarFigureInitializedWindow = false;
    mitk::IRenderWindowPart* renderPart = this->GetRenderWindowPart();
    if (renderPart)
    {
      foreach(QmitkRenderWindow* renderWindow, renderPart->GetRenderWindows().values())
      {
        if (node->GetBoolProperty("PlanarFigureInitializedWindow", PlanarFigureInitializedWindow,
                                  renderWindow->GetRenderer()))
        {
          selectedRenderWindow = renderWindow;
          break;
        }
      }
    }

    // make node visible
    if (selectedRenderWindow)
    {
      mitk::Point3D centerP = _PlaneGeometry->GetOrigin();
      //selectedRenderWindow->GetSliceNavigationController()->ReorientSlices(
      //centerP, _PlaneGeometry->GetNormal());
      selectedRenderWindow->GetSliceNavigationController()->SelectSliceByPoint(
        centerP);

      // now paint infos also on renderwindow
      this->SetMeasurementInfoToRenderWindow(plainInfoText, selectedRenderWindow);
    }
  }
  // no last planarfigure
  else
    this->SetMeasurementInfoToRenderWindow("", 0);

  this->RequestRenderWindowUpdate();
}

void QmitkMeasurementView::NodeAddedInDataStorage(const mitk::DataNode* node)
{
  if(!m_Visible)
    return;
  mitk::DataNode* nonConstNode = const_cast<mitk::DataNode*>(node);
  mitk::PlanarFigure* figure = dynamic_cast<mitk::PlanarFigure*>(nonConstNode->GetData());
  if(figure)
  {
    mitk::PlanarFigureInteractor::Pointer figureInteractor
      = dynamic_cast<mitk::PlanarFigureInteractor*>(node->GetInteractor());

    if(figureInteractor.IsNull())
      figureInteractor = mitk::PlanarFigureInteractor::New("PlanarFigureInteractor", nonConstNode);

    // remove old interactor if present
    if( m_CurrentFigureNode.IsNotNull() && m_CurrentFigureNodeInitialized == false )
    {
      mitk::Interactor::Pointer oldInteractor = m_CurrentFigureNode->GetInteractor();
      if(oldInteractor.IsNotNull())
        mitk::GlobalInteraction::GetInstance()->RemoveInteractor(oldInteractor);

      this->RemoveEndPlacementObserverTag();
      this->GetDataStorage()->Remove(m_CurrentFigureNode);
    }

    mitk::GlobalInteraction::GetInstance()->AddInteractor(figureInteractor);
  }
}


void QmitkMeasurementView::PlanarFigureInitialized()
{
  if(m_CurrentFigureNode.IsNull())
    return;

  m_CurrentFigureNodeInitialized = true;

  this->PlanarFigureSelectionChanged();

  m_DrawLine->setChecked(false);
  m_DrawPath->setChecked(false);
  m_DrawAngle->setChecked(false);
  m_DrawFourPointAngle->setChecked(false);
  m_DrawEllipse->setChecked(false);
  m_DrawRectangle->setChecked(false);
  m_DrawPolygon->setChecked(false);
  // enable the crosshair navigation
  this->EnableCrosshairNavigation();
}


void QmitkMeasurementView::PlanarFigureSelected( itk::Object* object, const itk::EventObject& )
{
  // Mark to-be-edited PlanarFigure as selected
  mitk::PlanarFigure* figure = dynamic_cast< mitk::PlanarFigure* >( object );
  if ( figure != NULL )
  {
    // Get node corresponding to PlanarFigure
    mitk::DataNode::Pointer figureNode = this->GetDataStorage()->GetNode(
      mitk::NodePredicateData::New( figure ) );

    // Select this node (and deselect all others)
    QList< mitk::DataNode::Pointer > selectedNodes = this->GetDataManagerSelection();
    for ( int i = 0; i < selectedNodes.size(); i++ )
    {
      selectedNodes[i]->SetSelected( false );
    }
    std::vector< mitk::DataNode* > selectedNodes2 = m_SelectedPlanarFigures->GetNodes();
    for ( unsigned int i = 0; i < selectedNodes2.size(); i++ )
    {
      selectedNodes2[i]->SetSelected( false );
    }
    figureNode->SetSelected( true );

    this->RemoveEndPlacementObserverTag();
    m_CurrentFigureNode = figureNode;

    *m_SelectedPlanarFigures = figureNode;

    // Re-initialize after selecting new PlanarFigure
    this->PlanarFigureSelectionChanged();
  }
}

void QmitkMeasurementView::RemoveEndPlacementObserverTag()
{

  if(m_CurrentFigureNode.IsNotNull())
  {
    mitk::PlanarFigure* figure = dynamic_cast<mitk::PlanarFigure*>(m_CurrentFigureNode->GetData());
    if( figure != 0 )
      figure->RemoveObserver( m_EndPlacementObserverTag );
  }
}

mitk::DataNode::Pointer QmitkMeasurementView::DetectTopMostVisibleImage()
{
  // get all images from the data storage
  mitk::DataStorage::SetOfObjects::ConstPointer Images = this->GetDataStorage()->GetSubset( mitk::NodePredicateDataType::New("Image") );

  mitk::DataNode::Pointer currentNode( m_SelectedImageNode->GetNode() );
  int maxLayer = itk::NumericTraits<int>::min();

  // iterate over selection
  for (mitk::DataStorage::SetOfObjects::ConstIterator sofIt = Images->Begin(); sofIt != Images->End(); ++sofIt)
  {
    mitk::DataNode::Pointer node = sofIt->Value();
    if ( node.IsNull() )
      continue;
    if (node->IsVisible(NULL) == false)
      continue;
    int layer = 0;
    node->GetIntProperty("layer", layer);
    if ( layer < maxLayer )
      continue;

    currentNode = node;
  }

  return currentNode;
}

void QmitkMeasurementView::AddFigureToDataStorage(mitk::PlanarFigure* figure, const QString& name,
                                              const char *propertyKey, mitk::BaseProperty *property )
{
  if ( m_CurrentFigureNode.IsNotNull() )
  {
    m_CurrentFigureNode->GetData()->RemoveObserver( m_EndPlacementObserverTag );
  }

  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetName(name.toStdString());
  newNode->SetData(figure);

  // Add custom property, if available
  if ( (propertyKey != NULL) && (property != NULL) )
  {
    newNode->AddProperty( propertyKey, property );
  }

  // add observer for event when figure has been placed
  typedef itk::SimpleMemberCommand< QmitkMeasurementView > SimpleCommandType;
  SimpleCommandType::Pointer initializationCommand = SimpleCommandType::New();
  initializationCommand->SetCallbackFunction( this, &QmitkMeasurementView::PlanarFigureInitialized );
  m_EndPlacementObserverTag = figure->AddObserver( mitk::EndPlacementPlanarFigureEvent(), initializationCommand );

  // add observer for event when figure is picked (selected)
  typedef itk::MemberCommand< QmitkMeasurementView > MemberCommandType;
  MemberCommandType::Pointer selectCommand = MemberCommandType::New();
  selectCommand->SetCallbackFunction( this, &QmitkMeasurementView::PlanarFigureSelected );
  m_SelectObserverTag = figure->AddObserver( mitk::SelectPlanarFigureEvent(), selectCommand );

  // add observer for event when interaction with figure starts
  SimpleCommandType::Pointer startInteractionCommand = SimpleCommandType::New();
  startInteractionCommand->SetCallbackFunction( this, &QmitkMeasurementView::DisableCrosshairNavigation);
  m_StartInteractionObserverTag = figure->AddObserver( mitk::StartInteractionPlanarFigureEvent(), startInteractionCommand );

  // add observer for event when interaction with figure starts
  SimpleCommandType::Pointer endInteractionCommand = SimpleCommandType::New();
  endInteractionCommand->SetCallbackFunction( this, &QmitkMeasurementView::EnableCrosshairNavigation);
  m_EndInteractionObserverTag = figure->AddObserver( mitk::EndInteractionPlanarFigureEvent(), endInteractionCommand );

  // figure drawn on the topmost layer / image
  this->GetDataStorage()->Add(newNode, this->DetectTopMostVisibleImage() );
  QList<mitk::DataNode::Pointer> selectedNodes = GetDataManagerSelection();
  for(int i = 0; i < selectedNodes.size(); i++)
  {
    selectedNodes[i]->SetSelected(false);
  }
  std::vector<mitk::DataNode*> selectedNodes2 = m_SelectedPlanarFigures->GetNodes();
  for(unsigned int i = 0; i < selectedNodes2.size(); i++)
  {
    selectedNodes2[i]->SetSelected(false);
  }
  newNode->SetSelected(true);


  m_CurrentFigureNodeInitialized = false;
  m_CurrentFigureNode = newNode;

  *m_SelectedPlanarFigures = newNode;

  this->RequestRenderWindowUpdate();
}

bool QmitkMeasurementView::AssertDrawingIsPossible(bool checked)
{
  if (m_SelectedImageNode->GetNode().IsNull())
  {
    checked = false;
    this->HandleException("Please select an image!", this->m_Parent, true);
    m_DrawLine->setChecked(false);
    return false;
  }

  mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart());
  if (linkedRenderWindow)
  {
    linkedRenderWindow->EnableSlicingPlanes(false);
  }
  // disable the crosshair navigation during the drawing
  this->DisableCrosshairNavigation();

  return checked;
}

void QmitkMeasurementView::ActionDrawLineTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;
  mitk::PlanarLine::Pointer figure = mitk::PlanarLine::New();
  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_LineCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Line%1").arg(++m_LineCounter);
  }
  else{
    qString = QString("Line%1").arg(m_LineCounter);
  }
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarLine initialized...";
}

void QmitkMeasurementView::ActionDrawPathTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  mitk::PlanarPolygon::Pointer figure = mitk::PlanarPolygon::New();
  figure->ClosedOff();

  mitk::BoolProperty::Pointer closedProperty = mitk::BoolProperty::New( false );

  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_PathCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Path%1").arg(++m_PathCounter);
  }
  else{
    qString = QString("Path%1").arg(m_PathCounter);
  }
  this->AddFigureToDataStorage(figure, qString,
    "ClosedPlanarPolygon", closedProperty);

  MITK_DEBUG << "PlanarPath initialized...";
}

void QmitkMeasurementView::ActionDrawAngleTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  mitk::PlanarAngle::Pointer figure = mitk::PlanarAngle::New();
  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_AngleCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Angle%1").arg(++m_AngleCounter);
  }
  else{
    qString = QString("Angle%1").arg(m_AngleCounter);
  }
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarAngle initialized...";
}

void QmitkMeasurementView::ActionDrawFourPointAngleTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  mitk::PlanarFourPointAngle::Pointer figure =
    mitk::PlanarFourPointAngle::New();
  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_FourPointAngleCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Four Point Angle%1").arg(++m_FourPointAngleCounter);
  }
  else{
    qString = QString("Four Point Angle%1").arg(m_FourPointAngleCounter);
  }
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarFourPointAngle initialized...";
}

void QmitkMeasurementView::ActionDrawEllipseTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  mitk::PlanarCircle::Pointer figure = mitk::PlanarCircle::New();
  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_EllipseCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Circle%1").arg(++m_EllipseCounter);
  }
  else{
    qString = QString("Circle%1").arg(m_EllipseCounter);
  }
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarCircle initialized...";
}

void QmitkMeasurementView::ActionDrawRectangleTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  mitk::PlanarRectangle::Pointer figure = mitk::PlanarRectangle::New();
  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_RectangleCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Rectangle%1").arg(++m_RectangleCounter);
  }
  else{
    qString = QString("Rectangle%1").arg(m_RectangleCounter);
  }
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarRectangle initialized...";
}

void QmitkMeasurementView::ActionDrawPolygonTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  mitk::PlanarPolygon::Pointer figure = mitk::PlanarPolygon::New();
  figure->ClosedOn();
  QString qString;
  if(m_CurrentFigureNode.IsNull() || m_PolygonCounter == 0 || m_CurrentFigureNodeInitialized){
    qString = QString("Polygon%1").arg(++m_PolygonCounter);
  }
  else{
    qString = QString("Polygon%1").arg(m_PolygonCounter);
  }
  this->AddFigureToDataStorage(figure, qString);

  MITK_DEBUG << "PlanarPolygon initialized...";
}

void QmitkMeasurementView::ActionDrawArrowTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  MITK_WARN << "Draw Arrow not implemented yet.";
}

void QmitkMeasurementView::ActionDrawTextTriggered(bool checked)
{
  if(!this->AssertDrawingIsPossible(checked))
    return;

  MITK_WARN << "Draw Text not implemented yet.";
}

void QmitkMeasurementView::Activated()
{
  m_Activated = true;
  MITK_DEBUG << "QmitkMeasurementView::Activated";

  if (mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart()))
  {
    linkedRenderWindow->EnableSlicingPlanes(false);
  }

  mitk::TNodePredicateDataType<mitk::PlanarFigure>::Pointer isPlanarFigure
    = mitk::TNodePredicateDataType<mitk::PlanarFigure>::New();

  mitk::DataStorage::SetOfObjects::ConstPointer _NodeSet = this->GetDataStorage()->GetAll();
  mitk::DataNode* node = 0;
  mitk::PlanarFigure* figure = 0;
  mitk::PlanarFigureInteractor::Pointer figureInteractor = 0;
  // finally add all nodes to the model
  for(mitk::DataStorage::SetOfObjects::ConstIterator it=_NodeSet->Begin(); it!=_NodeSet->End()
    ; it++)
  {
    node = const_cast<mitk::DataNode*>(it->Value().GetPointer());
    figure = dynamic_cast<mitk::PlanarFigure*>(node->GetData());
    if(figure)
    {
      figureInteractor = dynamic_cast<mitk::PlanarFigureInteractor*>(node->GetInteractor());

      if(figureInteractor.IsNull())
        figureInteractor = mitk::PlanarFigureInteractor::New("PlanarFigureInteractor", node);

      mitk::GlobalInteraction::GetInstance()->AddInteractor(figureInteractor);
    }
  }

  m_Visible = true;

}

void QmitkMeasurementView::Deactivated()
{
  MITK_DEBUG << "QmitkMeasurementView::Deactivated";
}

void QmitkMeasurementView::ActivatedZombieView(berry::IWorkbenchPartReference::Pointer )
{
  if (mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart()))
  {
    linkedRenderWindow->EnableSlicingPlanes(true);
  }

  this->SetMeasurementInfoToRenderWindow("", m_LastRenderWindow);
  this->EnableCrosshairNavigation();

  mitk::DataStorage::SetOfObjects::ConstPointer _NodeSet = this->GetDataStorage()->GetAll();
  mitk::DataNode* node = 0;
  mitk::PlanarFigure* figure = 0;
  mitk::PlanarFigureInteractor::Pointer figureInteractor = 0;
  // finally add all nodes to the model
  for(mitk::DataStorage::SetOfObjects::ConstIterator it=_NodeSet->Begin(); it!=_NodeSet->End()
    ; it++)
  {
    node = const_cast<mitk::DataNode*>(it->Value().GetPointer());
    figure = dynamic_cast<mitk::PlanarFigure*>(node->GetData());
    if(figure)
    {

      figure->RemoveAllObservers();
      figureInteractor = dynamic_cast<mitk::PlanarFigureInteractor*>(node->GetInteractor());

      if(figureInteractor)
        mitk::GlobalInteraction::GetInstance()->RemoveInteractor(figureInteractor);
    }
  }

  m_Activated = false;
}

void QmitkMeasurementView::Visible()
{
  m_Visible = true;
  MITK_DEBUG << "QmitkMeasurementView::Visible";
}

void QmitkMeasurementView::Hidden()
{
  m_Visible = false;
  MITK_DEBUG << "QmitkMeasurementView::Hidden";
}

void QmitkMeasurementView::PropertyChanged(const mitk::DataNode*, const mitk::BaseProperty*)
{
  this->PlanarFigureSelectionChanged();
}

void QmitkMeasurementView::NodeChanged(const mitk::DataNode* /*node* /)
{
  this->PlanarFigureSelectionChanged();
}

void QmitkMeasurementView::NodeRemoved(const mitk::DataNode* /*node* /)
{
  this->PlanarFigureSelectionChanged();
}

void QmitkMeasurementView::CopyToClipboard(bool)
{
  std::vector<QString> headerRow;
  std::vector<std::vector<QString> > rows;
  QString featureName;
  QString featureQuantity;
  std::vector<QString> newRow;
  headerRow.push_back("Name");

  std::vector<mitk::DataNode*> nodes = m_SelectedPlanarFigures->GetNodes();

  for (std::vector<mitk::DataNode*>::iterator it = nodes.begin(); it
    != nodes.end(); ++it)
  {
    mitk::PlanarFigure* planarFigure =
      dynamic_cast<mitk::PlanarFigure*> ((*it)->GetData());
    if (!planarFigure)
      continue;

    newRow.clear();
    newRow.push_back(QString::fromStdString((*it)->GetName()));
    newRow.resize(headerRow.size());
    for (unsigned int i = 0; i < planarFigure->GetNumberOfFeatures(); ++i)
    {
      if ( !planarFigure->IsFeatureActive( i ) ) continue;

      featureName = planarFigure->GetFeatureName(i);
      featureName.append(QString(" [%1]").arg(planarFigure->GetFeatureUnit(i)));
      std::vector<QString>::iterator itColumn = std::find(headerRow.begin(),
        headerRow.end(), featureName);

      featureQuantity
        = QString("%1").arg(planarFigure->GetQuantity(i)).replace(QChar('.'),
        ",");
      if (itColumn == headerRow.end())
      {
        headerRow.push_back(featureName);
        newRow.push_back(featureQuantity);
      } else
      {
        newRow[std::distance(headerRow.begin(), itColumn)] = featureQuantity;
      }

    }
    rows.push_back(newRow);
  }

  QString clipboardText;
  for (std::vector<QString>::iterator it = headerRow.begin(); it
    != headerRow.end(); ++it)
    clipboardText.append(QString("%1 \t").arg(*it));

  for (std::vector<std::vector<QString> >::iterator it = rows.begin(); it
    != rows.end(); ++it)
  {
    clipboardText.append("\n");
    for (std::vector<QString>::iterator it2 = (*it).begin(); it2 != (*it).end(); ++it2)
    {
      clipboardText.append(QString("%1 \t").arg(*it2));
    }
  }

  QApplication::clipboard()->setText(clipboardText, QClipboard::Clipboard);

}

void QmitkMeasurementView::SetMeasurementInfoToRenderWindow(const QString& text,
                                                        QmitkRenderWindow* _RenderWindow)
{
  if(m_LastRenderWindow != _RenderWindow)
  {

    if(m_LastRenderWindow)
    {
      QObject::disconnect( m_LastRenderWindow, SIGNAL( destroyed(QObject*) )
        , this, SLOT( OnRenderWindowDelete(QObject*) ) );
    }
    m_LastRenderWindow = _RenderWindow;
    if(m_LastRenderWindow)
    {
      QObject::connect( m_LastRenderWindow, SIGNAL( destroyed(QObject*) )
        , this, SLOT( OnRenderWindowDelete(QObject*) ) );
    }
  }

  if(m_LastRenderWindow)
  {
    if (!text.isEmpty() && m_SelectedPlanarFigures->GetNode()->IsSelected())
    {
      m_MeasurementInfoAnnotation->SetText(1, text.toLatin1().data());
      mitk::VtkLayerController::GetInstance(m_LastRenderWindow->GetRenderWindow())->InsertForegroundRenderer(
        m_MeasurementInfoRenderer, true);
    }
    else
    {
      if (mitk::VtkLayerController::GetInstance(
        m_LastRenderWindow->GetRenderWindow()) ->IsRendererInserted(
        m_MeasurementInfoRenderer))
        mitk::VtkLayerController::GetInstance(m_LastRenderWindow->GetRenderWindow())->RemoveRenderer(
        m_MeasurementInfoRenderer);
    }
  }
}

void QmitkMeasurementView::EnableCrosshairNavigation()
{
  // enable the crosshair navigation
  if (mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart()))
  {
    linkedRenderWindow->EnableLinkedNavigation(true);
  }
}

void QmitkMeasurementView::DisableCrosshairNavigation()
{
  // disable the crosshair navigation during the drawing
  if (mitk::ILinkedRenderWindowPart* linkedRenderWindow =
      dynamic_cast<mitk::ILinkedRenderWindowPart*>(this->GetRenderWindowPart()))
  {
    linkedRenderWindow->EnableLinkedNavigation(false);
  }
}

void QmitkMeasurementView::SetFocus()
{

}

void QmitkMeasurementView::OnRenderWindowDelete(QObject * obj)
{
  if(obj == m_LastRenderWindow)
    m_LastRenderWindow = 0;
}

void QmitkMeasurementView::ReproducePotentialBug(bool)
{
  std::vector<mitk::DataNode*> nodes = m_SelectedPlanarFigures->GetNodes();
  QString output;

  for (std::vector<mitk::DataNode*>::iterator it = nodes.begin(); it
    != nodes.end(); ++it)
  {
    mitk::DataNode* node = *it;
    if (!node) continue;

    mitk::PlanarFigure* pf = dynamic_cast<mitk::PlanarFigure*>( node->GetData() );
    if (!pf) continue;

    output.append("huhu");
    output.append( QString::fromStdString( node->GetName() ) );

    /**
      Bug reproduction:

      1. get geometry of planar figure from object

      2. use this geometry to initialize rendering manager via InitializeViews

      3. see what is produced. the DisplayGeometry of the render window will NOT contain the planar figure nicely.

    * /

    mitk::PlaneGeometry::Pointer planarFigureGeometry = dynamic_cast<mitk::PlaneGeometry*>( pf->GetGeometry() );
    if (planarFigureGeometry.IsNull()) continue; // not expected


    mitk::PlaneGeometry::Pointer geometryForRendering = dynamic_cast<mitk::PlaneGeometry*>( planarFigureGeometry->Clone().GetPointer() );

    bool applyWorkaround(true);
    geometryForRendering->SetImageGeometry( applyWorkaround );


    std::cout << "==== with" << (applyWorkaround?"":"OUT") << " workaround ====================================" << std::endl;
    std::cout << "--- PlanarFigure geometry --------------" << std::endl;
    planarFigureGeometry->Print(std::cout);
    std::cout << "----------------------------------------" << std::endl;

    mitk::RenderingManager::GetInstance()->InitializeViews( geometryForRendering, mitk::RenderingManager::REQUEST_UPDATE_ALL, false );

    QmitkRenderWindow* renderWindow = this->GetRenderWindowPart()->GetRenderWindows().values().front();
    if (renderWindow == 0)
    {
      std::cout << "** No QmitkkRenderWindow available **" << std::endl;
      return;
    }

    std::cout << "--- Renderer->GetDisplayGeometry() ------------" << std::endl;

    renderWindow->GetRenderer()->GetDisplayGeometry()->Print(std::cout);
    std::cout << "--- Renderer->GetCurrentWorldGeometry2D() -----" << std::endl;
    renderWindow->GetRenderer()->GetCurrentWorldGeometry2D()->Print(std::cout);
    std::cout << "--- Renderer->GetWorldGeometry() --------------" << std::endl;
    renderWindow->GetRenderer()->GetWorldGeometry()->Print(std::cout);
  }

  m_SelectedPlanarFiguresText->setText(output);

}
*/

