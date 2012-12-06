/*
Copyright (c) 2000-2012 Lee Thomason (www.grinninglizard.com)

This software is provided 'as-is', without any express or implied 
warranty. In no event will the authors be held liable for any 
damages arising from the use of this software.

Permission is granted to anyone to use this software for any 
purpose, including commercial applications, and to alter it and 
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must 
not claim that you wrote the original software. If you use this 
software in a product, an acknowledgment in the product documentation 
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and 
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source 
distribution.
*/

#include <ctype.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include <vector>

#include "micropather.h"
using namespace micropather;

//#define PROFILING_RUN

#ifdef _MSC_VER
typedef unsigned __int64 U64;
#else
typedef unsigned long long U64;
#endif
typedef unsigned int U32;

#ifdef _MSC_VER
inline U64 FastTime()
{
	union 
	{
		U64 result;
		struct
		{
			U32 lo;
			U32 hi;
		} split;
	} u;
	u.result = 0;

	_asm {
		//pushad;	// don't need - aren't using "emit"
		cpuid;		// force all previous instructions to complete - else out of order execution can confuse things
		rdtsc;
		mov u.split.hi, edx;
		mov u.split.lo, eax;
		//popad;
	}				
	return u.result;
}
#else
#define rdtscll(val)  __asm__ __volatile__ ("rdtsc" : "=A" (val))
inline U64 FastTime() {
    U64 val;
 	rdtscll( val );
 	return val;     
}    
#endif

inline int TimeDiv( U64 time, int count )
{
	if ( count == 0 ) count = 1;
	U64 timePer = time / (U64)count;
	MPASSERT( timePer < INT_MAX );
	return (int) timePer / 1000;
}

const int MAPX = 90;
const int MAPY = 20;
const char gMap[MAPX*MAPY+1] =
  //"012345678901234567890123456789"
	"     |      |                |     |      ||               |     |      |                |"
	"     |      |----+    |      +     |      ||---+    |      +     |      |----+    |      +"
	"---+ +---  -+      +--+--+    ---+ +---  -+|     +--+--+    ---+ +---  -+      +--+--+    "
	"   |                     +-- +   |        ||           +-- +   |                     +-- +"
	"        +----+  +---+                     ||  +---+                 +----+  +---+         "
	"---+ +  +    +            |   ---+ +                    |  2---+ +  +    +            |   "
	"   | |  +----+    +----+  +--+   | |      ||    +----+  +--+322| |  +----+    +----+  +--+"
	"     |            |    |           |      ||    |    |   222232  |            |    |      "
	"   | +-------+  +-+    |------------------+|  +-+    |--+  2223| +-------+  +-+    |--+   "
	"---+                   |                  ||         |  222+---+                   |     +"
	"     |      |          |                  ||          22233|     |      |                |"
	"     |      |----+    ++                  ||---+3333|    22+     |      |----+    |      +"
	"---+ +---  -+      +--+-------------------||22223+--+--+    ---+ +---  -+      +--+--+    "
	"   |                     +-- +   |          22223333   +-- +   |                     +-- +"
	"        +----+  +---+                 +---+|  +---+     222         +----+  +---+         "
	"---+ +  +    +            |   ---+ +  +   +|            |222---+ +  +    +            |   "
	"   | |  +----+    +----+  +--+   | |  +---+|  22+----+  +--+233|2|  +----+    +----+  +--+"
	"     |            |    |           |      ||2222|    |    2223333|            |    |      "
	"   | +-------+  +-+    |--+      | +------++  +-+    |--+      |2+-------+  +-+    |--+   "
	"---+                   |     +---+        ||         |     +---+                   |      ";

class Dungeon : public Graph
{
  public:
	MPVector<void*> path;
	MicroPather* aStar;
	int maxDir;

	Dungeon() {
		aStar = new MicroPather( this, MAPX*MAPY, 6 );
		maxDir = 4;
	}

	virtual ~Dungeon() {
		delete aStar;
	}

	int Passable( int nx, int ny ) 
	{
		if (    nx >= 0 && nx < MAPX 
			 && ny >= 0 && ny < MAPY )
		{
			int index = ny*MAPX+nx;
			char c = gMap[ index ];
			if ( c == ' ' )
				return 1;
			else if ( c >= '1' && c <= '9' ) {
				int val = c-'0';
				MPASSERT( val > 0 );
				return val;
			}
		}		
		return 0;
	}

	void NodeToXY( void* node, int* x, int* y ) 
	{
		int index = (int)node;
		*y = index / MAPX;
		*x = index - *y * MAPX;
	}

	void* XYToNode( int x, int y )
	{
		return (void*) ( y*MAPX + x );
	}
		
	
	virtual float LeastCostEstimate( void* nodeStart, void* nodeEnd ) 
	{
		int xStart, yStart, xEnd, yEnd;
		NodeToXY( nodeStart, &xStart, &yStart );
		NodeToXY( nodeEnd, &xEnd, &yEnd );

		int dx = xStart - xEnd;
		int dy = yStart - yEnd;
		return (float) sqrt( (double)(dx*dx) + (double)(dy*dy) );
	}

	virtual void  AdjacentCost( void* node, MPVector< StateCost > *neighbors ) 
	{
		int x, y;
		//					E  N  W   S     NE  NW  SW SE
		const int dx[8] = { 1, 0, -1, 0,	1, -1, -1, 1 };
		const int dy[8] = { 0, -1, 0, 1,	-1, -1, 1, 1 };
		const float cost[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 
								1.41f, 1.41f, 1.41f, 1.41f };

		NodeToXY( node, &x, &y );

		for( int i=0; i<maxDir; ++i ) {
			int nx = x + dx[i];
			int ny = y + dy[i];

			int pass = Passable( nx, ny );
			if ( pass > 0 ) {
				// Normal floor
				StateCost nodeCost = { XYToNode( nx, ny ), cost[i] * (float)(pass) };
				neighbors->push_back( nodeCost );
			}
		}
	}

	virtual void  PrintStateInfo( void* node ) 
	{
		int x, y;
		NodeToXY( node, &x, &y );
		printf( "(%2d,%2d)", x, y );
	}

};


int main( int argc, const char* argv[] )
{
	Dungeon dungeon;

	#ifdef PROFILING_RUN
	const int NUM_TEST = 117;
	#else
	const int NUM_TEST = 389;
	#endif
	
	int   indexArray[ NUM_TEST ];
	float costArray[ NUM_TEST ];
	U64   timeArray[ NUM_TEST ];
	int   resultArray[ NUM_TEST ];

	int i;

	bool useBinaryHash = false;
	bool useList = false;
	bool debug = false;

	#ifdef DEBUG
	debug = true;
	#endif
	#ifdef USE_BINARY_HASH
	useBinaryHash = true;
	#endif
	#ifdef USE_LIST
	useList = true;
	#endif 
	
	printf( "SpeedTest binaryHash=%s list=%s debug=%s\n",
			useBinaryHash ? "true" : "false",
			useList ? "true" : "false",
			debug ? "true" : "false" );
					
	// Set up the test indices. Distribute, then randomize.
	for( i=0; i<NUM_TEST; ++i ) {
		indexArray[i] = (MAPX*MAPY) * i / NUM_TEST;
		costArray[i] = 0.0f;
		
		int y = indexArray[i] / MAPX;
		int x = indexArray[i] - MAPX*y;
		while ( !dungeon.Passable( x, y ) ) {
			indexArray[i] += 1;
			y = indexArray[i] / MAPX;
			x = indexArray[i] - MAPX*y;
		}
	}
	for( i=0; i<NUM_TEST; ++i )
	{
		int swapWith = rand()%NUM_TEST;
		int temp = indexArray[i];
		indexArray[i] = indexArray[swapWith];
		indexArray[swapWith] = temp;
	}

	int compositeScore = 0;
	for ( int numDir=4; numDir<=8; numDir+=4 )
	{
		dungeon.maxDir = numDir;
		dungeon.aStar->Reset();

		for( int reset=0; reset<=1; ++reset )
		{
			clock_t clockStart = clock();
			for( i=0; i<NUM_TEST; ++i ) 
			{
				if ( reset )
					dungeon.aStar->Reset();
				
				int startState = indexArray[i];
				int endState = indexArray[ (i==(NUM_TEST-1)) ? 0 : i+1];

				U64 start = FastTime();
				resultArray[i] = dungeon.aStar->Solve( (void*)startState, (void*)endState, &dungeon.path, &costArray[i] );
				U64 end = FastTime();

				timeArray[i] = end-start;
			}
			clock_t clockEnd = clock();

			#ifndef PROFILING_RUN
			// -------- Results ------------ //
			const float shortPath = (float)(MAPX / 4);
			const float medPath = (float)(MAPX / 2 );

			int count[5] = { 0, 0, 0, 0 };	// short, med, long, fail short, fail long
			U64 time[5] = { 0, 0, 0, 0 };

			for( i=0; i<NUM_TEST; ++i )
			{
				if ( resultArray[i] == MicroPather::SOLVED ) {
					if ( costArray[i] < shortPath ) {
						++count[0];				
						time[0] += timeArray[i];
					}
					else if ( costArray[i] < medPath ) {
						++count[1];
						time[1] += timeArray[i];
					}
					else {
						++count[2];
						time[2] += timeArray[i];
					}
				}
				else if ( resultArray[i] == MicroPather::NO_SOLUTION ) {

					int startState = indexArray[i];
					int endState = indexArray[ (i==(NUM_TEST-1)) ? 0 : i+1];
					int startX, startY, endX, endY;
					dungeon.NodeToXY( (void*)startState, &startX, &startY );
					dungeon.NodeToXY( (void*)endState, &endX, &endY );

					int distance = abs( startX - endX ) + abs( startY - endY );

					if ( distance < shortPath ) {
						++count[3];
						time[3] += timeArray[i];
					}
					else {
						++count[4];
						time[4] += timeArray[i];
					}
				}
			}

			printf( "Average of %d runs. Reset=%s. Dir=%d.\n",
					NUM_TEST, reset ? "true" : "false", numDir );
			printf( "short(%4d, cutoff=%.1f)= %d\n", count[0], shortPath, TimeDiv( time[0], count[0] ));
			printf( "med  (%4d, cutoff=%.1f)= %d\n", count[1], medPath, TimeDiv( time[1], count[1] ));
			printf( "long (%4d)             = %d\n", count[2], TimeDiv( time[2], count[2] ));
			printf( "fail (%4d) short       = %d\n", count[3], TimeDiv( time[3], count[3] ));
			printf( "fail (%4d) long        = %d\n", count[4], TimeDiv( time[4], count[4] ));

			int total = 0;
			for( int k=0; k<5; ++k ) {
				total += TimeDiv( time[k], count[k] );
			}	
			printf( "Average                = %d\n", total / 5 );
			compositeScore += total;

			//printf( "Average time / test     = %.3f ms\n\n", 1000.0f * float(clockEnd-clockStart)/float( NUM_TEST*CLOCKS_PER_SEC ) );
			#endif
            //dungeon.aStar->DumpHashTable();			
		}
	}
	printf( "Composite average = %d\n", compositeScore / (5*4) );

	return 0;
}

