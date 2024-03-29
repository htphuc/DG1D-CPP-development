#include "TNT2.h"
#include "Grid.h"
#include "ReferenceElement.h"
#include "GridFunction.h"
#include "VectorGridFunction.h"
#include "Evolution.h"
#include "globals.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include "ConfigParams.h"
#include "DiffEq.h"
#include "TwoDVectorGridFunction.h"
#include "Modes.h"
#include "namespaces.h"
#include "orbit.h"
#include <complex>
#include "source_interface.h"
#include "WriteFile.h"

using namespace std;
using namespace layers;
using namespace orbit;
using namespace window;
using namespace source_interface;


//Initial condition options
void initialGaussian(TwoDVectorGridFunction<complex<double>>& uh, Grid& grd);
void initialSinusoid(TwoDVectorGridFunction<complex<double>>& uh, Grid& grd);
void initialSchwarzchild(TwoDVectorGridFunction<complex<double>>& uh, Grid& grd, DiffEq& eqn);


//Characterization of convergence, error using L2 norm
double LTwoError(Grid thegrid, TwoDVectorGridFunction<complex<double>>& uh0, 
                 TwoDVectorGridFunction<complex<double>>& uhend);

int main()
{

  //params is initialized at the beginning of this file by its 
  //inclusion in the file
  //modify such that hyperboloidal and tortoise boundaries
  // occur at element boundaries

  //setup the modes
  Modes lmmodes(params.modes.lmax);
  if(params.metric.schwarschild){
    double rmin = params.schw.p_orb / (1.0 + params.schw.ecc);
    double rmax = params.schw.p_orb / (1.0 - params.schw.ecc);
    double xip = 0.5*(rmin+rmax);
    Sminus = params.hyperb.Sminus;
    double rstar_orb = rstar_of_r(xip, params.schw.mass);
    
    double deltar = (rstar_orb - Sminus)* 2.0/params.grid.numelems; 
    Splus = rstar_orb +round(0.5* params.grid.numelems) *deltar;
    Rminus = rstar_orb 
      - round(0.175 * params.grid.numelems) * deltar;
    Rplus = rstar_orb 
      + round(0.125 * params.grid.numelems) * deltar;
    Wminus = Rminus + params.window.noffset * deltar;
    Wplus = Rplus - params.window.noffset * deltar;

    cout << "R_star orbit" << endl;
    cout << rstar_orb << endl << endl;
    
    cout << "Sminus Rminus Rplus Splus Wminus Wplus" << endl;
    cout << Sminus << " " << Rminus << " " 
	 << Rplus <<" " << Splus << " " 
	 << Wminus << " " << Wplus << endl;
    cout << endl;
  
    //initialize orbit
    initialize_orbit();
    cout << "p =" << p << endl;
    cout << "e =" << e << endl;
    cout << "chi =" << chi << endl;
    cout << "phi = " << phi << endl;
    cout << endl;

 
    if(params.opts.useSource) {
      init_source( lmmodes, params.schw.mass);
    }
    R1= invert_tortoise(Rminus, params.schw.mass) + 2.0*params.schw.mass;
    R2 = invert_tortoise(Rplus, params.schw.mass) + 2.0* params.schw.mass;
    w1 = params.schw.p_orb-(invert_tortoise(2.0*deltar, params.schw.mass)
                          +2.0*params.schw.mass)-R1;
    w2 = R2 - (params.schw.p_orb + invert_tortoise(2.0*deltar, params.schw.mass)+2.0*params.schw.mass);
    nmodes = lmmodes.ntotal;

    cout << "R1 R2 w1 w2" << endl;
    cout << R1 << " " << R2 << " " << w1 <<  " " << w2 << endl << endl;
  
    if(params.opts.useSource) {
      set_window(R1, w1, 1.0, 1.5, R2, w2, 1.0, 1.5, lmmodes.ntotal);
    }
  }
  
  double lowlim, uplim; 
  
  //setup the grid and the reference element
  if (params.metric.flatspacetime) {
    lowlim = params.grid.lowerlim;
    uplim = params.grid.upperlim;
    
   } else if (params.metric.schwarschild) {
    lowlim = Sminus;
    uplim = Splus;
  }
 
  Grid thegrid(params.grid.elemorder, params.grid.numelems, lmmodes.ntotal,
               lowlim, uplim);

  cout << "grid established " << endl;
  

  //find the indices associated with the radii to extract the solution at
  
  OutputIndices ijoutput;

  if(params.metric.schwarschild){
    //  int ifinite, iSplus, jfinite, jSplus;
    thegrid.find_extract_radii(rstar_of_r(params.grid.outputradius,
					  params.schw.mass), Splus, 
			       ijoutput);
    
    cout << "Oribital radius and output radius in Schwarzschild coords" << endl;
    cout << params.schw.p_orb<< " " << params.grid.outputradius << endl << endl;
    cout << "Output indices for finite and scri-plus radii" << endl;
    cout << ijoutput.ifinite << " " << ijoutput.jfinite << " " 
	 << ijoutput.iSplus << " " << ijoutput.jSplus << endl << endl;
  }
  

  cout << "entering diff eq" << endl;
  
  //setup the differential equation
  DiffEq theequation(thegrid, lmmodes, lmmodes.ntotal);

  cout << "diff eq established" << endl;
  
  //Declaration of calculation variables and 
  //Initialization to either zero or value read from file
  //Solution to PDE, possibly a vector
  TwoDVectorGridFunction<complex<double>> uh(lmmodes.ntotal,
					     params.grid.pdenum,
                                             params.grid.numelems,
                                             params.grid.elemorder+1,
					     {0.0,0.0});

  //Initialization of initial value of calculation variables.
  TwoDVectorGridFunction<complex<double>> uh0(lmmodes.ntotal,
                                              params.grid.pdenum,
                                              params.grid.numelems,
                                              params.grid.elemorder + 1,
                                              {0.0,0.0});
  //Right hand side of PDE
  TwoDVectorGridFunction<complex<double>> RHStdvgf(lmmodes.ntotal,
						   params.grid.pdenum,
						   params.grid.numelems,
						   params.grid.elemorder + 1,
						   {0.0,0.0}); 
  

  //Setup initial conditions and initialize window
  if(params.waveeq.issinusoid){
    initialSinusoid(uh, thegrid);
  } else if(params.waveeq.isgaussian) {
    initialGaussian(uh, thegrid);
  } else if(params.metric.schwarschild) {
    initialSchwarzchild(uh, thegrid, theequation);
    //need to write initial swcharzchild
  }
          
  //output window function
  ofstream fs;
  fs.open("window.txt");
  for(int i=0; i<params.grid.numelems; i++) {
    for(int j = 0; j<params.grid.elemorder+1; j++) {
      fs << thegrid.gridNodeLocations().get(i,j) << " " 
         << theequation.window.get(i,j) << " " << theequation.dwindow.get(i,j) << " "
         << theequation.d2window.get(i,j) << endl;
    }
  }
  cout << "window output" << endl;

  uh0 = uh;


  //Set time based on smallest grid spacing. This assumes all elements
  // are equal in size

  double dx = thegrid.gridNodeLocations().get(0, 1) - thegrid.gridNodeLocations().get(0, 0);
  
  double deltat;
  double maxspeed = 1.0;

  if(params.metric.schwarschild){
    deltat = params.time.courantfac * dx/maxspeed;
    cout << "set and actual time step, based on courant factor" << endl;
    cout << dx << " " << deltat << endl << endl;
  }else if(params.metric.flatspacetime){
    //int nt = ceil((params.time.tmax-params.time.t0) / params.time.courantfac / dx);
    //deltat = (params.time.tmax - params.time.t0) / nt;
    deltat = params.time.dt;
    cout << "deltat set to dt";
    cout << deltat << endl;
  }


  theequation.modeRHS(thegrid, uh, RHStdvgf, 0.0, false);


  if (params.metric.schwarschild){
    lmmodes.sum_m_modes(uh,0.0, ijoutput.ifinite, ijoutput.jfinite);
  }

  
  for(int k = 0; k < uh.TDVGFdim(); k++) {
    if(params.file.outputtimefixed) {


      write_fixed_time(k,params.time.t0,uh,RHStdvgf,thegrid,
		       theequation,lmmodes,true,
		       params.file.pdesolution,1);
      write_fixed_time(k,params.time.t0,uh,RHStdvgf,thegrid,
		       theequation,lmmodes,true,"rhs",3);
	     /*write_fixed_time(k,params.time.t0,uh,RHStdvgf,thegrid,
		       theequation,lmmodes,true,"source",2);
     write_fixed_time(k,params.time.t0,uh,RHStdvgf,thegrid,
		       theequation,lmmodes,true,"up",4);
	     */
    }
    
    if(params.file.outputradiusfixed){
      write_fixed_radius(ijoutput,k,params.time.t0,uh,RHStdvgf,thegrid,
			 theequation,lmmodes, true,
			 params.file.fixedradiusfilename,1);

      if(k==params.modes.lmax){
	write_summed_psi(ijoutput,k,params.time.t0,uh,RHStdvgf,thegrid,
			 theequation,lmmodes,true,
			 "psil",1);
	write_summed_psi(ijoutput,k,params.time.t0,uh,RHStdvgf,thegrid,
			 theequation,lmmodes,true,
			 "psitl",2);
	write_summed_psi(ijoutput,k,params.time.t0,uh,RHStdvgf,thegrid,
			 theequation,lmmodes,true,
			 "psiphil",3);
	write_summed_psi(ijoutput,k,params.time.t0,uh,RHStdvgf,thegrid,
			 theequation,lmmodes,true,
			 "psirl",4);
      }//end if k==lmax
    }//end if outputradiusfixed
  }//end for k
  
  //Initialize loop variables to determine when output
  //double output = deltat / 2.0;
  int outputcount =0;
  double t= params.time.t0;
  
  //  for(double t = params.time.t0; t < params.time.tmax + deltat; t += deltat) {
  while(t<params.time.tmax){
    //Increment the count to determine whether or not to output
    outputcount++;
    
    //Increment the time integration
    rk4lowStorage(thegrid, theequation, uh, RHStdvgf, t, deltat);
    //Initial conditions, numerical fluxes, boundary conditions handled inside 
    //Evolution.cpp, in RHS.

    //increment time
    t+=deltat;
    
    
    if (outputcount%params.time.outputevery == 0){
      //Output in gnuplot format
    
     for(int k = 0; k < uh.TDVGFdim(); k++) {
        if(params.file.outputtimefixed) {


	  write_fixed_time(k,t,uh,RHStdvgf,thegrid,
			   theequation,lmmodes,true,
			   params.file.pdesolution,1);
	  write_fixed_time(k,t,uh,RHStdvgf,thegrid,
			   theequation,lmmodes,true,"rhs",3);
	  /*	  write_fixed_time(ijoutput,k,t,uh,RHStdvgf,thegrid,
			   theequation,lmmodes,true,"source",2);
	  write_fixed_time(ijoutput,k,t,uh,RHStdvgf,thegrid,
			   theequation,lmmodes,true,"up",4);
	  */
	  
	 
	}
      
	if(params.file.outputradiusfixed){
	  write_fixed_radius(ijoutput,k,t,uh,RHStdvgf,thegrid,
			     theequation,lmmodes, true,
			   params.file.fixedradiusfilename,1);
	  if(k==params.modes.lmax){
	    lmmodes.sum_m_modes(uh, t, ijoutput.ifinite, ijoutput.jfinite);
	    write_summed_psi(ijoutput,k,t,uh,RHStdvgf,thegrid,
			     theequation,lmmodes,true,
			     "psil",1);
	    write_summed_psi(ijoutput,k,t,uh,RHStdvgf,thegrid,
			     theequation,lmmodes,true,
			     "psitl",2);
	    write_summed_psi(ijoutput,k,t,uh,RHStdvgf,thegrid,
			     theequation,lmmodes,true,
			     "psiphil",3);
	    write_summed_psi(ijoutput,k,t,uh,RHStdvgf,thegrid,
			     theequation,lmmodes,true,
			     "psirl",4);
	  }//end if k==lmax
	}//end if outputradiusfixed
      }//end for k
     ofstream fsL2;
     fsL2.open("L2error.txt", ios::app);
     fsL2.precision(15);
     if (outputcount==params.opts.L2outputcount){
       fsL2 << params.grid.elemorder << " " << params.grid.numelems << " " << deltat << " " << LTwoError(thegrid, uh0, uh) << " " << t << " " << outputcount << endl;
     }
     fsL2.close();
    }
  }//end while loop
}
                         

void initialSchwarzchild(TwoDVectorGridFunction<complex<double>>& uh, Grid& grd, DiffEq& eqn) {
  GridFunction<double> rho(uh.GFvecDim(), uh.GFarrDim(), false);
  rho=grd.gridNodeLocations();
  ofstream fs;
  fs.open("initialdata.txt");
  for(int i = 0; i < uh.GFvecDim(); i++) {
    for (int j = 0; j < uh.GFarrDim(); j++) {
      for (int n = 0; n < uh.TDVGFdim(); n++) {
        double modeval = exp(-0.5 * pow((rho.get(i,j) / params.schw.sigma), 2.0));
        uh.set(n,0,i,j,0.0);
        if(!params.opts.useSource){
          uh.set(n,2,i,j,modeval);
        }
        
        uh.set(n,1,i,j,0.0);
	fs << setprecision(16);
        fs << rho.get(i,j) << " " << uh.get(0,2,i,j) << endl;

      }

      if(params.opts.useSource){
        double * r = &grd.rschw.get(i)[0];
        double * win = &eqn.window.get(i)[0];
        double * dwin = &eqn.dwindow.get(i)[0];
        double * d2win = &eqn.d2window.get(i)[0];
        calc_window(params.grid.elemorder+1,r,win, dwin, d2win);
      } 
      double dxmin = fabs(grd.gridNodeLocations().get(0,0)
                          -grd.gridNodeLocations().get(0,2));
    }
  }
  fs.close();
}
  

void initialSinusoid(TwoDVectorGridFunction<complex<double>>& uh, Grid& grd){
  double omega = 2.0 * PI / params.sine.wavelength;
  
  for(int k = 0; k < uh.TDVGFdim(); k++) {
    for(int i = 0; i < uh.GFvecDim(); i++){
      for (int j = 0; j < uh.GFarrDim(); j++){
        double psi = params.sine.amp * sin(omega * grd.gridNodeLocations().get(i, j)
                                           + params.sine.phase);
        double rhovar = omega * params.sine.amp * cos(omega * grd.gridNodeLocations().get(i, j)
                                                     + params.sine.phase);
        double pivar = -params.waveeq.speed * rhovar;
        //travelling wave
        //are rho and pi backward?
        uh.set(k, 0, i, j, psi);
        uh.set(k, 1, i, j, pivar);
        uh.set(k, 2, i, j, rhovar);
      }
    }
  }
}
void initialGaussian(TwoDVectorGridFunction<complex<double>>& uh, Grid& grd){
  for(int k = 0; k < uh.TDVGFdim(); k++) {
    for(int i = 0; i < uh.GFvecDim(); i++){
      for(int j = 0; j < uh.GFarrDim(); j++){
        double gaussian = params.gauss.amp * exp(-pow((grd.gridNodeLocations().get(i, j)
                                                       - params.gauss.mu), 2.0)
                                                 / 2.0 
                                                 / pow(params.gauss.sigma, 2.0));
        double dgauss = -(grd.gridNodeLocations().get(i, j) - params.gauss.mu)
          / pow(params.gauss.sigma, 2.0) * gaussian;
        uh.set(k, 0, i, j, gaussian);
        uh.set(k, 1, i, j, 0.0); //time derivative is zero
        //starts at center and splits
        uh.set(k, 2, i, j, dgauss);
      }
    }
  }
}

  double LTwoError(Grid thegrid, TwoDVectorGridFunction<complex<double>>& uh0, 
                   TwoDVectorGridFunction<complex<double>>& uhend)
{
  //FIX THIS SO IT DEALS WITH SUM OF MODES and nonrepeating waveforms
  //The square root of the integral of the squared difference
  double L2;
  L2 = 0.0;
  Array1D<double> weights;
  weights = thegrid.refelem.getw();
  for(int i = 0; i < uh0.GFvecDim(); i++){
    for(int j = 0; j < uh0.GFarrDim(); j++){
      double added = weights[j] 
        * pow(abs(uh0.get(0, 0, i, j)
                  - uhend.get(0, 0, i, j)), 2.0)
        / thegrid.jacobian(i);
      L2 += added;
    }
  }
  L2 = sqrt(L2); 
  return L2;
}

