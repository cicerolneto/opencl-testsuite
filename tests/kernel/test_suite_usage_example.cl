// TODO: provide echo script or change runlines to be platform independent...
//       maybe sh is actually required in windows too...

// RUN: echo "Hello worlds this documents some subtitutions which are available in test scripts"
// RUN: echo "Substitute %\%: %%"
// RUN: echo "Substitute %\s: %s"
// RUN: echo "Substitute %\S: %S"
// RUN: echo "Substitute %\p: %p"
// RUN: echo "Substitute %\{pathsep}: %{pathsep}"
// RUN: echo "Substitute %\t: %t"
// RUN: echo "Substitute %\T: %T"
// RUN: echo "Substitute %\{src_root}: %{src_root}"
// RUN: echo "Substitute %\{inputs}: %{inputs}"
// RUN: echo "Substitute %\{lit}: %{lit}"
// RUN: echo "Substitute %\{python}: %{python}"
// RUN: echo "Substitute %\{device}: %{device}"
// RUN: echo "Substitute %\{device_id}: %{device_id}"
// RUN: echo "Substitute %\{ocl_tester}: %{ocl_tester}"
// RUN: true && echo "Set this to false to see values for arguments"
