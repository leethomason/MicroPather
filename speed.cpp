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
#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include <vector>
#include <chrono>

#include "micropather.h"
using namespace micropather;

#ifdef _MSC_VER
#include <Windows.h>
// The std::chronos high resolution clocks are no where near accurate enough on Windows 10.
// Many calls come back at 0 time. 
typedef uint64_t TimePoint;

inline uint64_t FastTime()
{
	uint64_t t;
	QueryPerformanceCounter((LARGE_INTEGER*)&t);
	return t;
}

inline int64_t Nanoseconds(TimePoint start, TimePoint end)
{
	uint64_t freq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	return (end - start) * 1000 * 1000 * 1000 / freq;
}
#else
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;

inline TimePoint FastTime()
{
	return std::chrono::high_resolution_clock::now();
}

inline int64_t Nanoseconds(TimePoint start, TimePoint end)
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}
#endif


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
		int index = (int)((intptr_t)node);
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

	const int NUM_TEST = 389;
	
	int		indexArray[ NUM_TEST ];	// a bunch of locations to go from-to
	float	costArray[ NUM_TEST ];
	int64_t timeArray[ NUM_TEST ];
	int		resultArray[ NUM_TEST ];

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
					
	// Set up the test locations, making sure they
	// are all valid.
	for (int i = 0; i < NUM_TEST; ++i) {
		indexArray[i] = (MAPX*MAPY) * i / NUM_TEST;
		costArray[i] = 0.0f;

		int y = indexArray[i] / MAPX;
		int x = indexArray[i] - MAPX*y;
		while (!dungeon.Passable(x, y)) {
			indexArray[i] += 1;
			y = indexArray[i] / MAPX;
			x = indexArray[i] - MAPX*y;
		}
	}
	// Randomize the locations.
	for (int i = 0; i < NUM_TEST; ++i)
	{
		int swapWith = rand() % NUM_TEST;
		int temp = indexArray[i];
		indexArray[i] = indexArray[swapWith];
		indexArray[swapWith] = temp;
	}

	int64_t compositeScore = 0;
	for ( int numDir=4; numDir<=8; numDir+=4 )
	{
		dungeon.maxDir = numDir;
		dungeon.aStar->Reset();

		static const int SHORT_PATH = 0;
		static const int MED_PATH	= 1;
		static const int LONG_PATH  = 2;
		static const int FAIL_SHORT = 3;
		static const int FAIL_LONG  = 4;

		for( int reset=0; reset<=1; ++reset )
		{
			TimePoint clockStart = FastTime();
			for( int i=0; i<NUM_TEST; ++i ) 
			{
				if ( reset )
					dungeon.aStar->Reset();
				
				int startState = indexArray[i];
				int endState = indexArray[ (i==(NUM_TEST-1)) ? 0 : i+1];

				TimePoint start = FastTime();
				resultArray[i] = dungeon.aStar->Solve( (void*)startState, (void*)endState, &dungeon.path, &costArray[i] );
				TimePoint end = FastTime();

				timeArray[i] = Nanoseconds(start, end);
				MPASSERT(timeArray[i]);
			}
			TimePoint clockEnd = FastTime();

			#ifndef PROFILING_RUN
			// -------- Results ------------ //
			const float shortPath = (float)(MAPX / 4);
			const float medPath = (float)(MAPX / 2 );

			int count[5] = { 0 };	// short, med, long, fail short, fail long
			int64_t time[5] = { 0 };

			for(int i=0; i<NUM_TEST; ++i )
			{
				int idx = 0;
				if ( resultArray[i] == MicroPather::SOLVED ) {
					if ( costArray[i] < shortPath ) {
						idx = SHORT_PATH;
					}
					else if ( costArray[i] < medPath ) {
						idx = MED_PATH;
					}
					else {
						idx = LONG_PATH;
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
						idx = FAIL_SHORT;
					}
					else {
						idx = FAIL_LONG;
					}
				}
				count[idx] += 1;
				time[idx] += timeArray[i];
			}

			printf( "Average of %d runs. Reset=%s. Dir=%d.\n",
					NUM_TEST, reset ? "true" : "false", numDir );
			printf( "short(%4d)       = %7.2f\n", count[0],	double(time[0]) / count[0] * 0.001 );
			printf( "med  (%4d)       = %7.2f\n", count[1],	double(time[1]) / count[1] * 0.001 );
			printf( "long (%4d)       = %7.2f\n", count[2],	double(time[2]) / count[2] * 0.001 );
			printf( "fail short (%4d) = %7.2f\n", count[3],	double(time[3]) / count[3] * 0.001 );
			printf( "fail long  (%4d) = %7.2f\n", count[4],	double(time[4]) / count[4] * 0.001 );

			int64_t totalTime = 0;
			int totalCount = 0;
			for( int k=0; k<5; ++k ) {
				totalTime += time[k];
				totalCount += count[k];
			}	
			printf( "Average           = %7.2f\n", double(totalTime) / totalCount * 0.001 );
			compositeScore += totalTime / totalCount;
			#endif
		}
	}
	printf( "Composite average = %7.2f\n", double(compositeScore) / 4 * 0.001);

	return 0;
}

