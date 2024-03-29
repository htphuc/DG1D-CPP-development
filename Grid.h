#ifndef GRID_H
#define GRID_H

#include "TNT2.h"
#include "ReferenceElement.h"
#include "GridFunction.h"
#include <fstream>
#include "VectorGridFunction.h"
#include "TwoDVectorGridFunction.h"
#include <complex>

//Indices of the grid (i) and of the nodes (j) where data will be output
// at a finite radius and at Splus. Based on params.grid.outputradius
struct OutputIndices
{int ifinite, jfinite, iSplus, jSplus;};

class Grid
{
 private:
  int NumElem; //N. Number of elements in grid.
  int order;  //Np. Element order
  vector<double> elementBoundaries; //length N+1
  vector<double> drdx; //jacobian
  GridFunction<double> nodeLocs; //N by Np. Physical node locations.
  void calcjacobian();

 public:
  GridFunction<double> rschw;//Schwarzschild coordinate
  GridFunction<double> rstar; //tortoise coordinate

  Grid(int elemorder, int numelements, int nummodes, double lowerlim, 
       double upperlim);
  ReferenceElement refelem; //member variable: the reference element

  //find the radii at which to output the data (returns grid and node indices)
  void find_extract_radii(double rfinite, double rSplus, OutputIndices& ijoutput);
  int numberElements();//Returns number of elements, calculated from input file
  GridFunction<double> gridNodeLocations();  //Returns physical node location
  vector<double> gridBoundaries(); //Returns the boundaries of the elements
  double jacobian(int elemnum); //Returns the jacobian of a specific element
  int nodeOrder(); //Returns the node order (same for all elements)
};

#endif
