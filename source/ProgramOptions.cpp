/*
 * ProgramOptions.cpp
 *
 *  Copyright (C) 2008-2012 Lars Otten
 *  This file is part of DAOOPT.
 *
 *  DAOOPT is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  DAOOPT is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with DAOOPT.  If not, see <http://www.gnu.org/licenses/>.
 *  
 *  Created on: Nov 15, 2008
 *      Author: Lars Otten <lotten@ics.uci.edu>
 */

#include "ProgramOptions.h"

using namespace std;

ProgramOptions* parseCommandLine(int ac, char** av) {

  ProgramOptions* opt = new ProgramOptions;

  // executable name
  opt->executableName = av[0];

  try{

    po::options_description desc("Valid options");
    desc.add_options()
      ("input-file,f", po::value<string>(), "path to problem file (required)")
      ("evid-file,e", po::value<string>(), "path to optional evidence file")
      ("ordering,o", po::value<string>(), "read elimination ordering from this file (first to last)")
      ("adaptive", "enable adaptive ordering scheme")
      ("maxTime", po::value<int>()->default_value(numeric_limits<int>::max()), "timeout threshold (seconds)")
      ("minibucket", po::value<string>(), "path to read/store mini bucket heuristic")
      ("subproblem,s", po::value<string>(), "limit search to subproblem specified in file")
      ("suborder,r",po::value<int>()->default_value(0), "subproblem order (0:width-inc 1:width-dec 2:heur-inc 3:heur-dec)")
      ("sol-file,c", po::value<string>(), "path to output optimal solution to")
      ("out-bound-file", po::value<string>(), "path to output current best solution to")
      ("ibound,i", po::value<int>()->default_value(10), "i-bound for mini bucket heuristics")
      ("cbound,j", po::value<int>()->default_value(1000), "context size bound for caching")
      ("mplp", po::value<int>()->default_value(-1), "use MPLP mini buckets (#iter)")
      ("mplps", po::value<double>()->default_value(-1), "use MPLP mini buckets (sec)")
      ("mplpt", po::value<double>()->default_value(1e-7), "convergence tolerance for MPLP")
      ("jglp", po::value<int>()->default_value(-1), "use Join-Graph reparameterization (#iter)")
      ("jglps", po::value<double>()->default_value(-1), "use Join-Graph reparameterization (sec)")
      ("jglpi", po::value<int>()->default_value(-1), "Specify the i-bound for JGLP")
      ("fglpHeur","use pure FGLP heuristic")
      ("fglpMBEHeur","use FGLP/MBE hybrid heuristic")
      ("fglpMBEHeurChoice","use FGLP/MBE choice heuristic")
      ("useShiftedLabels","use shifted labels induced by FGLP")
      ("useNullaryShift","use FGLP update that shifts maximums into a nullary function")
      ("usePriority","use prioritized FGLP update schedule")
      ("ndfglp", po::value<int>()->default_value(-1), "iterations for computing FGLP at every node")
      ("ndfglps", po::value<double>()->default_value(-1), "time for computing FGLP at every node")
      ("ndfglpt", po::value<double>()->default_value(1e-7), "convergence tolerance for FGLP at every node")
      ("cutoff-depth,d", po::value<int>()->default_value(0), "number of variables to test heuristic")
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
      ("cbound-worker,k", po::value<int>()->default_value(1000), "context size bound for caching in worker nodes")
#else
      ("rotate,y", "use breadth-rotating AOBB")
      ("rotatelimit,z", po::value<int>()->default_value(1000), "nodes per subproblem stack rotation (0: disabled)")
#endif
      ("orderIter,t", po::value<int>()->default_value(25), "iterations for finding ordering")
      ("orderTime", po::value<int>()->default_value(-1), "maximum time for finding ordering")
      ("orderTolerance", po::value<int>()->default_value(0), "allowed deviation from minfill suggested optimal")
      ("max-width", po::value<int>(), "max. induced width to process, abort otherwise")
#if defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC
      ("cutoff-depth,d", po::value<int>()->default_value(-1), "cutoff depth for central search")
      ("cutoff-width,w", po::value<int>()->default_value(-1), "cutoff width for central search")
      ("cutoff-size,l", po::value<int>()->default_value(-1), "subproblem size cutoff for central search (* 10^5)")
      ("local-size,u", po::value<int>()->default_value(-1), "minimum subproblem size (* 10^5)")
      ("init-nodes,x", po::value<int>()->default_value(-1), "number of nodes (*10^5) for local initialization")
      ("local", "solve all parallel subproblems locally")
      ("noauto", "don't determine cutoff automatically")
      ("procs,p", po::value<int>()->default_value(-1), "max. number of concurrent subproblem processes")
      ("max-sub", po::value<int>()->default_value(-1), "only generate the first few subproblems (for testing)")
      ("tag", po::value<string>(), "tag of the parallel run (to differentiate filenames etc.)")
#endif
#ifdef PARALLEL_STATIC
      ("pre", "perform preprocessing and generate subproblems only")
      ("post", "read previously solved subproblems and compile solution")
      ("sampledepth", po::value<int>()->default_value(10), "Randomness branching depth for initial sampling")
      ("samplesizes", po::value<string>(), "Sequence of sample sizes for complexity prediction (in 10^5 nodes)")
      ("samplerepeat", po::value<int>()->default_value(1), "Number of sample sequence repeats")
      ("lookahead", po::value<int>()->default_value(5), "AOBB subproblem lookahead factor (multiplied by no. of problem variables)")
#endif
      ("bound-file,b", po::value<string>(), "file with initial lower bound on solution cost")
      ("initial-bound", po::value<double>(), "initial lower bound on solution cost" )
#ifdef ENABLE_SLS
      ("slsX", po::value<int>()->default_value(0), "Number of initial SLS iterations")
      ("slsT", po::value<int>()->default_value(5), "Time per SLS iteration")
#endif
      ("lds,a",po::value<int>()->default_value(-1), "run initial LDS search with given limit (-1: disabled)")
      ("memlimit,m", po::value<int>()->default_value(-1), "approx. memory limit for mini buckets (in MByte)")
      ("seed", po::value<int>(), "seed for random number generator, time() otherwise")
      ("or", "use OR search (build pseudo tree as chain)")
      ("nocaching", "disable context-based caching during search")
      ("nosearch,n", "perform preprocessing, output stats, and exit")
      ("match", "use moment-matching during MBE")
      ("dynamic", "use dynamic mini-bucket heuristics")
      ("dynmm", "uses dynamic moment-matching heuristic which maintains the tree structure")
#if not (defined PARALLEL_DYNAMIC || defined PARALLEL_STATIC)
      ("reduce", po::value<string>(), "path to output the reduced network to (removes evidence and unary variables)")
      ("collapse", "collapse functions with identical scopes onto each other")
      ("perturb",po::value<double>()->default_value(0), "set all zero values to this value")
      ("cvo", "use Kalev Kask's CVO ordering")
#endif
      ("pst-file", po::value<string>(), "path to output the pseudo tree to, for plotting")
      ("help,h", "produces this help message")
      ;

    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      cout << endl << desc << endl;
      exit(0);
    }

    if (!vm.count("input-file")) {
      cout << "No or invalid arguments given, "
	   << "call with '" << av[0] << " --help' for full list."  << endl;
      return NULL;
    }

    opt->in_problemFile = vm["input-file"].as<string>();

    if (vm.count("evid-file"))
      opt->in_evidenceFile = vm["evid-file"].as<string>();

    if (vm.count("ordering"))
      opt->in_orderingFile = vm["ordering"].as<string>();

    if (vm.count("adaptive"))
      opt->autoIter = true;
    else
      opt->autoIter = false;

    if (vm.count("maxTime"))
      opt->maxTime = vm["maxTime"].as<int>();

    if (vm.count("subproblem"))
      opt->in_subproblemFile = vm["subproblem"].as<string>();

    if (vm.count("sol-file"))
      opt->out_solutionFile = vm["sol-file"].as<string>();
    
    if (vm.count("out-bound-file"))
      opt->out_boundFile = vm["out-bound-file"].as<string>();

    if (vm.count("minibucket"))
      opt->in_minibucketFile = vm["minibucket"].as<string>();

    if (vm.count("suborder")) {
      opt->subprobOrder = vm["suborder"].as<int>();
      if (opt->subprobOrder < 0 || opt->subprobOrder > 3) {
        cout << endl << desc << endl;
        exit(0);
      }
    }

    if (vm.count("ibound"))
      opt->ibound = vm["ibound"].as<int>();

    if (vm.count("cbound")) {
      opt->cbound = vm["cbound"].as<int>();
      opt->cbound_worker = vm["cbound"].as<int>();
    }

    if (vm.count("mplp"))
        opt->mplp = vm["mplp"].as<int>();
    if (vm.count("mplps"))
        opt->mplps = vm["mplps"].as<double>();
    if (vm.count("mplpt"))
        opt->mplpt = vm["mplpt"].as<double>();
    if (vm.count("jglp"))
        opt->jglp = vm["jglp"].as<int>();
    if (vm.count("jglps"))
        opt->jglps = vm["jglps"].as<double>();
    if (vm.count("jglpi"))
        opt->jglpi = vm["jglpi"].as<int>();

    if (vm.count("fglpHeur"))
        opt->fglpHeur = true;
    else
        opt->fglpHeur = false;

    if (vm.count("fglpMBEHeur"))
        opt->fglpMBEHeur = true;
    else
        opt->fglpMBEHeur = false;

    if (vm.count("fglpMBEHeurChoice"))
        opt->fglpMBEHeurChoice = true;
    else
        opt->fglpMBEHeurChoice = false;

    if (vm.count("useShiftedLabels"))
        opt->useShiftedLabels = true;
    else
        opt->useShiftedLabels = false;
    
    if (vm.count("useNullaryShift"))
        opt->useNullaryShift = true;
    else
        opt->useNullaryShift = false;

    if (vm.count("usePriority"))
        opt->usePriority = true;
    else
        opt->usePriority = false;

    if (vm.count("ndfglp"))
        opt->ndfglp = vm["ndfglp"].as<int>();
    if (vm.count("ndfglps")) {
        opt->ndfglps = vm["ndfglps"].as<double>();
    }
    if (vm.count("ndfglpt")) {
        opt->ndfglpt = vm["ndfglpt"].as<double>();
    }

    if (vm.count("cbound-worker"))
      opt->cbound_worker = vm["cbound-worker"].as<int>();

    if (vm.count("orderIter"))
      opt->order_iterations = vm["orderIter"].as<int>();
    if (vm.count("orderTime"))
      opt->order_timelimit = vm["orderTime"].as<int>();
    if (vm.count("orderTolerance"))
      opt->order_tolerance = vm["orderTolerance"].as<int>();

    if (vm.count("max-width"))
      opt->maxWidthAbort = vm["max-width"].as<int>();

    if (vm.count("cutoff-depth"))
      opt->cutoff_depth = vm["cutoff-depth"].as<int>();

    if (vm.count("cutoff-width"))
      opt->cutoff_width = vm["cutoff-width"].as<int>();

    if (vm.count("cutoff-size"))
      opt->cutoff_size = vm["cutoff-size"].as<int>();

    if (vm.count("local-size"))
      opt->local_size = vm["local-size"].as<int>();

    if (vm.count("init-nodes"))
      opt->nodes_init = vm["init-nodes"].as<int>();

    if (vm.count("noauto"))
      opt->autoCutoff = false;
    else
      opt->autoCutoff = true;

    if (vm.count("procs"))
      opt->threads = vm["procs"].as<int>();

    if (vm.count("max-sub"))
      opt->maxSubprob = vm["max-sub"].as<int>();

    if (vm.count("bound-file"))
      opt->in_boundFile = vm["bound-file"].as<string>();

    if (vm.count("initial-bound"))
      opt->initialBound = vm["initial-bound"].as<double>();

    if (vm.count("lds"))
      opt->lds = vm["lds"].as<int>();

    if (vm.count("slsX"))
      opt->slsIter = vm["slsX"].as<int>();
    if (vm.count("slsT"))
      opt->slsTime = vm["slsT"].as<int>();

    if (vm.count("memlimit"))
      opt->memlimit = vm["memlimit"].as<int>();

    if (vm.count("or"))
      opt->orSearch = true;
    else
      opt->orSearch = false;

    if (vm.count("nosearch"))
      opt->nosearch = true;
    else
      opt->nosearch = false;

    if (vm.count("nocaching"))
      opt->nocaching = true;
    else
      opt->nocaching = false;

    if (vm.count("match"))
      opt->match = true;
    else
      opt->match = false;

    if (vm.count("rotate"))
      opt->rotate = true;
    if (vm.count("rotatelimit"))
      opt->rotateLimit = vm["rotatelimit"].as<int>();

    if (vm.count("seed"))
      opt->seed = vm["seed"].as<int>();

    if (vm.count("tag"))
      opt->runTag = vm["tag"].as<string>();

    if (vm.count("local"))
      opt->par_solveLocal = true;
    if (vm.count("pre"))
      opt->par_preOnly = true;
    else if (vm.count("post"))
      opt->par_postOnly = true;

    if (vm.count("sampledepth"))
      opt->sampleDepth = vm["sampledepth"].as<int>();
    if (vm.count("samplesizes"))
      opt->sampleSizes = vm["samplesizes"].as<string>();
    if (vm.count("samplerepeat"))
      opt->sampleRepeat = vm["samplerepeat"].as<int>();

    if (vm.count("lookahead"))
      opt->aobbLookahead = vm["lookahead"].as<int>();

    if (vm.count("reduce"))
      opt->out_reducedFile = vm["reduce"].as<string>();

    if (vm.count("collapse"))
      opt->collapse = true;
    else
      opt->collapse = false;

    if (vm.count("perturb"))
      opt->perturb = vm["perturb"].as<double>();

    if (vm.count("cvo"))
      opt->order_cvo = true;
    else
      opt->order_cvo = false;

    if (vm.count("pst-file"))
      opt->out_pstFile = vm["pst-file"].as<string>();

    if (vm.count("subproblem") && !vm.count("ordering")) {
      cerr << "Error: Specifying a subproblem requires reading a fixed ordering from file." << endl;
      exit(1);
    }

    {
      //Extract the problem name
      string& fname = opt->in_problemFile;
      size_t len, start, pos1, pos2;
#if defined(WIN32)
      pos1 = fname.find_last_of("\\");
#elif defined(LINUX)
      pos1 = fname.find_last_of("/");
#endif
      pos2 = fname.find_last_of(".");
      if (pos1 == string::npos) { len = pos2; start = 0; }
      else { len = (pos2-pos1-1); start = pos1+1; }
      opt->problemName = fname.substr(start, len);
    }

  } catch (exception& e) {
    cerr << "error: " << e.what() << endl;
    return NULL;
  } catch (...) {
    cerr << "Exception of unknown type!" << endl;
    exit(1);
  }

  return opt;

}

