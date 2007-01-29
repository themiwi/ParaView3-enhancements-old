/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVAnimationScene.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationScene.h"

#include "vtkObjectFactory.h"

#include <vtkstd/set>

class vtkPVAnimationSceneSetOfDouble : public vtkstd::set<double> {};

vtkStandardNewMacro(vtkPVAnimationScene);
vtkCxxRevisionMacro(vtkPVAnimationScene, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkPVAnimationScene::vtkPVAnimationScene()
{
  this->TimeSteps = new vtkPVAnimationSceneSetOfDouble;
}

//-----------------------------------------------------------------------------
vtkPVAnimationScene::~vtkPVAnimationScene()
{
  delete this->TimeSteps;
}


//-----------------------------------------------------------------------------
void vtkPVAnimationScene::AddTimeStep(double time)
{
  this->TimeSteps->insert(time);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveTimeStep(double time)
{
  vtkPVAnimationSceneSetOfDouble::iterator iter =
    this->TimeSteps->find(time);
  if (iter != this->TimeSteps->end())
    {
    this->TimeSteps->erase(iter);
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveAllTimeSteps()
{
  this->TimeSteps->clear();
}

//-----------------------------------------------------------------------------
unsigned int vtkPVAnimationScene::GetNumberOfTimeSteps()
{
  return static_cast<unsigned int>(this->TimeSteps->size());
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::Play()
{
  if (this->PlayMode != PLAYMODE_TIMESTEPS)
    {
    this->Superclass::Play();
    return;
    }

  if (this->InPlay)
    {
    return;
    }

  if (this->TimeMode == vtkAnimationCue::TIMEMODE_NORMALIZED)
    {
    vtkErrorMacro("Cannot play a scene with normalized time mode");
    return;
    }
  if (this->EndTime <= this->StartTime)
    {
    vtkErrorMacro("Scene start and end times are not suitable for playing");
    return;
    }

  this->InPlay = 1;
  this->StopPlay = 0;

  double frame_rate = this->FrameRate? this->FrameRate : 1.0;

  do
    {
    this->Initialize(); // Set the Scene in unintialized mode.

    vtkPVAnimationSceneSetOfDouble::iterator iter = this->TimeSteps->lower_bound(this->StartTime);
    if (iter == this->TimeSteps->end())
      {
      break;
      }
    double deltatime = 0.0;
    do
      {
      this->Tick((*iter), deltatime);

      // needed to compute delta times.
      double previous_tick_time = (*iter);
      iter++;

      if (iter == this->TimeSteps->end())
        {
        break;
        }

      if (frame_rate > 1)
        {
        double increment = ((*iter)-previous_tick_time)/frame_rate;
        double itime = previous_tick_time+increment;
        for (int cc=0; cc < frame_rate-1; cc++)
          {
          this->Tick(itime, increment);
          previous_tick_time = itime;
          itime += increment;
          }
        }

      deltatime = (*iter) - previous_tick_time;
      deltatime = (deltatime < 0)? -1*deltatime : deltatime;
      } while (!this->StopPlay && this->CueState != vtkAnimationCue::INACTIVE);
    // End of loop for 1 cycle.

    } while (this->Loop && !this->StopPlay);

  this->StopPlay = 0;
  this->InPlay = 0;
}


//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetNextTimeStep(double timestep)
{
  vtkPVAnimationSceneSetOfDouble::iterator iter = 
    this->TimeSteps->upper_bound(timestep);
  if (iter == this->TimeSteps->end())
    {
    return timestep;
    }
  return (*iter);
}

//-----------------------------------------------------------------------------
double vtkPVAnimationScene::GetPreviousTimeStep(double timestep)
{
  double value = timestep;
  vtkPVAnimationSceneSetOfDouble::iterator iter = this->TimeSteps->begin();
  for (;iter != this->TimeSteps->end(); ++iter)
    {
    if ((*iter) >= timestep)
      {
      return value;
      }
    value = (*iter);
    }
  return value;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
