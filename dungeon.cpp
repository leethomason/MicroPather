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


#define USE_PATHER

#include <ctype.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>

#include <vector>
#include <iostream>

#ifdef USE_PATHER

#include "micropather.h"
using namespace micropather;
#endif


const int MAPX = 30;
const int MAPY = 10;
const char gMap[MAPX*MAPY+1] =
  //"012345678901234567890123456789"
	"     |      |                |"
	"     |      |----+    |      +"
	"---+ +---DD-+      +--+--+    "
	"   |                     +-- +"
	"        +----+  +---+         "
	"---+ +  D    D            |   "
	"   | |  +----+    +----+  +--+"
	"   D |            |    |      "
	"   | +-------+  +-+    |--+   "
	"---+                   |     +";

class Dungeon
#ifdef USE_PATHER
  : public Graph
#endif
{
  private:
	Dungeon( const Dungeon& );
	void operator=( const Dungeon& );
  
	int playerX, playerY;
	MPVector<void*> path;
	bool doorsOpen;
	bool showConsidered;

	MicroPather* pather;

  public:
	Dungeon() : playerX( 0 ), playerY( 0 ), doorsOpen( false ), showConsidered( false ), pather( 0 )
	{
		pather = new MicroPather( this, 20 );	// Use a very small memory block to stress the pather
	}

	virtual ~Dungeon() {
		delete pather;
	}

	int X()	{ return playerX; }
	int Y() { return playerY; }

	void ClearPath()
	{
		#ifdef USE_PATHER
		path.resize( 0 );
		#endif
	}

	void ToggleTouched() { 	showConsidered = !showConsidered; 
 							pather->Reset();
						  }

	void ToggleDoor() 
	{ 
		doorsOpen = !doorsOpen; 
	
		#ifdef USE_PATHER
		pather->Reset();

		#endif	
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
			else if ( c == 'D' )
				return 2;
		}		
		return 0;
	}

	int SetPos( int nx, int ny ) 
	{
		int result = 0;
		if ( Passable( nx, ny ) == 1 )
		{
			#ifdef USE_PATHER
				float totalCost;
				if ( showConsidered )
					pather->Reset();
					
				result = pather->Solve( XYToNode( playerX, playerY ), XYToNode( nx, ny ), &path, &totalCost );

				if ( result == MicroPather::SOLVED ) {
					playerX = nx;
					playerY = ny;
				}
				printf( "Pather returned %d\n", result );

			#else
				playerX = nx;
				playerY = ny;
			#endif
		}
		return result;
	}

	void Print() 
	{
		char buf[ MAPX+1 ];

		MPVector< void* > stateVec;
		
		if ( showConsidered )
			pather->StatesInPool( &stateVec );
		printf( " doors %s\n", doorsOpen ? "open" : "closed" );
		printf( " 0         10        20\n" );
		printf( " 012345678901234567890123456789\n" );
		for( int j=0; j<MAPY; ++j ) {
			// Copy in the line.
			memcpy( buf, &gMap[MAPX*j], MAPX+1 );
			buf[MAPX]=0;

			#ifdef USE_PATHER
			unsigned k;
			// Wildly inefficient demo code.
			unsigned size = path.size();
			for( k=0; k<size; ++k ) {
				int x, y;
				NodeToXY( path[k], &x, &y );
				if ( y == j )
					buf[x] = '0' + k%10;
			}
			if ( showConsidered )
			{
    			for( k=0; k<stateVec.size(); ++k ) {
           			int x, y;
    				NodeToXY( stateVec[k], &x, &y );
    				if ( y == j )
    					buf[x] = 'x';
        		}     
      		}  		
			#endif
			
			// Insert the player
			if ( j==playerY )
				buf[playerX] = 'i';

			printf( "%d%s\n", j%10, buf );
		}
	}

#ifdef USE_PATHER

	void NodeToXY( void* node, int* x, int* y ) 
	{
		intptr_t index = (intptr_t)node;
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

		/* Compute the minimum path cost using distance measurement. It is possible
		   to compute the exact minimum path using the fact that you can move only 
		   on a straight line or on a diagonal, and this will yield a better result.
		*/
		int dx = xStart - xEnd;
		int dy = yStart - yEnd;
		return (float) sqrt( (double)(dx*dx) + (double)(dy*dy) );
	}

	virtual void AdjacentCost( void* node, micropather::MPVector< StateCost > *neighbors ) 
	{
		int x, y;
		const int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
		const int dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
		const float cost[8] = { 1.0f, 1.41f, 1.0f, 1.41f, 1.0f, 1.41f, 1.0f, 1.41f };

		NodeToXY( node, &x, &y );

		for( int i=0; i<8; ++i ) {
			int nx = x + dx[i];
			int ny = y + dy[i];

			int pass = Passable( nx, ny );
			if ( pass > 0 ) {
				if ( pass == 1 || doorsOpen ) 
				{
					// Normal floor
					StateCost nodeCost = { XYToNode( nx, ny ), cost[i] };
					neighbors->push_back( nodeCost );
				}
				else 
				{
					// Normal floor
					StateCost nodeCost = { XYToNode( nx, ny ), FLT_MAX };
					neighbors->push_back( nodeCost );
				}
			}
		}
	}

	virtual void PrintStateInfo( void* node ) 
	{
		int x, y;
		NodeToXY( node, &x, &y );
		printf( "(%d,%d)", x, y );
	}

#endif
};

int main( int /*argc*/, const char** /*argv*/ )
{
	Dungeon dungeon;
	bool done = false;
	char buf[ 256 ];

	while ( !done ) {
		dungeon.Print();
		printf( "\n# # to move, q to quit, r to redraw, d to toggle doors, t for touched\n" );
		//gets( buf );
		//printf( "\n" );

		std::cin.getline( buf, 256 );

		if ( *buf )
		{
			if ( buf[0] == 'q' ) {
				done = true;
			}
			else if ( buf[0] == 'd' ) {
				dungeon.ToggleDoor();
				dungeon.ClearPath();
			}
			else if ( buf[0] == 't' ) {
				dungeon.ToggleTouched();   
			}    
			else if ( buf[0] == 'r' ) {
				dungeon.ClearPath();
			}
			else if ( isdigit( buf[0] ) ) {
				int x, y;
				sscanf( buf, "%d %d", &x, &y );	// sleazy, I know
				dungeon.SetPos( x, y );
			} 
		}
		else
		{				
			dungeon.ClearPath();
		}
	}
	return 0;
}
