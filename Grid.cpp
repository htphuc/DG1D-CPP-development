#include "Grid.h"

//du/dt + A du/dx + Bu = 0

Grid::Grid(int elemorder, int numelements, double lowerlim, 
           double upperlim):
  order{elemorder},
  NumElem{numelements},
  nodeLocs{0,elemorder + 1}, 
  refelem{elemorder},
  rschw{numelements,elemorder+1},
  rstar{numelements,elemorder+1}

{

  //assign evenly spaced element boundaries
  for(int i = 0; i <= numelements; i++) {
    elementBoundaries.push_back(lowerlim + i * (upperlim - lowerlim) 
                                / float(numelements));
  }
  
  //Get physical positions of nodes from the reference element
  Array1D<double> physicalPosition(elemorder + 1);
  for(int elem = 0; elem < numelements; elem++){
    physicalPosition = ((elementBoundaries[elem + 1] 
                         - elementBoundaries[elem]) / 2.0)
      *refelem.getr()
      +((elementBoundaries[elem + 1] + elementBoundaries[elem]) / 2.0);
    nodeLocs.append(physicalPosition);
  }
  
  //Calculate the jacobian associated with the transformation each element
  //from the reference element to physical space
  calcjacobian();
  
}


GridFunction<double> Grid::gridNodeLocations()
{
  return nodeLocs;
}

vector<double> Grid::gridBoundaries()
{
  return elementBoundaries;
}

void Grid::calcjacobian()
{
  for(int elem = 0; elem < NumElem; elem++){
    double rx = 2.0 / (elementBoundaries[elem + 1] 
                       - elementBoundaries[elem]);
    drdx.push_back(rx);
  }
}

double Grid::jacobian(int elemnum)
{
  return drdx[elemnum];
}

int Grid::nodeOrder()
{
  return order;
}

int Grid::numberElements()
{
  return NumElem;
}

void Grid::find_extract_radii(double rfinite, double rSplus, int& ifinite, 
                              int& iSplus, int& jfinite, int& jSplus){
  for(int elem=0; elem<NumElem; elem++){
    for(int node =0; node<=order; node++){
      cout << elem << " " << node << " " << nodeLocs.get(elem,node)-rfinite << endl;
      if(fabs(nodeLocs.get(elem,node)-rfinite)<1.0e-5)
        { 
          ifinite = elem;
          jfinite = node;
        } else if (fabs(nodeLocs.get(elem, node) - rSplus) < 1.0e-5)
        {
          iSplus = elem;
          jSplus = node;
        }
    }
  }
}
