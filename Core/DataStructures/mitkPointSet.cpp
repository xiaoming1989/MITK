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


#include "mitkPointSet.h"
#include "mitkOperation.h"
#include "mitkOperationActor.h"
#include "mitkPointOperation.h"
#include "mitkInteractionConst.h"
#include "mitkXMLWriter.h"
#include "mitkXMLReader.h"
#include "mitkPointSetWriter.h"
#include "mitkPointSetReader.h"
#include "mitkRenderingManager.h"

#include <itkSmartPointerForwardReference.txx>


mitk::PointSet::PointSet()
{
  m_PointSetSeries.resize( 1 );

  m_PointSetSeries[0] = DataType::New();
  PointDataContainer::Pointer pointData = PointDataContainer::New();
  m_PointSetSeries[0]->SetPointData( pointData );

  this->Initialize( 1 );
}


mitk::PointSet::~PointSet()
{
}


void mitk::PointSet::Initialize( int timeSteps )
{
  mitk::TimeSlicedGeometry::Pointer timeGeometry = this->GetTimeSlicedGeometry();

  mitk::Geometry3D::Pointer g3d = mitk::Geometry3D::New();
  g3d->Initialize();
  mitk::ScalarType timeBounds[] = {0.0, 1.0};
  g3d->SetTimeBounds( timeBounds );

  //
  // The geometry is propagated automatically to the other items,
  // if EvenlyTimed is true...
  //
  timeGeometry->InitializeEvenlyTimed( g3d.GetPointer(), timeSteps );
}


void mitk::PointSet::AdaptPointSetSeriesSize( int timeSteps )
{
  // Check if the vector is long enouth to contain the new element
  // at the given position. If not, expand it with sufficient pre-initialized
  // elements.
  if ( timeSteps > m_PointSetSeries.size() )
  {
    int oldSize = m_PointSetSeries.size();
    m_PointSetSeries.resize( timeSteps );

    int i;
    for ( i = oldSize; i < timeSteps; ++i )
    {
      m_PointSetSeries[i] = DataType::New();
      PointDataContainer::Pointer pointData = PointDataContainer::New();
      m_PointSetSeries[i]->SetPointData( pointData );
    }

    this->Initialize( timeSteps );
  }
}


int mitk::PointSet::GetPointSetSeriesSize() const
{
  return m_PointSetSeries.size();
}


int mitk::PointSet::GetSize( int t ) const
{
  if ( t < m_PointSetSeries.size() )
  {
    return m_PointSetSeries[t]->GetNumberOfPoints();
  }
  else
  {
    return 0;
  }
}

mitk::PointSet::DataType::Pointer mitk::PointSet::GetPointSet( int t ) const
{
  if ( t < m_PointSetSeries.size() )
  {
    return m_PointSetSeries[t];
  }
  else
  {
    return NULL;
  }
}

int mitk::PointSet::SearchPoint( Point3D point, float distance, int t  )
{
  if ( t >= m_PointSetSeries.size() )
  {
    return -1;
  }
  
  // Out is the point which is checked to be the searched point
  PointType out;
  out.Fill( 0 );
  PointType indexPoint;

  this->GetGeometry( t )->WorldToIndex(point, indexPoint);

  // Searching the first point in the Set, that is +- distance far away fro
  // the given point
  unsigned int i;
  PointsContainer::Iterator it, end;
  end = m_PointSetSeries[t]->GetPoints()->End();
  int bestIndex = -1;
  distance = distance * distance;
  
  // To correct errors from converting index to world and world to index
  if (distance == 0.0)
  {
    distance = 0.000001;
  }

  ScalarType bestDist = distance;
  ScalarType dist, tmp;

  for ( it = m_PointSetSeries[t]->GetPoints()->Begin(), i = 0;
        it != end; 
        ++it, ++i )
  {
    bool ok = m_PointSetSeries[t]->GetPoints()
      ->GetElementIfIndexExists( it->Index(), &out );

    if ( !ok )
    {
      return -1;
    }
    else if ( indexPoint == out ) //if totaly equal
    {
      return it->Index();
    }

    //distance calculation
    tmp = out[0] - indexPoint[0]; dist  = tmp * tmp;
    tmp = out[1] - indexPoint[1]; dist += tmp * tmp;
    tmp = out[2] - indexPoint[2]; dist += tmp * tmp;

    if ( dist < bestDist )
    {
      bestIndex = it->Index();
      bestDist  = dist;
    }
  }
  return bestIndex;
}

mitk::PointSet::PointType 
mitk::PointSet::GetPoint( int position, int t ) const
{
  PointType out;
  out.Fill(0);

  if ( t >= m_PointSetSeries.size() )
  {
    return out;
  }

  if ( m_PointSetSeries[t]->GetPoints()->IndexExists(position) )
  {
    m_PointSetSeries[t]->GetPoint( position, &out );

    this->GetGeometry(t)->IndexToWorld( out, out );

    return out;
  }
  else
  {
    return out;
  }
}


bool 
mitk::PointSet
::GetPointIfExists( PointIdentifier id, PointType* point, int t )
{
  if ( t >= m_PointSetSeries.size() )
  {
    return false;
  }

  if ( m_PointSetSeries[t]->GetPoints()->GetElementIfIndexExists(id, point) )
  {
    this->GetGeometry( t )->IndexToWorld( *point, *point );
    return true;
  }
  else
  {
    return false;
  }
}


void mitk::PointSet::SetPoint( PointIdentifier id, PointType point, int t )
{
  this->AdaptPointSetSeriesSize( t+1 );

  mitk::Point3D indexPoint;
  this->GetGeometry( t )->WorldToIndex( point, indexPoint );
  m_PointSetSeries[t]->SetPoint( id, indexPoint );
}


void mitk::PointSet::InsertPoint( PointIdentifier id, PointType point, int t )
{
  if ( t < m_PointSetSeries.size() )
  {
    mitk::Point3D indexPoint;
    this->GetGeometry( t )->WorldToIndex( point, indexPoint );
    m_PointSetSeries[t]->GetPoints()->InsertElement( id, indexPoint );
  }
}


bool mitk::PointSet::IndexExists( int position, int t )
{
  if ( t < m_PointSetSeries.size() )
  {
    return m_PointSetSeries[t]->GetPoints()->IndexExists( position );
  }
  else
  {
    return false;
  }
}

bool mitk::PointSet::GetSelectInfo( int position, int t )
{
  if ( this->IndexExists( position, t ) )
  {
    PointDataType pointData = { 0, false, PTUNDEFINED };
    m_PointSetSeries[t]->GetPointData( position, &pointData );
    return pointData.selected;
  }
  else
  {
    return false;
  }
}


const int mitk::PointSet::GetNumberOfSelected( int t )
{
  if ( t >= m_PointSetSeries.size() )
  {
    return 0;
  }

  int numberOfSelected = 0;
  PointDataIterator it;
  for ( it = m_PointSetSeries[t]->GetPointData()->Begin();
        it != m_PointSetSeries[t]->GetPointData()->End();
        it++ )
  {
    if (it->Value().selected == true)
    {
      ++numberOfSelected;
    }
  }

  return numberOfSelected;
}


int mitk::PointSet::SearchSelectedPoint( int t )
{
  if ( t >= m_PointSetSeries.size() )
  {
    return -1;
  }

  PointDataIterator it;
  for ( it = m_PointSetSeries[t]->GetPointData()->Begin(); 
        it != m_PointSetSeries[t]->GetPointData()->End();
        it++ )
  {
    if ( it->Value().selected == true )
    {
      return it->Index();
    }
  }
  return -1;
}

void mitk::PointSet::ExecuteOperation( Operation* operation )
{

  int timeStep = 0;

  switch (operation->GetOperationType())
  {
  case OpNOTHING:
    break;
  case OpINSERT://inserts the point at the given position and selects it. 
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);
      
      timeStep = this->GetTimeSlicedGeometry()
        ->MSToTimeStep( pointOp->GetTimeInMS() );

      int position = pointOp->GetIndex();

      PointType pt;
      pt.CastFrom(pointOp->GetPoint());

      //transfer from world to index coordinates 
      this->GetGeometry( timeStep )->WorldToIndex(pt, pt);

      m_PointSetSeries[timeStep]->GetPoints()->InsertElement(position, pt);

      PointDataType pointData = 
      {
        pointOp->GetIndex(), 
        pointOp->GetSelected(), 
        pointOp->GetPointType()
      };

      m_PointSetSeries[timeStep]->GetPointData()
        ->InsertElement(position, pointData);

      this->Modified();
      ((const itk::Object*)this)->InvokeEvent( NewPointEvent() );
      this->OnPointSetChange();
    }
    break;
  case OpMOVE://moves the point given by index
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      timeStep = this->GetTimeSlicedGeometry()
        ->MSToTimeStep( pointOp->GetTimeInMS() );

      PointType pt;
      pt.CastFrom(pointOp->GetPoint());
      
      //transfer from world to index coordinates 
      this->GetGeometry( timeStep )->WorldToIndex(pt, pt);

      m_PointSetSeries[timeStep]->SetPoint(pointOp->GetIndex(), pt);

      this->OnPointSetChange();

      this->Modified();
    }
    break;
  case OpREMOVE://removes the point at given by position 
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      timeStep = this->GetTimeSlicedGeometry()
        ->MSToTimeStep( pointOp->GetTimeInMS() );

      m_PointSetSeries[timeStep]->GetPoints()
        ->DeleteIndex((unsigned)pointOp->GetIndex());
      m_PointSetSeries[timeStep]->GetPointData()
        ->DeleteIndex((unsigned)pointOp->GetIndex());

      this->OnPointSetChange();

      this->Modified();
     ((const itk::Object*)this)->InvokeEvent( RemovedPointEvent() );
    }
    break;
  case OpSELECTPOINT://select the given point
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      timeStep = this->GetTimeSlicedGeometry()
        ->MSToTimeStep( pointOp->GetTimeInMS() );

      PointDataType pointData = {0, false, PTUNDEFINED};
      m_PointSetSeries[timeStep]->GetPointData(pointOp->GetIndex(), &pointData);
      pointData.selected = true;
      m_PointSetSeries[timeStep]->SetPointData(pointOp->GetIndex(), pointData);
      this->Modified();
    }
    break;
  case OpDESELECTPOINT://unselect the given point
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      timeStep = this->GetTimeSlicedGeometry()
        ->MSToTimeStep( pointOp->GetTimeInMS() );

      PointDataType pointData = {0, false, PTUNDEFINED};
      m_PointSetSeries[timeStep]->GetPointData(pointOp->GetIndex(), &pointData);
      pointData.selected = false;
      m_PointSetSeries[timeStep]->SetPointData(pointOp->GetIndex(), pointData);
      this->Modified();
    }
    break;
  case OpSETPOINTTYPE:
    {
      mitkCheckOperationTypeMacro(PointOperation, operation, pointOp);

      timeStep = this->GetTimeSlicedGeometry()
        ->MSToTimeStep( pointOp->GetTimeInMS() );

      PointDataType pointData = {0, false, PTUNDEFINED};
      m_PointSetSeries[timeStep]->GetPointData(pointOp->GetIndex(), &pointData);
      pointData.pointSpec = pointOp->GetPointType();
      m_PointSetSeries[timeStep]->SetPointData(pointOp->GetIndex(), pointData);
      this->Modified();
    }
    break;
  default:
    itkWarningMacro("mitkPointSet could not understrand the operation. Please check!");
    break;
  }
  
  //to tell the mappers, that the data is modifierd and has to be updated 
  //only call modified if anything is done, so call in cases
  //this->Modified();

  mitk::OperationEndEvent endevent(operation);
  ((const itk::Object*)this)->InvokeEvent(endevent);

  //*todo has to be done here, cause of update-pipeline not working yet
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}


void mitk::PointSet::UpdateOutputInformation()
{
  if ( this->GetSource( ) )
  {
      this->GetSource( )->UpdateOutputInformation( );
  }
  const DataType::BoundingBoxType *bb = m_PointSetSeries[0]->GetBoundingBox();
  BoundingBox::BoundsArrayType itkBounds = bb->GetBounds();
  float mitkBounds[6];

  //for assignment see Geometry3d::SetBounds(const float bounds)
  mitkBounds[0] = itkBounds[0];
  mitkBounds[1] = itkBounds[1];
  mitkBounds[2] = itkBounds[2];
  mitkBounds[3] = itkBounds[3];
  mitkBounds[4] = itkBounds[4];
  mitkBounds[5] = itkBounds[5];

  GetGeometry()->SetBounds(itkBounds);
}

void mitk::PointSet::SetRequestedRegionToLargestPossibleRegion()
{
}

bool mitk::PointSet::RequestedRegionIsOutsideOfTheBufferedRegion()
{
    return false;
}

bool mitk::PointSet::VerifyRequestedRegion()
{
    return true;
}

void mitk::PointSet::SetRequestedRegion( itk::DataObject * )
{
}

bool mitk::PointSet::WriteXMLData( XMLWriter& xmlWriter )
{
  BaseData::WriteXMLData( xmlWriter );
  std::string fileName = xmlWriter.GetRelativePath();
  if(!xmlWriter.IsFileExtension(".mps", fileName))
    fileName += ".mps";
  
  if(xmlWriter.SaveSourceFiles()){
    PointSetWriter::Pointer writer = PointSetWriter::New();
    fileName = xmlWriter.GetAbsolutePath();
    if(!xmlWriter.IsFileExtension(".mps", fileName))
      fileName += ".mps";
    writer->SetFileName( fileName.c_str() );
    writer->SetInput( this );
    writer->Update();
  }
  xmlWriter.WriteProperty( XMLReader::FILENAME, fileName.c_str() );
  return true;
}

bool mitk::PointSet::ReadXMLData( XMLReader& xmlReader )
{
  BaseData::ReadXMLData( xmlReader );

  std::string fileName;
  xmlReader.GetAttribute( XMLReader::FILENAME, fileName );

  if ( fileName.empty() )
    return false;

  PointSetReader::Pointer reader = PointSetReader::New();
  reader->SetFileName( fileName.c_str() );
  reader->Update();
  mitk::PointSet::Pointer psp =
    dynamic_cast<mitk::PointSet*>( reader->GetOutput() );
  if (psp.IsNotNull())
  {
    m_PointSetSeries[0] = psp->GetPointSet();
  }

  if ( m_PointSetSeries[0].IsNull() )
  {
    return false;
  }

  return true;
}
