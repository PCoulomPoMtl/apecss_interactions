// This source file is part of APECSS, an open-source software toolbox
// for the computation of pressure-driven bubble dynamics and acoustic
// emissions in spherical symmetry.
//
// Copyright (C) 2022 The APECSS Developers
//
// The APECSS Developers are listed in the README.md file available in
// the GitHub repository at https://github.com/polycfd/apecss.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// -------------------------------------------------------------------
// APECSS standalone example for an ultrasound-driven bubble.
// -------------------------------------------------------------------

#include <time.h>
#include "apecss.h"

int main(int argc, char **args)
{
  char str[APECSS_STRINGLENGTH_SPRINTF];
  char OptionsDir[APECSS_STRINGLENGTH];

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Initialize the case-dependent simulation parameters
  double tEnd = 0.0;
  double fa = 0.0;
  double pa = 0.0;
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  apecss_infoscreen();

  /* Read commandline options */
  sprintf(OptionsDir, "./run.apecss");
  int i = 1;  // First argument is the call to the executable
  while (i < argc)
  {
    if (strcmp("-options", args[i]) == 0)
    {
      sprintf(OptionsDir, "%s", args[i + 1]);
      i += 2;
    }
    else if (strcmp("-tend", args[i]) == 0)
    {
      sscanf(args[i + 1], "%le", &tEnd);
      i += 2;
    }
    else if (strcmp("-freq", args[i]) == 0)
    {
      sscanf(args[i + 1], "%le", &fa);
      i += 2;
    }
    else if (strcmp("-amp", args[i]) == 0)
    {
      sscanf(args[i + 1], "%le", &pa);
      i += 2;
    }
    else
    {
      char str[APECSS_STRINGLENGTH_SPRINTF];
      sprintf(str, "Unknown command line options: %s", args[i]);
      apecss_erroronscreen(1, str);
      ++i;
    }
  }

  /* Allocate and initialize the Bubble structure */
  struct APECSS_Bubble *Bubble = (struct APECSS_Bubble *) malloc(sizeof(struct APECSS_Bubble));
  apecss_bubble_initializestruct(Bubble);

  /* Allocate and set the default options for the fluids */
  Bubble->Gas = (struct APECSS_Gas *) malloc(sizeof(struct APECSS_Gas));
  apecss_gas_setdefaultoptions(Bubble->Gas);
  Bubble->Liquid = (struct APECSS_Liquid *) malloc(sizeof(struct APECSS_Liquid));
  apecss_liquid_setdefaultoptions(Bubble->Liquid);
  Bubble->Interface = (struct APECSS_Interface *) malloc(sizeof(struct APECSS_Interface));
  apecss_interface_setdefaultoptions(Bubble->Interface);

  /* Set default options for the bubble and the fluids */
  apecss_bubble_setdefaultoptions(Bubble);

  /* Read the options file */
  apecss_options_readfile(Bubble, OptionsDir);

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Set the case-dependent simulation parameters
  Bubble->tStart = 0.0;
  Bubble->tEnd = (APECSS_FLOAT) tEnd;
  Bubble->dt = APECSS_MIN(1.0e-7, Bubble->tEnd - Bubble->tStart);  // Initial time-step
  Bubble->Excitation = (struct APECSS_Excitation *) malloc(sizeof(struct APECSS_Excitation));
  Bubble->Excitation->type = APECSS_EXCITATION_SIN;
  Bubble->Excitation->f = (APECSS_FLOAT) fa;
  Bubble->Excitation->dp = (APECSS_FLOAT) pa;
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  /* Process all options */
  apecss_gas_processoptions(Bubble->Gas);
  apecss_interface_processoptions(Bubble->Interface);
  apecss_liquid_processoptions(Bubble->Liquid);
  apecss_bubble_processoptions(Bubble);

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Set the function pointers for the progress bar
  if (Bubble->Emissions != NULL)
  {
    // Optional! Displays a progress bar during the simulation, here only
    // if emissions are calculated.
    Bubble->progress_initial = apecss_bubble_solver_progress_initialscreen;
    Bubble->progress_update = apecss_bubble_solver_progress_updatescreen;
    Bubble->progress_final = apecss_bubble_solver_progress_finalscreen;
  }
  else
  {
    Bubble->progress_initial = apecss_bubble_solver_progress_initialnone;
    Bubble->progress_update = apecss_bubble_solver_progress_updatenone;
    Bubble->progress_final = apecss_bubble_solver_progress_finalnone;
  }
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  /* Initialize the bubble based on the selected options */
  apecss_bubble_initialize(Bubble);

  /* Solve the bubble dynamics */
  clock_t starttimebubble = clock();
  apecss_bubble_solver_initialize(Bubble);
  apecss_bubble_solver_run(Bubble->tEnd, Bubble);
  apecss_bubble_solver_finalize(Bubble);

  sprintf(str, "Solver concluded %i time-steps and %i sub-iterations in %.3f s.", Bubble->dtNumber, Bubble->nSubIter,
          (double) (clock() - starttimebubble) / CLOCKS_PER_SEC);
  apecss_writeonscreen(str);

  /* Write out all desired results */
  apecss_results_rayleighplesset_write(Bubble);
  apecss_results_emissionsspace_write(Bubble);
  apecss_results_emissionsnodespecific_write(Bubble);
  apecss_results_emissionsnodeminmax_write(Bubble);

  /* Make sure all allocated memory is freed */
  apecss_bubble_freestruct(Bubble);

  return (0);
}