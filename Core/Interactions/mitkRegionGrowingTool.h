/*=========================================================================
 
Program:   Medical Imaging & Interaction Toolkit
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

#ifndef mitkRegionGrowingTool_h_Included
#define mitkRegionGrowingTool_h_Included

#include "mitkFeedbackContourTool.h"

struct ipPicDescriptor;

namespace mitk
{

/**
  \brief A slice based region growing tool.

  \sa FeedbackContourTool

  \ingroup Interaction
  \ingroup Reliver

  When the user presses the mouse button, RegionGrowingTool will use the gray values at that position
  to initialize a region growing algorithm (in the affected 2D slice).

  By moving the mouse up and down while the button is still pressed, the user can change the parameters
  of the region growing algorithm (selecting more or less of an object). 
  The current result of region growing will always be shown as a contour to the user.

  After releasing the button, the current result of the region growing algorithm will be written to the
  working image of this tool's ToolManager.

  If the first click is <i>inside</i> a segmentation that was generated by region growing (recently),
  the tool will try to cut off a part of the segmentation. For this reason a skeletonization of the segmentation
  is generated and the optimal cut point is determined.

  \warning Only to be instantiated by mitk::ToolManager.

  $Author$
*/
class MITK_CORE_EXPORT RegionGrowingTool : public FeedbackContourTool
{
  public:
    
    mitkClassMacro(RegionGrowingTool, FeedbackContourTool);
    itkNewMacro(RegionGrowingTool);

    virtual const char** GetXPM() const;
    virtual const char* GetName() const;

  protected:
    
    RegionGrowingTool(); // purposely hidden
    virtual ~RegionGrowingTool();

    virtual void Activated();
    virtual void Deactivated();
    
    virtual bool OnMousePressed (Action*, const StateEvent*);
    virtual bool OnMousePressedInside (Action*, const StateEvent*, ipPicDescriptor* workingPicSlice, int initialWorkingOffset);
    virtual bool OnMousePressedOutside (Action*, const StateEvent*);
    virtual bool OnMouseMoved   (Action*, const StateEvent*);
    virtual bool OnMouseReleased(Action*, const StateEvent*);

    ipPicDescriptor* PerformRegionGrowingAndUpdateContour();

    Image::Pointer m_ReferenceSlice;
    Image::Pointer m_WorkingSlice;

    int m_LowerThreshold;
    int m_UpperThreshold;
    int m_InitialLowerThreshold;
    int m_InitialUpperThreshold;

    int m_ScreenYPositionAtStart;

  private:

    ipPicDescriptor* SmoothIPPicBinaryImage( ipPicDescriptor* image, int &contourOfs, ipPicDescriptor* dest = NULL );
    void SmoothIPPicBinaryImageHelperForRows( ipPicDescriptor* source, ipPicDescriptor* dest, int &contourOfs, int* maskOffsets, int maskSize, int startOffset, int endOffset );
    
    ipPicDescriptor* m_OriginalPicSlice;
    int m_SeedPointMemoryOffset;

    ScalarType m_VisibleWindow;
    ScalarType m_MouseDistanceScaleFactor;
    
    int m_PaintingPixelValue;
    int m_LastWorkingSeed;

    bool m_FillFeedbackContour;
};

} // namespace

#endif


