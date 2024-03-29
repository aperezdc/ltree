ltree(1)

# NAME

ltree - tool for manipulating *mtree*(5) directory hierarchy files

# SYNOPSIS

*ltree* [*-l*] [*-0*] < _mtree_

*ltree* *-C* [*-v*] [*-0*] [*-p* _path_] < _mtree_

*ltree* *-r* [*-v*] [*-0*] [*-p* _path_] < _mtree_

# DESCRIPTION

The *ltree* utility reads a specification from the standard input in the
same format as generated by the *mtree*(8) utility and perform a number
of actions based on it. Some of the actions operate on the file hierarchy
rooted in the current directory. Messages are written to the standard
output in certain conditions depending on the selected action.

Supported actions can be chosen with the following options:

*-l*
	Lists contents of the input _mtree_ specification. This is the default
	action and can be omitted.

*-C*
	Checks whether the file hierarchy matches the _mtree_ specification,
	reporting missing files and mismatched file and directory attributes.

*-R*
	Removes the files listed in the _mtree_ specification.

The following options modify the behavior of the actions (note that some
options do not apply to every action):

*-v*
	Verbose operation. The file name of each entry from the specification
	is printed to the standard output after it has been processed. This is
	always enabled automatically by the *-l* option.

*-0*
	Print entries in standard output followed by a null, instead of the
	newline character. This allows file names that contain newlines or
	other types of white space to be correctly interpreted by programs
	that process the output.

*-p* _path_
	Use the file hierarchy rooted in _path_, instead of the current
	directory.

The *ltree* utility exits with a status of 0 on success, 1 if any error
occured, and 2 if the file hierarchy did not match the specification.

# EXIT STATUS

The *ltree* utility exits with 0 on success, and >0 if an error occurs.

# SEE ALSO

mtree(8), mtree(5)

# HISTORY

The *mtree* utility appeared in 4.3BSD-Reno, and it was later on ported
to Linux by Archie L. Cobbs <archie@dellroad.org>.

The *ltree* utility was written in early 2018 to complement *mtree* by Adrian
Perez de Castro <aperez@igalia.com>.

The *libarchive*(3) library includes support for reading and writing the
*mtree*(5) format, and is used by *ltree*.
