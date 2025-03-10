/* Copyright 2021 Neil Zaim
 *
 * This file is part of WarpX.
 *
 * License: BSD-3-Clause-LBNL
 */

#include "FieldReduction.H"

#include "Utils/IntervalsParser.H"
#include "Utils/TextMsg.H"
#include "Utils/WarpXAlgorithmSelection.H"
#include "Utils/WarpXUtil.H"

#include <AMReX_Algorithm.H>
#include <AMReX_BLassert.H>
#include <AMReX_ParmParse.H>
#include <AMReX_Vector.H>

#include <algorithm>
#include <ostream>

#include <regex>

// constructor
FieldReduction::FieldReduction (std::string rd_name)
: ReducedDiags{rd_name}
{
    using namespace amrex::literals;

    // RZ coordinate is not working
#if (defined WARPX_DIM_RZ)
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(false,
        "FieldReduction reduced diagnostics does not work for RZ coordinate.");
#endif

    // read number of levels
    int nLevel = 0;
    amrex::ParmParse pp_amr("amr");
    pp_amr.query("max_level", nLevel);
    WARPX_ALWAYS_ASSERT_WITH_MESSAGE(nLevel == 0,
        "FieldReduction reduced diagnostics does not work with mesh refinement.");

    constexpr int noutputs = 1; // A single output in the Field reduction diagnostic
    // resize data array
    m_data.resize(noutputs, 0.0_rt);

    amrex::ParmParse pp_rd_name(rd_name);

    // read reduced function with parser
    std::string parser_string = "";
    Store_parserString(pp_rd_name,"reduced_function(x,y,z,Ex,Ey,Ez,Bx,By,Bz)",
                       parser_string);
    m_parser = std::make_unique<amrex::Parser>(
        makeParser(parser_string,{"x","y","z","Ex","Ey","Ez","Bx","By","Bz"}));

    // Replace all newlines and possible following whitespaces with a single whitespace. This
    // should avoid weird formatting when the string is written in the header of the output file.
    parser_string = std::regex_replace(parser_string, std::regex("\n\\s*"), " ");

    // read reduction type
    std::string reduction_type_string;
    pp_rd_name.get("reduction_type", reduction_type_string);
    m_reduction_type = GetAlgorithmInteger (pp_rd_name, "reduction_type");

    if (amrex::ParallelDescriptor::IOProcessor())
    {
        if ( m_IsNotRestart )
        {
            // open file
            std::ofstream ofs{m_path + m_rd_name + "." + m_extension, std::ofstream::out};
            // write header row
            int c = 0;
            ofs << "#";
            ofs << "[" << c++ << "]step()";
            ofs << m_sep;
            ofs << "[" << c++ << "]time(s)";
            ofs << m_sep;
            ofs << "[" << c++ << "]" + reduction_type_string + " of " + parser_string + " (SI units)";

            ofs << std::endl;
            // close file
            ofs.close();
        }
    }
}
// end constructor

// function that does an arbitrary reduction of the electromagnetic fields
void FieldReduction::ComputeDiags (int step)
{
    // Judge if the diags should be done
    if (!m_intervals.contains(step+1)) { return; }

    if (m_reduction_type == ReductionType::Maximum)
    {
        ComputeFieldReduction<amrex::ReduceOpMax>();
    }
    else if (m_reduction_type == ReductionType::Minimum)
    {
        ComputeFieldReduction<amrex::ReduceOpMin>();
    }
    else if (m_reduction_type == ReductionType::Sum)
    {
        ComputeFieldReduction<amrex::ReduceOpSum>();
    }
}
// end void FieldMaximum::ComputeDiags
