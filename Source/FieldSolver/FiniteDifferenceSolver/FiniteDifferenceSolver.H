/* Copyright 2020 Remi Lehe
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */

#ifndef WARPX_FINITE_DIFFERENCE_SOLVER_H_
#define WARPX_FINITE_DIFFERENCE_SOLVER_H_

#include "EmbeddedBoundary/WarpXFaceInfoBox.H"
#include "FiniteDifferenceSolver_fwd.H"

#include "BoundaryConditions/PML_fwd.H"
#include "MacroscopicProperties/MacroscopicProperties_fwd.H"

#include <AMReX_GpuContainers.H>
#include <AMReX_REAL.H>

#include <AMReX_BaseFwd.H>

#include <array>
#include <memory>

/**
 * \brief Top-level class for the electromagnetic finite-difference solver
 *
 * Stores the coefficients of the finite-difference stencils,
 * and has member functions to update fields over one time step.
 */
class FiniteDifferenceSolver
{
    public:

        // Constructor
        /** \brief Initialize the finite-difference Maxwell solver (for a given refinement level)
         *
         * This function initializes the stencil coefficients for the chosen finite-difference algorithm
         *
         * \param fdtd_algo Identifies the chosen algorithm, as defined in WarpXAlgorithmSelection.H
         * \param cell_size Cell size along each dimension, for the chosen refinement level
         * \param do_nodal  Whether the solver is applied to a nodal or staggered grid
         */
        FiniteDifferenceSolver (
            int const fdtd_algo,
            std::array<amrex::Real,3> cell_size,
            bool const do_nodal );

        void EvolveBLondon ( std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& current,
                       std::unique_ptr<amrex::MultiFab> const& Gfield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& face_areas,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& area_mod,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 >& ECTRhofield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Venl,
                       std::array< std::unique_ptr<amrex::iMultiFab>, 3 >& flag_info_cell,
                       std::array< std::unique_ptr<amrex::LayoutData<FaceInfoBox> >, 3 >& borrowing,
                       int lev, amrex::Real const dt, amrex::Real const penetration_depth );

        void EvolveB ( std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
                       std::unique_ptr<amrex::MultiFab> const& Gfield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& face_areas,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& area_mod,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 >& ECTRhofield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Venl,
                       std::array< std::unique_ptr<amrex::iMultiFab>, 3 >& flag_info_cell,
                       std::array< std::unique_ptr<amrex::LayoutData<FaceInfoBox> >, 3 >& borrowing,
                       int lev, amrex::Real const dt );

        void EvolveE ( std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Efield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Bfield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Jfield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& edge_lengths,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& face_areas,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 >& ECTRhofield,
                       std::unique_ptr<amrex::MultiFab> const& Ffield,
                       int lev, amrex::Real const dt );

        void EvolveF ( std::unique_ptr<amrex::MultiFab>& Ffield,
                       std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
                       std::unique_ptr<amrex::MultiFab> const& rhofield,
                       int const rhocomp,
                       amrex::Real const dt );

        void EvolveG (std::unique_ptr<amrex::MultiFab>& Gfield,
                      std::array<std::unique_ptr<amrex::MultiFab>,3> const& Bfield,
                      amrex::Real const dt);

        void EvolveECTRho ( std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
                            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& edge_lengths,
                            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& face_areas,
                            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& ECTRhofield,
                            const int lev );

        void ApplySilverMuellerBoundary(
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Efield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
            amrex::Box domain_box,
            amrex::Real const dt,
            amrex::Vector<int> field_boundary_lo,
            amrex::Vector<int> field_boundary_hi);

        void ComputeDivE ( const std::array<std::unique_ptr<amrex::MultiFab>,3>& Efield,
                           amrex::MultiFab& divE );

        /**
          * \brief Macroscopic E-update for non-vacuum medium using the user-selected
          * finite-difference algorithm and macroscopic sigma-method defined in
          * WarpXAlgorithmSelection.H
          *
          * \param[out] Efield  vector of electric field MultiFabs updated at a given level
          * \param[in] Bfield   vector of magnetic field MultiFabs at a given level
          * \param[in] Jfield   vector of current density MultiFabs at a given level
          * \param[in] dt       timestep of the simulation
          * \param[in] macroscopic_properties contains user-defined properties of the medium.
          */

        void MacroscopicEvolveE ( std::array< std::unique_ptr<amrex::MultiFab>, 3>& Efield,
#ifndef WARPX_MAG_LLG
                            std::array< std::unique_ptr<amrex::MultiFab>, 3> const& Bfield,
#else
                            std::array< std::unique_ptr<amrex::MultiFab>, 3> const& Hfield,
#endif
                            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Jfield,
                            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& edge_lengths,
                            amrex::Real const dt,
                            std::unique_ptr<MacroscopicProperties> const& macroscopic_properties);
#ifndef WARPX_DIM_RZ
#ifdef WARPX_MAG_LLG
        /**
          * \brief Macroscopic M-update, H-update and B=mu_o(H+M) computation for non-vacuum medium using finite-difference algorithm
          * solving Landau-Lifshitz-Gilbert (LLG) equation,
          * only Yee's algorithm is applicable for M calculation
          * These functions have first- or second- order accuracy with forward-Euler or iterative trapezoidal method
          * \param[out] Mfield   vector of magnetization MultiFabs updated at a given level; each MultiFab locates
          * on the face centers of the spatial cell; and each MultiFab contains three four-dimensional FabArrays
          * indicating the x, y, z locations and the field component
          * \param[out] Hfield   vector of magnetic field intensity MultiFabs at a given level
          * \param[out] Bfield   vector of magnetic flux density MultiFabs at a given level
          * \param[in] H_biasfield   vector of user-defined DC magnetic bias field MultiFabs at a given level
          * \param[in] Efield   vector of electric field MultiFabs at a given level
          * \param[in] dt   timestep of the simulation
          * \param[in] macroscopic_properties   contains user-defined properties of the medium.
          */

        void MacroscopicEvolveHM (
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> &Mfield,
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> &Hfield,            // H Maxwell
                       std::array< std::unique_ptr<amrex::MultiFab>, 3>& Bfield,
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> const &H_biasfield, // H bias
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> const &Efield,
                       amrex::Real const dt,
                       std::unique_ptr<MacroscopicProperties> const &macroscopic_properties);

        void MacroscopicEvolveHM_2nd (
                       int lev,
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> &Mfield,
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> &Hfield,            // H Maxwell
                       std::array< std::unique_ptr<amrex::MultiFab>, 3>& Bfield,
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> const &H_biasfield, // H bias
                       std::array<std::unique_ptr<amrex::MultiFab>, 3> const &Efield,
                       amrex::Real const dt,
                       std::unique_ptr<MacroscopicProperties> const &macroscopic_properties);

#endif
#endif // ifndef WARPX_DIM_RZ

        void EvolveBPML ( std::array< amrex::MultiFab*, 3 > Bfield,
                      std::array< amrex::MultiFab*, 3 > const Efield,
                      amrex::Real const dt,
                      const bool dive_cleaning);

       void EvolveEPML ( std::array< amrex::MultiFab*, 3 > Efield,
                      std::array< amrex::MultiFab*, 3 > const Bfield,
                      std::array< amrex::MultiFab*, 3 > const Jfield,
                      std::array< amrex::MultiFab*, 3 > const edge_lengths,
                      amrex::MultiFab* const Ffield,
                      MultiSigmaBox const& sigba,
                      amrex::Real const dt, bool pml_has_particles );

       void EvolveFPML ( amrex::MultiFab* Ffield,
                     std::array< amrex::MultiFab*, 3 > const Efield,
                     amrex::Real const dt );

       void MacroscopicEvolveEPML ( std::array< amrex::MultiFab*, 3 > Efield,
#ifndef WARPX_MAG_LLG
                      std::array< amrex::MultiFab*, 3 > const Bfield,
#else
                      std::array< amrex::MultiFab*, 3 > const Hfield,
#endif
                      std::array< amrex::MultiFab*, 3 > const Jfield,
                      amrex::MultiFab* const Ffield,
                      MultiSigmaBox const& sigba,
                      amrex::Real const dt, bool pml_has_particles,
                      std::unique_ptr<MacroscopicProperties> const& macroscopic_properties,
                      amrex::MultiFab* const eps_mf,
                      amrex::MultiFab* const mu_mf,
                      amrex::MultiFab* const sigma_mf);

#ifndef WARPX_DIM_RZ
#ifdef WARPX_MAG_LLG
        void EvolveHPML ( std::array< amrex::MultiFab*, 3 > Hfield,
                      std::array< amrex::MultiFab*, 3 > const Efield,
                      amrex::Real const dt,
                      const bool dive_cleaning);
#endif
#endif // ifndef WARPX_DIM_RZ

    private:

        int m_fdtd_algo;
        bool m_do_nodal;

#ifdef WARPX_DIM_RZ
        amrex::Real m_dr, m_rmin;
        int m_nmodes;
        // host-only
        amrex::Vector<amrex::Real> m_h_stencil_coefs_r, m_h_stencil_coefs_z;
        // device copy after init
        amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_r;
        amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_z;
#else
        // host-only
        amrex::Vector<amrex::Real> m_h_stencil_coefs_x, m_h_stencil_coefs_y, m_h_stencil_coefs_z;
        // device copy after init
        amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_x;
        amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_y;
        amrex::Gpu::DeviceVector<amrex::Real> m_stencil_coefs_z;
#endif

    public:
        // The member functions below contain extended __device__ lambda.
        // In order to compile with nvcc, they need to be public.

#ifdef WARPX_DIM_RZ
        template< typename T_Algo >
        void EvolveBCylindrical (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
            const int lev,
            amrex::Real const dt );

        template< typename T_Algo >
        void EvolveECylindrical (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Efield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Bfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Jfield,
            std::unique_ptr<amrex::MultiFab> const& Ffield,
            const int lev,
            amrex::Real const dt );

        template< typename T_Algo >
        void EvolveFCylindrical (
            std::unique_ptr<amrex::MultiFab>& Ffield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
            std::unique_ptr<amrex::MultiFab> const& rhofield,
            int const rhocomp,
            amrex::Real const dt );

        template< typename T_Algo >
        void ComputeDivECylindrical (
            const std::array<std::unique_ptr<amrex::MultiFab>,3>& Efield,
            amrex::MultiFab& divE );
#else
        template< typename T_Algo >
        void EvolveBLondonCartesian (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& current,
            std::unique_ptr<amrex::MultiFab> const& Gfield,
            int lev, amrex::Real const dt, amrex::Real const penetration_depth );

        template< typename T_Algo >
        void EvolveBCartesian (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
            std::unique_ptr<amrex::MultiFab> const& Gfield,
            int lev, amrex::Real const dt );

        template< typename T_Algo >
        void EvolveECartesian (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Efield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Bfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Jfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& edge_lengths,
            std::unique_ptr<amrex::MultiFab> const& Ffield,
            int lev, amrex::Real const dt );

        template< typename T_Algo >
        void EvolveFCartesian (
            std::unique_ptr<amrex::MultiFab>& Ffield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
            std::unique_ptr<amrex::MultiFab> const& rhofield,
            int const rhocomp,
            amrex::Real const dt );

        template< typename T_Algo >
        void EvolveGCartesian (
            std::unique_ptr<amrex::MultiFab>& Gfield,
            std::array<std::unique_ptr<amrex::MultiFab>,3> const& Bfield,
            amrex::Real const dt);

        void EvolveRhoCartesianECT (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& Efield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& edge_lengths,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& face_areas,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& ECTRhofield, int lev);

        void EvolveBCartesianECT (
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& face_areas,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& area_mod,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& ECTRhofield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Venl,
            std::array< std::unique_ptr<amrex::iMultiFab>, 3 >& flag_info_cell,
            std::array< std::unique_ptr<amrex::LayoutData<FaceInfoBox> >, 3 >& borrowing,
            int lev, amrex::Real const dt
        );

        template< typename T_Algo >
        void ComputeDivECartesian (
            const std::array<std::unique_ptr<amrex::MultiFab>,3>& Efield,
            amrex::MultiFab& divE );

        template< typename T_Algo, typename T_MacroAlgo >
        void MacroscopicEvolveECartesian (
            std::array< std::unique_ptr< amrex::MultiFab>, 3>& Efield,
#ifndef WARPX_MAG_LLG
            std::array< std::unique_ptr< amrex::MultiFab>, 3> const &Bfield,
#else
            std::array< std::unique_ptr< amrex::MultiFab>, 3> const &Hfield,
#endif
            std::array< std::unique_ptr< amrex::MultiFab>, 3> const& Jfield,
            std::array< std::unique_ptr<amrex::MultiFab>, 3 > const& edge_lengths,
            amrex::Real const dt,
            std::unique_ptr<MacroscopicProperties> const& macroscopic_properties);

#ifdef WARPX_MAG_LLG
        template< typename T_Algo >
        void MacroscopicEvolveHMCartesian(
            std::array<std::unique_ptr<amrex::MultiFab>, 3> &Mfield,
            std::array<std::unique_ptr<amrex::MultiFab>, 3> &Hfield,            // H Maxwell
            std::array< std::unique_ptr<amrex::MultiFab>, 3>& Bfield,
            std::array<std::unique_ptr<amrex::MultiFab>, 3> const &H_biasfield, // H bias
            std::array<std::unique_ptr<amrex::MultiFab>, 3> const &Efield,
            amrex::Real const dt,
            std::unique_ptr<MacroscopicProperties> const &macroscopic_properties);

        template< typename T_Algo >
        void MacroscopicEvolveHMCartesian_2nd(
            int lev,
            std::array<std::unique_ptr<amrex::MultiFab>, 3> &Mfield,
            std::array<std::unique_ptr<amrex::MultiFab>, 3> &Hfield,            // H Maxwell
            std::array< std::unique_ptr<amrex::MultiFab>, 3 >& Bfield,
            std::array<std::unique_ptr<amrex::MultiFab>, 3> const &H_biasfield, // H bias
            std::array<std::unique_ptr<amrex::MultiFab>, 3> const &Efield,
            amrex::Real const dt,
            std::unique_ptr<MacroscopicProperties> const &macroscopic_properties);
#endif

        template< typename T_Algo >
        void EvolveBPMLCartesian (
            std::array< amrex::MultiFab*, 3 > Bfield,
            std::array< amrex::MultiFab*, 3 > const Efield,
            amrex::Real const dt,
            const bool dive_cleaning);

        template< typename T_Algo >
        void EvolveEPMLCartesian (
            std::array< amrex::MultiFab*, 3 > Efield,
            std::array< amrex::MultiFab*, 3 > const Bfield,
            std::array< amrex::MultiFab*, 3 > const Jfield,
            std::array< amrex::MultiFab*, 3 > const edge_lengths,
            amrex::MultiFab* const Ffield,
            MultiSigmaBox const& sigba,
            amrex::Real const dt, bool pml_has_particles );

        template< typename T_Algo >
        void EvolveFPMLCartesian ( amrex::MultiFab* Ffield,
                      std::array< amrex::MultiFab*, 3 > const Efield,
                      amrex::Real const dt );

        template< typename T_Algo, typename T_MacroAlgo >
        void MacroscopicEvolveEPMLCartesian (
            std::array< amrex::MultiFab*, 3 > Efield,
#ifndef WARPX_MAG_LLG
            std::array< amrex::MultiFab*, 3 > const Bfield,
#else
            std::array< amrex::MultiFab*, 3 > const Hfield,
#endif
            std::array< amrex::MultiFab*, 3 > const Jfield,
            amrex::MultiFab* const Ffield,
            MultiSigmaBox const& sigba,
            amrex::Real const dt, bool pml_has_particles,
            std::unique_ptr<MacroscopicProperties> const& macroscopic_properties,
            amrex::MultiFab* const eps_mf,
            amrex::MultiFab* const mu_mf,
            amrex::MultiFab* const sigma_mf);


#ifdef WARPX_MAG_LLG
        template< typename T_Algo >
        void EvolveHPMLCartesian (
            std::array< amrex::MultiFab*, 3 > Bfield,
            std::array< amrex::MultiFab*, 3 > const Efield,
            amrex::Real const dt,
            const bool dive_cleaning);
#endif

#endif

};

#endif // WARPX_FINITE_DIFFERENCE_SOLVER_H_
