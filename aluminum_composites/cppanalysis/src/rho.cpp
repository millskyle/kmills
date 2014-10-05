#ifndef MSD_H
#define MSD_H

#include "data_structure.h"

int global_density(FileInfo *vasprun, Configuration *config) {
   if (!config->msd) {cout << "\nMSD called but not requested in configuration. Exiting"; return 1;}

   cout << "--- Starting Density calculations ---" <<endl;
   cout << "Density plots requested for " << config->rho_atoms.size() << " atom types: " << vec2str(config->rho_atoms) << endl;
   //We need to use unwrapped coordinates.  Unwrap if not already unwrapped.
   vasprun->unwrap(); 
   
   //Start a simple bash script which calls GNUplot to plot the msd data
   ofstream of2;
   of2.open("output/plot_msd.sh");
   of2 << "#!/bin/bash" << "\n" 
       << "gnuplot -persist << GNUPLOTINPUT" << "\n"
       << "set title \"Mean Square Displacement\"\n"
       << "set term pdf\n"
       << "set output \"msd.pdf\"\n"
       << "set xlabel \"Time, picoseconds\"\n"
       << "set ylabel \"distance, Angstroms\"\n"
//       << "unset key" << "\n"
       << "plot ";


   //For each atom in the requested atom types
   for (int atomname=0; atomname < config->msd_atoms.size(); atomname++) {
      //this pointer will point to the atomType object for this type of atom 
      atomType* atomobject = vasprun->GetAtom(config->msd_atoms[atomname]);
      //Calculate the center of mass for this atom_type. It'll be stored in the object.
      vasprun->calculate_COM(atomobject);
      //Make a pointer to the center of mass vector so I don't have to type so much all the time.
//    vector<threevector>& COM = vasprun->atoms[atomobject->atomindex].COM.COM_value;         
      cout << "Beginning MSD calculation for " << atomobject->element << ".\n";   
      int msd_count=0; //the integer number of data points that go into the aggregate sum
      double msd_sum=0; //the aggregate sum of of the displacements in the  timestep
      double xdist; // distances that the atom moved in x,y,z
      double ydist;
      double zdist;
      //for each timestep which we have positions for
      for (int t=1; t < atomobject->timesteps.size()-2; t++ ) {
         //for each atom in the position vector of vectors
         for (int a=0; a<atomobject->atomspertype-1; a++) {
               int t2=0; //the timestep to calculate displacement from
               // xdist = r(t) - r(0) - [  COM(t)  - COM(t) ]
               xdist = atomobject->timesteps[t].ppp_uw[a][0] - atomobject->timesteps[t2].ppp_uw[a][0]
                       - atomobject->timesteps[t].COM[0] + atomobject->timesteps[t2].COM[0];
               ydist = atomobject->timesteps[t].ppp_uw[a][1] - atomobject->timesteps[t2].ppp_uw[a][1]
                       - atomobject->timesteps[t].COM[1] + atomobject->timesteps[t2].COM[1];
               zdist = atomobject->timesteps[t].ppp_uw[a][2] - atomobject->timesteps[t2].ppp_uw[a][2]
                       - atomobject->timesteps[t].COM[2] + atomobject->timesteps[t2].COM[2];
               //the sum is the modulus of this vector w/o square root.
               msd_sum+=pow(xdist,2) + pow(ydist,2) + pow(zdist,2);
               //increment the sample counter (we'll use this to average later).
               msd_count++;
         }
         //push the data in to the object
         atomobject->timesteps[t].MSD= ( msd_sum / msd_count );
         //reset the counters
         msd_sum=0;
         msd_count=0;
      }

   //write out the data for this element to an element-specific file
   ofstream of;
   of.open("output/" + config->msd_data_prefix + atomobject->element + ".data");     
       
   //write out the gnuplot command, scaling the x-axis increment by the timestep to get it in picoseconds
   of2 << "'" << config->msd_data_prefix + atomobject->element << ".data' using (\\$0*" << vasprun->dt << "*0.001):1 with lines title '" << atomobject->element << "' , ";

   //write each timestep to a file
   for (int t=0; t < atomobject->timesteps.size(); t++) {
      if (atomobject->timesteps[t].MSD>=0) {
         of << atomobject->timesteps[t].MSD << "\n" ;
//         cout << atomobject->timesteps[t].MSD << "\n" ;
      }
   }
   of.close();

   
}

   //Close off the GNUPlot bash script
   of2 << "\nGNUPLOTINPUT\n";
   of2.close();

   //add a command to the global plot script to make the msd plots
   config->script_wrapper << "\nbash plot_msd.sh \n" ;   

   return 0;

}

#endif
