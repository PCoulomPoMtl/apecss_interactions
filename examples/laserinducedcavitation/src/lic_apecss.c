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
// APECSS standalone example for a laser-induced cavitation bubble,
// based on Liang et al., Journal of Fluid Mechanics 940 (2022), A5.
// -------------------------------------------------------------------

#include <time.h>
#include "apecss.h"

// Declaration of additional case-dependent functions
APECSS_FLOAT lic_gilmorevelocity_ode(APECSS_FLOAT *Sol, APECSS_FLOAT t, struct APECSS_Bubble *Bubble);
APECSS_FLOAT lic_particlevelocityderivative(APECSS_FLOAT *Sol, APECSS_FLOAT t, struct APECSS_Bubble *Bubble);
APECSS_FLOAT lic_equilibriumradius(APECSS_FLOAT t, struct APECSS_Bubble *Bubble);
APECSS_FLOAT lic_gas_pressure_hc(APECSS_FLOAT *Sol, struct APECSS_Bubble *Bubble);
APECSS_FLOAT lic_gas_pressurederivative_hc(APECSS_FLOAT *Sol, APECSS_FLOAT t, struct APECSS_Bubble *Bubble);

// Declaration of the structure holding the variables specific to the laser-induced cavitation case
struct LIC
{
  APECSS_FLOAT tauL, Rnbd, Rnc1, Rnc2, tmax1, tmax2;
};

int main(int argc, char **args)
{
  char str[APECSS_STRINGLENGTH_SPRINTF];
  char OptionsDir[APECSS_STRINGLENGTH];

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Initialize the case-dependent simulation parameters
  double tEnd = 0.0;

  // Allocate the structure holding the variables specific to the laser-induced cavitation case
  struct LIC *lic_data = (struct LIC *) malloc(sizeof(struct LIC));

  // Duration of the laser pulse (full-width at half-maximum) to model the 265 fs laser (see page 6 in Liang et al., JFM 940 (2022), A5)
  lic_data->tauL = 265.0e-15;

  // Equilibrium radius of the bubble with respect to maximum pressure generated by the laser pulse (see Table 1 in Liang et al., JFM 940 (2022), A5)
  lic_data->Rnbd = 13.718e-6;

  // Equilibrium radius of the bubble during the first collapse (see Table 1 in Liang et al., JFM 940 (2022), A5)
  lic_data->Rnc1 = 3.615e-6;

  // Equilibrium radius of the bubble during the second and subsequent collapses (see Table 1 in Liang et al., JFM 940 (2022), A5)
  lic_data->Rnc2 = 2.415e-6;

  // Time of the first maximum of the bubble radius (see Table 1 in Liang et al., JFM 940 (2022), A5)
  lic_data->tmax1 = 3.2440e-6;

  // Time of the second maximum of the bubble radius (see Table 1 in Liang et al., JFM 940 (2022), A5)
  lic_data->tmax2 = 7.2688e-6;
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  apecss_infoscreen();

  /* Read commandline options */
  sprintf(OptionsDir, "./run.apecss");  // This is the default
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
    else
    {
      char str[APECSS_STRINGLENGTH_SPRINTF];
      sprintf(str, "Unknown command line options: %s", args[i]);
      apecss_erroronscreen(1, str);
      ++i;
    }
  }

  /* Allocate and initialize Bubble structure */
  struct APECSS_Bubble *Bubble = (struct APECSS_Bubble *) malloc(sizeof(struct APECSS_Bubble));
  apecss_bubble_initializestruct(Bubble);

  /* Set default options and read the options for the bubble */
  apecss_bubble_setdefaultoptions(Bubble);
  apecss_bubble_readoptions(Bubble, OptionsDir);

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Hook case-dependent data structure to the void data pointer
  Bubble->user_data = lic_data;
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  /* Allocate the structures for the fluid properties and ODE solver parameters */
  struct APECSS_Gas *Gas = (struct APECSS_Gas *) malloc(sizeof(struct APECSS_Gas));
  struct APECSS_Liquid *Liquid = (struct APECSS_Liquid *) malloc(sizeof(struct APECSS_Liquid));
  struct APECSS_Interface *Interface = (struct APECSS_Interface *) malloc(sizeof(struct APECSS_Interface));
  struct APECSS_NumericsODE *NumericsODE = (struct APECSS_NumericsODE *) malloc(sizeof(struct APECSS_NumericsODE));

  /* Set the default options for the fluid properties and solver parameters  */
  apecss_gas_setdefaultoptions(Gas);
  apecss_liquid_setdefaultoptions(Liquid);
  apecss_interface_setdefaultoptions(Interface);
  apecss_odesolver_setdefaultoptions(NumericsODE);

  /* Read the options file for the fluid properties and solver parameters  */
  apecss_gas_readoptions(Gas, OptionsDir);
  apecss_liquid_readoptions(Liquid, OptionsDir);
  apecss_interface_readoptions(Interface, OptionsDir);
  apecss_odesolver_readoptions(NumericsODE, OptionsDir);

  /* Associate the bubble with the relevant fluid properties and solver parameters */
  Bubble->Gas = Gas;
  Bubble->Liquid = Liquid;
  Bubble->Interface = Interface;
  Bubble->NumericsODE = NumericsODE;

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Set the case-dependent simulation parameters
  Bubble->tStart = 0.0;
  Bubble->tEnd = (APECSS_FLOAT) tEnd;
  Bubble->dt = 1.0e-15;  // Initial time-step
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  /* Process all options */
  apecss_gas_processoptions(Gas);
  apecss_liquid_processoptions(Liquid);
  apecss_interface_processoptions(Interface);
  apecss_odesolver_processoptions(NumericsODE);
  apecss_bubble_processoptions(Bubble);

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Set the function pointers for the progress bar
  Bubble->progress_initial = apecss_bubble_solver_progress_initialscreen;
  Bubble->progress_update = apecss_bubble_solver_progress_updatescreen;
  Bubble->progress_final = apecss_bubble_solver_progress_finalscreen;

  // Change the function pointers to include the specific models used by Liang et al., JFM 940 (2022), A5
  Bubble->ode[0] = lic_gilmorevelocity_ode;  // Modified Gilmore model, see below.
  Gas->get_pressure = lic_gas_pressure_hc;  // Gas pressure based on the equilibrium radius
  Gas->get_pressurederivative = lic_gas_pressurederivative_hc;
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

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Freeing the structure holding the variables specific to the laser-induced cavitation case
  free(lic_data);
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  free(Bubble);
  free(Gas);
  free(Liquid);
  free(Interface);
  free(NumericsODE);

  return (0);
}

APECSS_FLOAT lic_gilmorevelocity_ode(APECSS_FLOAT *Sol, APECSS_FLOAT t, struct APECSS_Bubble *Bubble)
{
  // Gilmore model including the particle velocity generated by the laser, see Eq. (3.17) in Liang et al., JFM 940 (2022), A5

  APECSS_FLOAT pL = Bubble->Liquid->get_pressure_bubblewall(Sol, t, Bubble);
  APECSS_FLOAT pInf = Bubble->get_pressure_infinity(t, Bubble);
  APECSS_FLOAT rhoL = apecss_liquid_density_nasg(pL, Bubble->Liquid);
  APECSS_FLOAT rhoInf = apecss_liquid_density_nasg(pInf, Bubble->Liquid);
  APECSS_FLOAT H = apecss_liquid_enthalpy_nasg(pL, rhoL, Bubble->Liquid) - apecss_liquid_enthalpy_nasg(pInf, rhoInf, Bubble->Liquid);
  APECSS_FLOAT dot_Hexpl =
      Bubble->Liquid->get_pressurederivative_bubblewall_expl(Sol, t, Bubble) / rhoL - Bubble->get_pressurederivative_infinity(t, Bubble) / rhoInf;
  APECSS_FLOAT inv_cL = 1.0 / apecss_liquid_soundspeed_nasg(pL, rhoL, Bubble->Liquid);
  APECSS_FLOAT dot_pvisc_impl =
      Bubble->Liquid->get_pressurederivative_viscous_impl(Sol[1], Bubble) + Bubble->Interface->get_pressurederivative_viscous_impl(Sol[1], Bubble->Interface);
  APECSS_FLOAT GilmoreCoeffB = 1.0 + dot_pvisc_impl * inv_cL / rhoL;

  APECSS_FLOAT dotU_Gilmore =
      (((1.0 + Sol[0] * inv_cL) * H - 1.5 * (1.0 - Sol[0] * APECSS_ONETHIRD * inv_cL) * APECSS_POW2(Sol[0])) / ((1.0 - Sol[0] * inv_cL) * Sol[1]) +
       dot_Hexpl * inv_cL) /
      GilmoreCoeffB;

  return (dotU_Gilmore + lic_particlevelocityderivative(Sol, t, Bubble));
}

APECSS_FLOAT lic_particlevelocityderivative(APECSS_FLOAT *Sol, APECSS_FLOAT t, struct APECSS_Bubble *Bubble)
{
  // Derivative of the particle velocity according to Eq. (3.30) in Liang et al., JFM 940 (2022), A5

  struct LIC *lic_data = Bubble->user_data;

  if (t > 2.0 * lic_data->tauL)
  {
    return (0.0);
  }
  else
  {
    APECSS_FLOAT pinf = Bubble->get_pressure_infinity(t, Bubble);
    APECSS_FLOAT sigma = Bubble->Interface->get_surfacetension(Sol[1], Bubble);
    APECSS_FLOAT Rn = lic_equilibriumradius(t, Bubble);

    // Hugoniot parameters of Rice and Walsh (see page 11 in Liang et al., JFM 940 (2022), A5)
    APECSS_FLOAT c1 = 5190.0;
    APECSS_FLOAT c2 = 25306.0;

    // Pressure generated by the laser-induced breakdown
    APECSS_FLOAT P = (pinf * APECSS_POW4(Rn) + 2.0 * sigma * APECSS_POW3(Rn)) / APECSS_POW4(Bubble->R0);  // Eq. (3.19) in Liang et al., JFM 940 (2022), A5
    APECSS_FLOAT dotP = ((2.0 * pinf * Rn + 3.0 * sigma) / (3.0 * APECSS_POW4(Bubble->R0) * lic_data->tauL)) *
                        (APECSS_POW3(lic_data->Rnbd) - APECSS_POW3(Bubble->R0)) *
                        (1.0 - APECSS_COS(APECSS_PI * t / lic_data->tauL));  // Eq (3.22) in Liang et al., JFM 940 (2022), A5

    return (dotP / APECSS_SQRT(APECSS_POW2(Bubble->Liquid->rhoref) * APECSS_POW2(Bubble->Liquid->cref) +
                               4.0 * Bubble->Liquid->rhoref * c2 * P / (APECSS_LN_OF_10 * c1)));
  }
}

APECSS_FLOAT lic_equilibriumradius(APECSS_FLOAT t, struct APECSS_Bubble *Bubble)
{
  struct LIC *lic_data = Bubble->user_data;

  if (t < 2.0 * lic_data->tauL)  // Eq. (3.9) in Liang et al., JFM 940 (2022), A5
    return (APECSS_POW((APECSS_POW3(Bubble->R0) + (APECSS_POW3(lic_data->Rnbd) - APECSS_POW3(Bubble->R0)) *
                                                      (t - (lic_data->tauL / APECSS_PI) * APECSS_SIN(APECSS_PI * t / lic_data->tauL)) / (2.0 * lic_data->tauL)),
                       APECSS_ONETHIRD));
  else if (t < lic_data->tmax1)
    return (lic_data->Rnbd);
  else if (t < lic_data->tmax2)
    return (lic_data->Rnc1);
  else
    return (lic_data->Rnc2);
}

APECSS_FLOAT lic_gas_pressure_hc(APECSS_FLOAT *Sol, struct APECSS_Bubble *Bubble)
{
  // Gas pressure based on the equilibrium radius, see Eq. (3.3) in Liang et al., JFM 940 (2022), A5

  APECSS_FLOAT sigma = Bubble->Interface->get_surfacetension(Sol[1], Bubble);
  APECSS_FLOAT Rn = lic_equilibriumradius(Bubble->t, Bubble);

  // Definition of the hardcore radius based on the discussion in the last paragraph of page 15 in Liang et al., JFM 940 (2022), A5
  struct LIC *lic_data = Bubble->user_data;
  APECSS_FLOAT rhc = 0.0;
  if (Bubble->t > lic_data->tmax1) rhc = Rn / 9.0;

  return ((Bubble->p0 + 2.0 * sigma / Rn) * APECSS_POW((APECSS_POW3(Rn) - APECSS_POW3(rhc)) / (APECSS_POW3(Sol[1]) - APECSS_POW3(rhc)), Bubble->Gas->Gamma));
}

APECSS_FLOAT lic_gas_pressurederivative_hc(APECSS_FLOAT *Sol, APECSS_FLOAT t, struct APECSS_Bubble *Bubble)
{
  // Derivative of the gas pressure based on the equilibrium radius

  // Definition of the hardcore radius based on the discussion in the last paragraph of page 15 in Liang et al., JFM 940 (2022), A5
  struct LIC *lic_data = Bubble->user_data;
  APECSS_FLOAT rhc = 0.0;
  if (Bubble->t > lic_data->tmax1) rhc = lic_equilibriumradius(Bubble->t, Bubble) / 9.0;

  return (-3.0 * lic_gas_pressure_hc(Sol, Bubble) * Bubble->Gas->Gamma * APECSS_POW2(Sol[1]) * Sol[0] / (APECSS_POW3(Sol[1]) - APECSS_POW3(rhc)));
}
