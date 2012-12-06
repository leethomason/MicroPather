# Python program to set the version.
##############################################


def fileProcess( name, lineFunction ):
	filestream = open( name, 'r' )
	if filestream.closed:
		print( "file " + name + " not open." )
		return

	output = ""
	print( "--- Processing " + name + " ---------" )
	while 1:
		line = filestream.readline()
		if not line: break
		output += lineFunction( line )
	filestream.close()
	
	if not output: return			# basic error checking
	
	print( "Writing file " + name )
	filestream = open( name, "w" );
	filestream.write( output );
	filestream.close()
	
	
def echoInput( line ):
	return line
	

import sys
major = input( "Major: " )
minor = input( "Minor: " )
build = input( "Build: " )

print "Version: " + `major` + "." + `minor` + "." + `build`

#### Write the buildlilith #####

def buildlinuxRule( line ):

	i = line.rfind( "_" )

	if i >= 4 and line[i] == "_" and line[i-2] == "_" and line[i-4] == "_":
		# This is ghetto. Should really use regular expressions.
		i -= 4
		print "buildmicro instance found"
		return line[0:i] + "_" + `major` + "_" + `minor` + "_" + `build` + line[i+6:]
	else:
		return line

fileProcess( "buildmicro", buildlinuxRule )


