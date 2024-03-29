#ifndef NAMESPACES_H
#define NAMESPACES_H

namespace orbit
{
  extern double p; //semilatus rectum
  extern double e; //eccentricity
  extern double chi; //parameter that runs from 0 to 2pi in on radial cycle
  extern double phi; //standard angle phi
}

namespace layers
{
  //all in computational coordinate,
  //mix of hyperboloidal and tortoise
  extern double Splus; //scri-plus
  extern double Sminus; //horizon
  extern double Rplus; //transition from tortoise to hyperboloidal region
  extern double Rminus; //transition from hyperboloidal to tortoise region
  extern double Wplus; //outer edge of window function
  extern double Wminus; //inner edge of window function
}

namespace window
{//Schwarzschild coordinates
  extern double R1; //Rminus
  extern double R2; //Rplus
  extern double w1; //Wminus
  extern double w2; //Wplus
  extern double s1;//parameters to transition function (Hyperboloidal to tortoise)
  extern double s2;//parameters to transition function (Hyperboloidal to tortoise)
  extern double q1;//parameters to transition function (Hyperboloidal to tortoise)
  extern double q2;//parameters to transition function (Hyperboloidal to tortoise)
  extern double nmodes; //total number of modes in computation
}

#endif
