/*=========================================================================

Program:   Medical Imaging & Interaction Toolkit
Language:  C++
Date:      $Date$
Version:   $Revision: 18029 $

Copyright (c) German Cancer Research Center, Division of Medical and
Biological Informatics. All rights reserved.
See MITKCopyright.txt or http://www.mitk.org/copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/


#include "mitkPlanarPolygon.h"
#include "mitkGeometry2D.h"
#include "mitkProperties.h"


mitk::PlanarPolygon::PlanarPolygon()
: FEATURE_ID_CIRCUMFERENCE( this->AddFeature( "Circumference", "mm" ) ),
  FEATURE_ID_AREA( this->AddFeature( "Area", "mm^2" ) )
{
  // Polygon has at least two control points
  this->ResetNumberOfControlPoints( 2 );

  // Polygon is closed by default
  this->SetProperty( "closed", mitk::BoolProperty::New( true ) );

  m_PolyLines->InsertElement( 0, VertexContainerType::New());
}


mitk::PlanarPolygon::~PlanarPolygon()
{
}


void mitk::PlanarPolygon::SetClosed( bool closed )
{
  this->SetProperty( "closed", mitk::BoolProperty::New( closed ) );

  if ( !closed )
  {
    // For non-closed polygons: use "Length" as feature name; disable area
    this->SetFeatureName( FEATURE_ID_CIRCUMFERENCE, "Length" );
    this->DeactivateFeature( FEATURE_ID_AREA );
  }
  else
  {
    // For closed polygons: use "Circumference" as feature name; enable area
    this->SetFeatureName( FEATURE_ID_CIRCUMFERENCE, "Circumference" );
    this->ActivateFeature( FEATURE_ID_AREA );
  }

  this->Modified();
}


//void mitk::PlanarPolygon::Initialize()
//{
//  // Default initialization of circle control points
//
//  mitk::Geometry2D *geometry2D = 
//    dynamic_cast< mitk::Geometry2D * >( this->GetGeometry( 0 ) );
//
//  if ( geometry2D == NULL )
//  {
//    MITK_ERROR << "Missing Geometry2D for PlanarCircle";
//    return;
//  }
//
//  mitk::ScalarType width = geometry2D->GetBounds()[1];
//  mitk::ScalarType height = geometry2D->GetBounds()[3];
//  
//  mitk::Point2D &centerPoint = m_ControlPoints->ElementAt( 0 );
//  mitk::Point2D &boundaryPoint = m_ControlPoints->ElementAt( 1 );
//
//  centerPoint[0] = width / 2.0;
//  centerPoint[1] = height / 2.0;
//
//  boundaryPoint[0] = centerPoint[0] + 20.0;
//  boundaryPoint[1] = centerPoint[1];
//}


void mitk::PlanarPolygon::GeneratePolyLine()
{
  // if more elements are needed that have been reserved -> reserve
  if ( m_PolyLines->ElementAt( 0 )->size() < this->GetNumberOfControlPoints() )
  {
    m_PolyLines->ElementAt( 0 )->Reserve( this->GetNumberOfControlPoints() );
  }
  // if more elements have been reserved/set before than are needed now -> clear vector
  else if (m_PolyLines->ElementAt( 0 )->size() > this->GetNumberOfControlPoints())
  {
    m_PolyLines->ElementAt( 0 )->clear();
  }


  // TODO: start polygon at specified initalize point...
  m_PolyLines->ElementAt( 0 )->Reserve( this->GetNumberOfControlPoints() );
  for ( unsigned int i = 0; i < this->GetNumberOfControlPoints(); ++i )
  {
    m_PolyLines->ElementAt( 0 )->ElementAt( i ) = m_ControlPoints->ElementAt( i );  
  }
}

void mitk::PlanarPolygon::GenerateHelperPolyLine(double /*mmPerDisplayUnit*/, unsigned int /*displayHeight*/)
{
  // A polygon does not require helper objects
}

void mitk::PlanarPolygon::EvaluateFeaturesInternal()
{
  // Calculate circumference
  double circumference = 0.0;
  unsigned int i;
  for ( i = 0; i < this->GetNumberOfControlPoints() - 1; ++i )
  {
    circumference += this->GetWorldControlPoint( i ).EuclideanDistanceTo( 
      this->GetWorldControlPoint( i + 1 ) );
  }

  if ( this->IsClosed() )
  {
    circumference += this->GetWorldControlPoint( i ).EuclideanDistanceTo(
      this->GetWorldControlPoint( 0 ) );
  }

  this->SetQuantity( FEATURE_ID_CIRCUMFERENCE, circumference );


  // Calculate polygon area (if closed)
  double area = 0.0;
  if ( this->IsClosed() && (this->GetGeometry2D() != NULL) )
  {
    for ( i = 0; i < this->GetNumberOfControlPoints(); ++i )
    {
      Point2D p0 = this->GetControlPoint( i );
      Point2D p1 = this->GetControlPoint( (i + 1) % this->GetNumberOfControlPoints() );

      area += p0[0] * p1[1] - p1[0] * p0[1];
    }
    area /= 2.0;
  }
  
  this->SetQuantity( FEATURE_ID_AREA, fabs( area ) );

}


void mitk::PlanarPolygon::PrintSelf( std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf( os, indent );

  if (this->IsClosed())
    os << indent << "Polygon is closed\n";
  else
    os << indent << "Polygon is not closed\n";
}