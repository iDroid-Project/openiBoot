This folder will contain doxygen generated documentation.

To generate the documentation, make sure you have the appropriate
doxygen package installed for your operating system (and that it's
on the system path), then run `scons docs`.

This will generated html and latex directories alongside this
document.



The documentation is only generated from specific comments in the
source code. To add a comment that will be read by doxygen, 
start it with /**, and put it before the item of interest.

For example, in a header:

/**
 * This function does cool stuff.
 *
 * A longer explanation of the cool stuff the function does.
 *
 * @param _x some top-secret X value!
 */
void my_test_function(int _x);
