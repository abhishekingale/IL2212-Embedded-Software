IL2212 - Embedded Software Lab Project
======================================

This is the project structure used throughout the IL2212 - Embedded Software Lab course held at KTH in VT19. Please consult the [Canvas course page](https://kth.instructure.com/courses/7693/pages/laboratory-information-about-the-project) for further information and documentation.

## Contributors

* _Student 1_ (email1@kth.se)
* _Student 2_ (email2@kth.se)
* _Group ##_

## Contents 

This repository is organized as follows:

 * `app` contains source files for the lab applications, grouped in subfolders by project. It also contains build automation scripts for helping with the lab tasks.
 * `bsp` is an empty folder which will contain the generated Board Support Packages for different configurations.
 * `hardware` contains files describing the custom hardware platform. 
 * `model` contains the (ready-to-build) ForSyDe-Haskell project which represents the executable functional specification of the application that needs to be implemented on the given hardware platform.
 * `test` contains input images and scripts that help in generating input data, output plots and testing the implementations against the golden model.
 
For more detailed usage instructions, check the `README` files found in each folder.
