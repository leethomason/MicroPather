MicroPather
===========

MicroPather is a path finder and A* solver (astar or a-star) written in platform 
independent C++ that can be easily integrated into existing code. MicroPather 
focuses on being a path finding engine for video games but is a generic A* solver. 
MicroPather is open source, with a license suitable for open source or commercial 
use.

The goals of MicroPather are:
* Easy integration into games and other software
* Easy to use and simple interface
* Fast enough

Demo
----

MicroPather comes with a demo application - dungeon.cpp - to show off pathing. 
It's ASCII art dungeon exploring at its finest.

The demo shows an ASCII art dungeon. You can move around by typing a new location, and it will 
print the path to that location. In the screen shot above, the path starts in 
the upper left corner, and steps to the 'i' at about the middle of 
the screen avoiding ASCII walls on the way. The numbers show the path from 0 
to 9 then back to 0.

You can even open and close the doors to change possible paths. 'd' at the command prompt.

A Windows Visual C++ 2010 project file and a Linux Makefile are provided. Building it 
for another environment is trivial: just compile dungeon.cpp, micropather.h, and 
micropather.cpp to a command line app in your environment.

About A*
--------
In video games, the pathfinding problem comes up in many modern games. What 
is the shortest distance from point A to point B? Humans are good at that problem 
- you pathfind naturally almost every time you move - but tricky to express 
as a computer algorithm. A* is the workhorse technique to solve pathing. It 
is directed, meaning that it is optimized to find a solution quickly rather 
than by brute force, but it will never fail to find a solution if there is one.

A* is much more universal that just pathfinding. A* and MicroPather could be 
used to find the solution to general state problems, for example it could be 
used to solve for a rubiks cube puzzle.

Terminology
-----------

The *Graph* is the search space. For the pathfinding problem, 
this is your Map. It can be any kind of map: squares like the dungeon example, 
polygonal, 3D, hexagons, etc.

In pathfinding, a *State* is just a position on the Map. In 
the demo, the player starts at State (0,0). Adjacent states are very important, 
as you might image. If something is at state (1,1) in the dungeon example, 
it has 8 adjacent states (0,1), (2,1) etc. it can move to. Why State instead 
of location or node? The terminology comes from the more general application. 
The states of a cube puzzle aren't locations, for example.

States are separated by *Cost*. For simple pathfinding in 
the dungeon, the *Cost* is simply distance. The cost from state 
(0,0) to (1,0) is 1.0, and the cost from (0,0) to (1,1) is sqrt(2), about 
1.4. *Cost* is challenging and interesting because it can be 
distance, time, or difficulty.
* using distance as the cost will give the shortest length path
* using traversal time as the cost will give the fastest path
* using difficulty as the cost will give the easiest path
etc.

More info: http://www-cs-students.stanford.edu/~amitp/gameprog.html#paths

Integrating MicroPather Into Your Code
--------------------------------------
Nothing could by simpler! Or at least that's the goal. More importantly, none 
of your game data structures need to change to use MicroPather. The steps, in 
brief, are:

1. Include MicroPather files</p>
2. Implement the Graph interface</p>
3. Call the Solver</p>

*Include files*

There are only 2 files for micropather: micropather.cpp and micropather.h. 
So there's no build, no make, just add the 2 files to your project. That's it. 
They are standard C++ and don't require exceptions or RTTI. (I know, a bunch 
of you like exceptions and RTTI. But it does make it less portable and slightly 
slower to use them.)

Assuming you build a debug version of your project with _DEBUG or DEBUG (and 
everyone does) MicroPather will run extra checking in these modes.

*Implement Graph Interface*

You have some class called Game, or Map, or World, that organizes and stores 
your game map. This object (we'll call it Map) needs to inherit from the abstract 
class Graph:

	class Map : public Graph

Graph is pure abstract, so your map class won't be changed by it (except for 
possibly gaining a vtable), or have strange conflicts.
Before getting to the methods of Graph, lets think states, as in:

	void Foo( void* state )
	
The state pointer is provided by you, the game programmer. What it is? It is 
a unique id for a state. For something like a 3D terrain map, like Lilith3D 
uses, the states are pointers to a map structure, a 'QuadNode' in 
this case. So the state would simply be:

	void* state = (void*) quadNode;
	
On the other hand, the Dungeon example doesn't have an object per map location, 
just an x and y. It then uses:

	void* state = (void*)( y * MAPX + x );
	
The state can be anything you want, as long as it is unique and you can convert 
to it and from it.

Now, the methods of Graph.

	/**
		Return the least possible cost between 2 states. For example, if your pathfinding 
		is based on distance, this is simply the straight distance between 2 points on the 
		map. If you pathfinding is based on minimum time, it is the minimal travel time 
		between 2 points given the best possible terrain.
	*/
	virtual float LeastCostEstimate( void* stateStart, void* stateEnd ) = 0;

	/** 
		Return the exact cost from the given state to all its neighboring states. This
		may be called multiple times, or cached by the solver. It *must* return the same
		exact values for every call to MicroPather::Solve(). It should generally be a simple,
		fast function with no callbacks into the pather.
	*/	
	virtual void AdjacentCost( void* state, MP_VECTOR< micropather::StateCost > *adjacent ) = 0;

	/**
		This function is only used in DEBUG mode - it dumps output to stdout. Since void* 
		aren't really human readable, normally you print out some concise info (like "(1,2)") 
		without an ending newline.
	*/
	virtual void  PrintStateInfo( void* state ) = 0;

*Call the Solver*

	MicroPather* pather = new MicroPather( myGraph );	// Although you really should set the default params for your game.
	
	micropather::MPVector< void* > path;
	float totalCost = 0;
	int result = pather->Solve( startState, endState, &path, &totalCost );

That's it. Given the start state and the end state, the sequence of states 
from start to end will be written to the vector.

MicroPather does a lot of caching. You want to create one and keep in around.
It will cache lots of information about your graph, and get faster as it is 
called. However, for caching to work, the connections between states and the 
costs of those connections must stay the same. (Else the cache information will 
be invalid.) If costs between connections does change, be sure to call Reset().

	pather->Reset();

Reset() is a fast call if it doesn't need to do anything.

Future Improvements and Social Coding
-------------------------------------

I really like getting patches, improvements, and performance enhancements. 
Some guidelines:
* Pull requests are the best way to send a change.
* The "ease of use" goal is important to this project. It can be sped up by
  deeper integration into the client code (all states must subclass the State
  object, for example) but that dramatically reduces usability.

Thanks for checking out MicroPather!

Lee Thomason
 