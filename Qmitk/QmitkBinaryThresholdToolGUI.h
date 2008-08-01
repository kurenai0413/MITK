/*=========================================================================
 
Program:   Medical Imaging & Interaction Toolkit
Module:    $RCSfile: mitk.cpp,v $
Language:  C++
Date:      $Date$
Version:   $Revision: 1.0 $
 
Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.
 
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.
 
=========================================================================*/

#ifndef QmitkBinaryThresholdToolGUI_h_Included
#define QmitkBinaryThresholdToolGUI_h_Included

#include "QmitkToolGUI.h"
#include "mitkBinaryThresholdTool.h"

class QSlider;

/**
  \brief GUI for mitk::BinaryThresholdTool.

  This GUI shows a slider to change the tool's threshold and an OK button to accept a preview for actual thresholding.

  Last contributor: $Author$
*/
class QMITK_EXPORT QmitkBinaryThresholdToolGUI : public QmitkToolGUI
{
  Q_OBJECT

  public:
    
    mitkClassMacro(QmitkBinaryThresholdToolGUI, QmitkToolGUI);
    itkNewMacro(QmitkBinaryThresholdToolGUI);
    
    virtual ~QmitkBinaryThresholdToolGUI();

    void OnThresholdingIntervalBordersChanged(int lower, int upper);
    void OnThresholdingValueChanged(int current);

  signals:

  public slots:
  
  protected slots:

    void OnNewToolAssociated(mitk::Tool*);

    void OnSliderValueChanged(int value);
    void OnAcceptThresholdPreview();

  protected:

    QmitkBinaryThresholdToolGUI();

    QSlider* m_Slider;

    mitk::BinaryThresholdTool::Pointer m_BinaryThresholdTool;
};

#endif

