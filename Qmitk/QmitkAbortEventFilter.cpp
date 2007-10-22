/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile$
Language:  C++
Date:      $Date$
Version:   $Revision$

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "mitkVolumeDataVtkMapper3D.h"
#include "QmitkAbortEventFilter.h"
#include "QmitkRenderWindow.h"
#include "mitkRenderingManager.h"

#include "mitkNodePredicateProperty.h"
#include "mitkDataStorage.h"
#include "mitkProperties.h"

#include <qapplication.h>
#include <qeventloop.h>

QmitkAbortEventFilter*
QmitkAbortEventFilter
::GetInstance()
{
  static QmitkAbortEventFilter instance;
  return &instance;
}

QmitkAbortEventFilter::QmitkAbortEventFilter()
{
}
QmitkAbortEventFilter::~QmitkAbortEventFilter()

{
}

bool QmitkAbortEventFilter::eventFilter( QObject *object, QEvent *event )
{   
  if (mitk::RenderingManager::GetInstance()->IsRendering() )
  {
    switch ( event->type() )
    {
   
    case QEvent::MouseButtonPress:
    { 
      m_ButtonPressed = true;
      //std::cout << "#BP "<<std::endl;

      mitk::RenderingManager::GetInstance()->AbortRendering( NULL );
      QMouseEvent* me = ( QMouseEvent* )( event );

      QMouseEvent* newEvent = new QMouseEvent(
        me->type(), me->pos(), me->globalPos(), me->button(), me->state()
      );
      m_EventQueue.push( ObjectEventPair(object, newEvent) );
      return true;
    }

    
    case QEvent::MouseButtonDblClick:
    { 
      //std::cout << "#DC "<<std::endl;
      mitk::RenderingManager::GetInstance()->AbortRendering( NULL );
      QMouseEvent* me = ( QMouseEvent* )( event );

      QMouseEvent* newEvent = new QMouseEvent(
        me->type(), me->pos(), me->globalPos(), me->button(), me->state()
      );
      m_EventQueue.push( ObjectEventPair(object, newEvent) );
      return true;
    }

    case QEvent::MouseButtonRelease:
    { 
      m_ButtonPressed = false;
      //std::cout << "#BR "<<std::endl;
      
      QMouseEvent* me = ( QMouseEvent* )( event );

      QMouseEvent* newEvent = new QMouseEvent(
        me->type(), me->pos(), me->globalPos(), me->button(), me->state()
      );
      m_EventQueue.push( ObjectEventPair(object, newEvent) );
      return true;
    }

    case QEvent::MouseMove:
    { 
      if(m_ButtonPressed && mitk::RenderingManager::GetInstance()->GetCurrentLOD()!=0)
      {
        //std::cout << "#MM "<<std::endl;
        mitk::RenderingManager::GetInstance()->SetCurrentLOD(0);
        mitk::RenderingManager::GetInstance()->AbortRendering( NULL );
      }
      return true;
    }
    
   
    case QEvent::Resize:
      { 
        //std::cout << "#R "<<std::endl;
        mitk::RenderingManager::GetInstance()->AbortRendering( NULL );

        QResizeEvent* re = ( QResizeEvent* )( event );

        QResizeEvent* newResizeEvent = new QResizeEvent( re->size(), re->oldSize() );
        m_EventQueue.push( ObjectEventPair(object, newResizeEvent) );
        return true;
      }

    case QEvent::Paint:
      { 
        //std::cout << "#P ";
        QPaintEvent* pe = ( QPaintEvent* )( event );
        QPaintEvent* newPaintEvent = new QPaintEvent( pe->region() );
        m_EventQueue.push( ObjectEventPair(object, newPaintEvent) );
        return true;
      }

     case QEvent::ChildInserted: //change Layout (Big3D, 2D images up, etc.)
      {
        //std::cout << "#CI "<<std::endl;
        mitk::RenderingManager::GetInstance()->SetCurrentLOD(0);
        mitk::RenderingManager::GetInstance()->AbortRendering( NULL );
        return false;
      }

      case QEvent::KeyPress:
      { 
        //std::cout << "#KP "<<std::endl;
        mitk::RenderingManager::GetInstance()->AbortRendering( NULL );
        QKeyEvent* ke = ( QKeyEvent* )( event );
        QKeyEvent* newEvent = new QKeyEvent(
          ke->type(), ke->key(), ke->ascii(), ke->state(), ke->text(), false, ke->count()
        );
        m_EventQueue.push( ObjectEventPair(object, newEvent) );
        return true;
      }

      case QEvent::Timer:
      { 
        //std::cout << "#T ";
        QTimerEvent* te = ( QTimerEvent* )( event );
        QTimerEvent* newEvent = new QTimerEvent(te->timerId());
        m_EventQueue.push( ObjectEventPair(object, newEvent) );
        return true;
      }

      default:
      {
        //std::cout<<"Event Type: (Rendering)"<<event->type()<<std::endl;
        return false;
      }
    }
  }
 else
 {
    switch ( event->type() )
    {
      case QEvent::MouseButtonPress:
      {
        m_ButtonPressed = true;
        //std::cout << "#BP2 "<<std::endl;
        return false;
      }

      case QEvent::MouseMove:
      {
        if(m_ButtonPressed)
        {
        //std::cout << "#MM2 "<<std::endl;
          mitk::DataStorage* dataStorage = mitk::DataStorage::GetInstance();
          mitk::NodePredicateProperty VolRenTurnedOn("volumerendering", new mitk::BoolProperty(true));
          mitk::DataStorage::SetOfObjects::ConstPointer VolRenSet = 
              dataStorage->GetSubset( VolRenTurnedOn );
          if ( VolRenSet->Size() > 0 )
          {
            mitk::RenderingManager::GetInstance()->SetCurrentLOD(0);
          }
          
        }
        return false;
      }

      case QEvent::MouseButtonRelease:
      {
        m_ButtonPressed = false;
        //std::cout << "#BR2 "<<std::endl;
        return false;
      }
    
      case QEvent::Resize:
      {
        //std::cout << "#R2 "<<std::endl;
        //mitk::RenderingManager::GetInstance()->SetCurrentLOD(0);
        return false;
      } 

      case QEvent::ChildInserted:
      {
        //std::cout << "#CI2 ";
        mitk::RenderingManager::GetInstance()->SetCurrentLOD(0);
        return false;
      }

      default:
      {
        //std::cout<<"Event Type: (Not Rendering)"<<event->type()<<std::endl;
        return false;
      }
    }
  }
}

void QmitkAbortEventFilter::ProcessEvents()
{ 
  //std::cout << "P";
  qApp->eventLoop()->processEvents( QEventLoop::AllEvents );
}

void QmitkAbortEventFilter::IssueQueuedEvents()
{
  while ( !m_EventQueue.empty() )
  {
    ObjectEventPair eventPair = m_EventQueue.front();
    QApplication::postEvent( eventPair.first, eventPair.second );
    m_EventQueue.pop();
  }

}
